/*
 * Copyright (c) 2016 Renat R. Dusaev <crank@qcrypt.org>
 * Author: Renat R. Dusaev <crank@qcrypt.org>
 * Author: Bogdan Vasilishin <togetherwith@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

# ifndef H_PIPE_T_H
# define H_PIPE_T_H

# include "pipet.tcc"

# include <queue>

namespace pipet {

namespace helpers {
/// This class represents a combination of arbiter, pipeline and
/// source. Instances of this class is usually constructed as an interim
/// proxying object in complex expressions.
template< typename PipelineT
        , typename SourceT
        , typename ArbiterT=GenericArbiter<int>
        >
class EvaluationProxy {
public:
    typedef PipelineT Pipeline;
    typedef SourceT Source;
    typedef ArbiterT Arbiter;
private:
    Pipeline & _p;
    Source * _sPtr;
    Arbiter _arbiter;
    typename Arbiter::LoopResult _latestResult;
public:
    EvaluationProxy(Pipeline & p) : _p(p), _sPtr(nullptr) {}
    EvaluationProxy(Pipeline & p, Source & src) : _p(p), _sPtr(&src) {}
    
    Pipeline & pipeline() { return _p; }
    Source & source() { return *_sPtr; }
    Arbiter & arbiter() { return _arbiter; }
};  // class EvaluationProxy

/// Proxying class representing temporary object collecting single events given
/// to pipeline with left bitwise shift operator.
template< typename PipelineT
        , typename ArbiterT=GenericArbiter<int> >
class ThinEvaluationProxy : public std::queue<const typename PipelineT::Message *>
                          , public interfaces::Source<typename PipelineT::Message> {
public:
    typedef PipelineT Pipeline;
    typedef ArbiterT Arbiter;
    typedef ThinEvaluationProxy<Pipeline, Arbiter> Self;
    typedef typename Pipeline::Message Message;
    typedef std::queue<const typename PipelineT::Message *> Parent;
private:
    Pipeline & _p;
    Message * _reentrantMessagePtr;
    Arbiter _arbiter;
public:
    ThinEvaluationProxy( Pipeline & ppl ) : _p(ppl), _reentrantMessagePtr(nullptr) {}
    virtual Message * get() override {
        if( Parent::empty() ) {
            return nullptr;
        }
        if( !_reentrantMessagePtr ) {
            aux::MessageTraits<Message>::delete_copy( _reentrantMessagePtr );
        }
        _reentrantMessagePtr = aux::MessageTraits<Message>::copy( *Parent::front() );
        Parent::pop();
        return _reentrantMessagePtr;
    }
    ~ThinEvaluationProxy() {
        if( !_reentrantMessagePtr ) {
            aux::MessageTraits<Message>::delete_copy( _reentrantMessagePtr );
        }
    }

    Arbiter & arbiter() { return _arbiter; }
    Pipeline & pipeline() { return _p; }
};  // class ThinEvaluationProxy
}  // namespace helpers

template< typename PipelineT
        , typename ArbiterT >
helpers::ThinEvaluationProxy<PipelineT, ArbiterT>
operator<<( helpers::ThinEvaluationProxy<PipelineT, ArbiterT> && ep
          , const typename PipelineT::Message & msg) {
    ep.push( &msg );
    return ep;
}

template< typename PipelineT
        , typename ArbiterT >
helpers::ThinEvaluationProxy<PipelineT, ArbiterT>
operator>>( helpers::ThinEvaluationProxy<PipelineT, ArbiterT> && ep
          , typename helpers::ThinEvaluationProxy<PipelineT, ArbiterT>::Message & msgRef) {
    PipelineT::TheHandlerTraits::pull_one( ep.arbiter()
                                               , ep.pipeline().upcast()
                                               , ep
                                               , msgRef );
    // TODO: check that pull was good
    return ep;
}

/// Overloaded bitwise-OR operator with source as left operand and pipeline
/// object as right operand constructs the EvaluationProxy instance that
/// further may be used for sequential extraction of messages successfully
/// passing the pipeline with right bitwise shift operator (>>).
template< typename PipelineT
        , typename SourceT >
helpers::EvaluationProxy< PipelineT
               , SourceT >
operator|( SourceT & src, PipelineT & ppl ) {
    return helpers::EvaluationProxy<PipelineT, SourceT>( ppl, src );
}

template< typename PipelineT
        , typename SourceT >
helpers::EvaluationProxy< PipelineT
               , SourceT >
operator>>( helpers::EvaluationProxy< PipelineT
                           , SourceT > && ep
          , typename PipelineT::Message & msgRef) {
    typedef helpers::EvaluationProxy<PipelineT, SourceT> TheEvaluationProxy;
    typename TheEvaluationProxy::Arbiter::LoopResult rc =
        PipelineT::TheHandlerTraits::pull_one( ep.arbiter()
                                             , ep.pipeline().upcast()
                                             , ep.source()
                                             , msgRef );
    return ep;
}

}  // namespace pipet

# endif  // H_PIPE_T_H

