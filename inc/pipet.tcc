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

namespace pipet {

/// This class represents a combination of arbiter, pipeline and
/// source. Instances of this class is usually constructed as an interim
/// proxying object in complex expressions.
template< typename PipelineT
        , typename SourceT
        , typename ArbiterT=interfaces::GenericArbiter<int>
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
    Source & source() { return _sPtr; }
    Arbiter & arbiter() { return _arbiter; }
};  // class EvaluationProxy

/// Overloaded bitwise-OR operator with source as left operand and pipeline
/// object as right operand constructs the EvaluationProxy instance that
/// further may be used for sequential extraction of messages successfully
/// passing the pipeline with right bitwise shift operator (>>).
template< typename PipelineT
        , typename SourceT >
EvaluationProxy< PipelineT
               , SourceT >
operator|( SourceT & src, PipelineT & ppl ) {
    return EvaluationProxy( ppl, src );
}

template< typename PipelineT
        , typename SourceT >
EvaluationProxy< PipelineT
               , SourceT >
operator>>( EvaluationProxy< PipelineT
                           , SourceT > ep
          , MessageT & msgRef) {
    typedef EvaluationProxy<PipelineT, SourceT> TheEvaluationProxy;
    TheEvaluationProxy::Arbiter::LoopResult rc =
        PipelineT::TheHandlerTraits::pull_one( ep.arbiter()
                                             , ep.pipeline()
                                             , ep.source()
                                             , msgRef );
}

}  // namespace pipet

# endif  // H_PIPE_T_H

