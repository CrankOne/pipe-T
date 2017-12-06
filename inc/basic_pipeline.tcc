/*
 * Copyright (c) 2016 Renat R. Dusaev <crank@qcrypt.org>
 * Author: Renat R. Dusaev <crank@qcrypt.org>
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

# ifndef H_PIPE_T_BASIC_PIPELINE_H
# define H_PIPE_T_BASIC_PIPELINE_H

# include "pipe-t-error.hpp"

# include <type_traits>
# include <vector>

namespace pipet {

template<typename PipelineTraitsT> class BasicPipeline;

namespace aux {

template<typename T>
using STLAllocatedVector = std::vector<T>;

}  // namespace aux

/// The most basic pipeline handler class. Is an abstract base for linear
/// pipeline.
template< typename MessageT
        , typename ProcessingResultT>
class iHandler {
public:
    typedef MessageT Message;
    typedef ProcessingResultT ProcRes;
public:
    virtual ~iPipelineHandler() {}
    virtual ProcRes process( Message & ) = 0;

    # if 0
    template<typename T> typename aux::ProcessorTraits< T
                                                      , Message
                                                      , ProcessingResultT
                                                      , PipelineHandler
                                                      , iPipelineHandler
                                                      >::ProcessorRef
    processor() {
        return dynamic_cast<typename aux::ProcessorTraits< T
                                                         , Message
                                                         , ProcessingResultT
                                                         , PipelineHandler
                                                         , iPipelineHandler
                                                         >::Handler *>(this)->processor();
    }
    # endif
};  // class iBasicPipelineHandler

// ...
//template< typename MessageT
//        , typename ProcessingResultT
//        , typename RealProcessingResultT>
//class iWrappingPipelineHandler {
//public:
//    wrap_result<ProcessingResultT, RealProcessingResultT>
//};

template< typename DesiredT,
          typename RealT >
struct HandlerResultConverter;  // default has to be NOT implemented!

/**@brief Pipeline handler template wrapping the processing functor.
 * @class PipelineHandler
 *
 * Represents a concrete handler maintaining processor instance. This class
 * wraps the particular processor instance and forwards generalized invokations
 * from pipeline to particular processor types.
 * */
template< typename MessageT
        , typename DesiredProcessingResultT
        , typename RealProcessingResultT
        , typename ProcessorT
        , template< typename TMsgT
                  , typename TProcResT> class TAbstractHandlerT
        >
class PipelineHandler : public TAbstractHandlerT< MessageT
                                               , DesiredProcessingResultT>
                      , public HandlerResultConverter<DesiredProcessingResultT, RealProcessingResultT> {
public:
    typedef MessageT Message;
    typedef DesiredProcessingResultT ProcRes;
    typedef ProcessorT Processor;
    typedef TAbstractHandlerT<Message, DesiredProcessingResultT> Parent;
    typedef typename std::conditional<std::is_function<Processor>::value,
                                      ProcessorT,
                                      ProcessorT&>::type ProcessorRef;
    typedef HandlerResultConverter<DesiredProcessingResultT, RealProcessingResultT> ThisHandlerResultConverter;
private:
    ProcessorRef _p;
public:
    PipelineHandler( ProcessorRef p ) :_p(p) {}
    virtual ProcRes process( Message & m )
                { return ThisHandlerResultConverter::convert_result(_p( m )); }
    ProcessorRef processor() { return _p; }
    const ProcessorRef processor() const { return _p; }
};

// Template specialization for processor returning the same result as expected
// by pipeline.
template< typename MessageT
        , typename ProcessingResultT
        , typename ProcessorT
        , template< typename TMsgT
                  , typename TProcResT> class TAbstractHandlerT
        >
class PipelineHandler< MessageT
                     , ProcessingResultT
                     , ProcessingResultT
                     , ProcessorT
                     , TAbstractHandlerT> : public TAbstractHandlerT< MessageT
                                                             , ProcessingResultT> {
public:
    typedef MessageT Message;
    typedef ProcessingResultT ProcRes;
    typedef ProcessorT Processor;
    typedef TAbstractHandlerT<Message, ProcRes> Parent;
    typedef typename std::conditional<std::is_function<Processor>::value,
                                      ProcessorT,
                                      ProcessorT&>::type ProcessorRef;
private:
    ProcessorRef _p;
public:
    PipelineHandler( ProcessorRef p ) : _p(p) {}
    virtual ProcRes process( Message & m ) override { return _p( m ); }
    ProcessorRef processor() { return _p; }
    const ProcessorRef processor() const { return _p; }
};  // class PipelineHandler

}  // namespace aux

/**@brief Auxilliary scope-like struct containing key types for pipeline
 *        classes.
 * @class PipelineTraitsT
 *
 * The type traits is a common technique to summarize large set of C++ template
 * parameters in a neat manner.
 *
 * For instance, see: http://www.drdobbs.com/cpp/c-type-traits/184404270
 *
 * This interim traits are designed for BasicPipeline primitive template class.
 *
 * The PipelineProcResT type has to be compy-constructible.
 * The MessageT type has no any special restrictions.
 * The ProcResT type has to be copy-constructible.
 * */
template< typename MessageT
        , typename ProcResT
        , typename PipelineProcResT
        , template<typename T> class TChainT=aux::STLAllocatedVector >
struct PipelineTraits {
    /// Traits typedef.
    typedef PipelineTraits<MessageT, ProcResT, PipelineProcResT, TChainT> Self;
    /// Message type (e.g. physical event).
    typedef MessageT   Message;
    /// Result type, returning by handler call.
    typedef ProcResT   ProcRes;
    /// Result type, that shall be returned by pipeline invokation.
    typedef PipelineProcResT PipelineProcRes;
    /// Concrete handler interfacing type.
    typedef aux::iPipelineHandler<Message, ProcRes> AbstractHandler;
    /// Concrete chain interfacing type (parent for this class).
    typedef TChainT<AbstractHandler *> Chain;
    /// Base source interface that has to be implemented.
    struct ISource {
        virtual Message * next( ) = 0;
    };  // class ISource
    /// Interface making a decision, whether to continue processing on few
    /// stages.
    /// Invokation of _V_pop_result() is guaranteed at the ending of processing
    /// loop.
    class IArbiter {
    protected:
        /// Shall return true if result returned by current handler allows
        /// further propagation.
        virtual bool _V_consider_handler_result( ProcRes ) = 0;
        /// Shall return result considering current state and reset state.
        virtual PipelineProcRes _V_pop_result() = 0;
        /// Whether to retrieve next message from source and start
        /// event propagation.
        virtual bool _V_next_message() = 0;
        friend class BasicPipeline<Self>;
    };  // class IArbiter

    template< typename TMsgT
            , typename TProcResT> using AbstractHandlerTemplateClass =
            aux::iPipelineHandler<TMsgT, TProcResT >;

    /// This trait aliases a template class referencing end-point handler
    /// template.
    template< typename TMsgT
            , typename TDesiredProcResT
            , typename TRealProcResT
            , typename TCallableT > using HandlerTemplateClass = \
            aux::PipelineHandler< TMsgT
                                , TDesiredProcResT
                                , TRealProcResT
                                , TCallableT
                                , aux::iPipelineHandler >;
};


namespace aux {

template< typename CallableT
        , typename MessageT
        , typename DesiredProcResT
        , template< typename TMsgT
                  , typename TDesiredProcResT
                  , typename TRealProcResT
                  , typename TCallableT
                  , template<typename, typename> class TAbstractHandlerTT> class THandlerT
        , template<typename, typename> class TAbstractHandlerT
        >
struct ProcessorTraits {
    typedef typename std::result_of<CallableT(MessageT &)>::type RealProcRes;
    typedef THandlerT< MessageT
                     , DesiredProcResT
                     , RealProcRes
                     , CallableT
                     , TAbstractHandlerT > Handler;
    //typedef typename Handler::Parent AbstractHandler;
    typedef typename Handler::ProcessorRef ProcessorRef;
    
    template<typename AbstractHandlerT>
    static AbstractHandlerT * new_handler( ProcessorRef procRef ) {
        return new Handler( procRef );
    }
};

}  // namespace aux


/**@brief Strightforward pipeline template primitive.
 * @class BasicPipeline
 *
 * This is the basic implementation of pipeline that performs sequential
 * invokation of processing atoms stored at ordered container, guided by
 * arbitering class.
 * */
template< typename HandlerT
        , typename PipelineResultT
        , template<typename T> class TChainT=aux::STLAllocatedVector >
class BasicPipeline : public TChainT<typename HandlerTraits<HandlerT>::HandlerRef> {
public:
    typedef HandlerT Handler;
    typedef PipelineResultT PipelineResult;
    typedef HandlerTraits<Handler> TheHandlerTraits;
    // Other types are deduced using the TheHandlerTraits info:
    typedef typename TheHandlerTraits::Message          Message;
    typedef typename TheHandlerTraits::Result           Result;
    typedef typename TheHandlerTraits::AbstractHandler  AbstractHandler;
    typedef typename TheHandlerTraits::HandlerRef       HandlerRef;
    typedef typename TChainT<HandlerRef>                Chain;

    //typedef typename PipelineTraitsT::Message   Message;
    //typedef typename PipelineTraitsT::ProcRes   ProcRes;
    //typedef typename PipelineTraitsT::PipelineProcRes PipelineProcRes;
    //typedef typename PipelineTraitsT::AbstractHandler AbstractHandler;
    //typedef typename PipelineTraitsT::Chain     Chain;
    //typedef typename PipelineTraitsT::ISource   ISource;
    //typedef typename PipelineTraitsT::IArbiter  IArbiter;
private:
    IArbiter * _a;
protected:
    IArbiter * arbiter_ptr() { return _a; }
    const IArbiter * arbiter_ptr() const { return _a; }
public:
    /// Ctr. Requires an arbiter instance to act.
    BasicPipeline( IArbiter * a ) : _a(a) {}
    /// Virtual dtr (trivial).
    virtual ~BasicPipeline() {
        for( auto & ahPtr : *static_cast<Chain *>(this) ) {
            delete ahPtr;
        }
    }
    /// Run pipeline evaluation on source.
    virtual PipelineProcRes process( ISource & src ) {
        if( ! arbiter_ptr() ) {
            pipet_error( Uninitialized, "Arbiter object pointer is not set for "
                    "pipeline instance %p while process() was invoked.",
                    this );
        }
        IArbiter & a = *arbiter_ptr();
        Message * msg;
        while(!!(msg = src.next())) {
            for( AbstractHandler * h : *static_cast<Chain*>(this) ) {
                if( ! a._V_consider_handler_result( h->process(*msg) ) ) {
                    break;
                }
            }
            if( ! a._V_next_message() ) {
                break;
            }
        }
        return a._V_pop_result();
    }
    /// Run pipeline evaluation on single message.
    virtual PipelineProcRes process( Message & msg ) {
        if( ! arbiter_ptr() ) {
            pipet_error( Uninitialized, "Arbiter object pointer is not set for "
                    "pipeline instance %p while process() was invoked.",
                    this );
        }
        IArbiter & a = *arbiter_ptr();
        for( AbstractHandler * h : *static_cast<Chain*>(this) ) {
            if( ! a._V_consider_handler_result( h->process(msg) ) ) {
                break;
            }
        }
        return a._V_pop_result();
    }
    /// Shortcut for inserting processor at the back of pipeline.
    template<typename CallableT>
    void push_back( CallableT & p ) {
        Chain::push_back(
                aux::ProcessorTraits< CallableT
                                    , Message
                                    , ProcRes
                                    , PipelineTraitsT::template HandlerTemplateClass
                                    , PipelineTraitsT::template AbstractHandlerTemplateClass
                                    >::template new_handler<AbstractHandler>(p) );
    }
    /// Call operator for single message --- to use pipeline as a processor.
    PipelineProcRes operator()( Message & msg ) {
        return process( msg );
    }
};  // class BasicPipeline

}  // namespace pipet

# endif  // H_PIPE_T_BASIC_PIPELINE_H


