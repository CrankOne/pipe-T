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

# ifndef H_PIPE_T_MANIFOLD_H
# define H_PIPE_T_MANIFOLD_H

# include "basic_pipeline.tcc"

# include <stack>
# include <utility>

namespace pipet {

template< typename MessageT
        , typename ResultT> class Manifold;

namespace aux {

/// Possible result flags and composite shortcuts that may be returned by
/// procedure. Note, that flags itself may be counter-intuitive, so better
/// stand for shortcuts (the have RC_ prefix). Priority of treatment:
/// 1. Global abort --- stop processing, gently finalize all the processing
/// handlers (flush buffers, build plots, close files, etc).
/// 2. Event abort  --- no further treatment of current shall be performed.
/// No modification have to be considered.
/// 3. Modification --- shall propagate modified flag to caller.
/// 4. Discriminate --- shall pull out the event/sub-event/sample. Arbiter,
/// depending of its modification, may keep process this event.
/// Note: all modification will be refused upon abort/discrimination.
enum ManifoldRC : int8_t {
    RC_AbortAll     = 0x0,
    kNextMessage    = 0x1,
    kNextHandler    = 0x2,
    kForkFilled     = 0x4,
    RC_Continue     = kNextMessage | kNextHandler
};

# if 0
template<typename MessageT>
struct iManifoldProcessor {
public:
    typedef MessageT Message;
public:
    virtual ManifoldRC operator()( Message & ) = 0;
};
# endif

template< typename MessageT
        , typename DesiredProcessingResultT
        , typename RealProcessingResultT
        , typename ProcessorT>
class ManifoldHandler : public PipelineHandler< MessageT
                                              , DesiredProcessingResultT
                                              , RealProcessingResultT
                                              , ProcessorT> {
public:
    typedef PipelineHandler< MessageT
                           , DesiredProcessingResultT
                           , RealProcessingResultT
                           , ProcessorT> Parent;
    typedef typename Parent::Message Message;
    typedef DesiredProcessingResultT ProcRes;
    typedef ProcessorT Processor;
    typedef typename Parent::ProcessorRef ProcessorRef;
public:
    ManifoldHandler( ProcessorRef p ) : Parent(p) {}
};

template< typename MessageT
        , typename ManifoldResultT
        , template<typename T> class TChainT=aux::STLAllocatedVector>
struct ManifoldTraits : protected PipelineTraits< MessageT
                                                , ManifoldRC
                                                , ManifoldResultT > {
    /// Self traits typedef.
    typedef ManifoldTraits<MessageT, ManifoldResultT> Self;
    /// Parent (hidden) typedef.
    typedef PipelineTraits< MessageT
                          , ManifoldRC
                          , ManifoldResultT > Parent;
    /// Message type (e.g. physical event).
    typedef typename Parent::Message   Message;
    /// Result type, returning by handler call.
    typedef typename Parent::ProcRes   ProcRes;
    /// Result type, that shall be returned by pipeline invokation.
    typedef typename Parent::PipelineProcRes PipelineProcRes;
    /// Base source interface that has to be implemented.
    typedef typename Parent::ISource ISource;
    // Concrete handler interfacing type.
    class AbstractHandler : public Parent::AbstractHandler {
    private:
        ISource * _jSrc;
    public:
        //Handler( Processor * p ) : aux::PipelineHandler<Self>( p ) {
        //    _jSrc = dynamic_cast<ISource *>(p);
        //}
        /// Will return pointer to junction as a source.
        virtual ISource * junction_ptr() {
            return _jSrc;
        }
    };
    /// Concrete chain interfacing type (parent for this class).
    typedef TChainT<AbstractHandler *> Chain;
    /// This IArbiter interface introduces additional is_fork_filled() method
    /// that returns true, if latest handler result raised the JUNCTION_DONE
    /// flag.
    struct IArbiter : public Parent::IArbiter {
    private:
        bool _doAbort
           , _doSkip
           , _forkFilled
           ;
    protected:
        virtual void _reset_flags() {
            _doAbort = _doSkip = _forkFilled = false;
        }
        //virtual PipelineProcRes _V_
    public:
        /// Causes transitions
        virtual bool _V_consider_handler_result( ManifoldRC fs ) override {
            _doAbort = !((kNextMessage & fs) | (kNextHandler & fs));
            _forkFilled = kForkFilled & fs;
            return kNextMessage & fs;
        }
        virtual bool _V_next_message() override {
            return !_doAbort;
        }
        virtual bool is_fork_filled() const {
            return _forkFilled;
        }
        // This is the single method that has to be overriden by particular
        // descendant.
        //virtual PipelineProcRes _V_pop_result() = 0;
    public:
        bool do_skip() const { return _doSkip; }
        bool do_abort() const { return _doAbort; }
        friend class Manifold<Message, ManifoldResultT>;
        friend class BasicPipeline<Self>;
    };  // class IArbiter

    /// This trait aliases a template class referencing end-point handler
    /// template.
    template< typename TMsgT
            , typename TDesiredProcResT
            , typename TRealProcResT
            , typename TCallableT > using HandlerTemplateClass = \
            aux::ManifoldHandler<TMsgT, TDesiredProcResT, TRealProcResT, TCallableT>;
};


template<>
struct HandlerResultConverter<ManifoldRC, void> {
    virtual ManifoldRC convert_result( void ) {
        return RC_Continue;
    }
};

template<>
struct HandlerResultConverter<ManifoldRC, bool> {
    virtual ManifoldRC convert_result( bool v ) {
        return v ? RC_Continue : kNextMessage;
    }
};

}  // namespace aux

/**@brief Assembly of pipelines with elaborated composition management.
 * @class Manifold
 *
 * The Manifold class manages assembly of pipelines, performing fork/junction
 * (de-)multiplexing, changing the event types and automated insertion of
 * auxilliary handlers.
 * */
template< typename MessageT
        , typename ResultT>
class Manifold : public BasicPipeline< aux::ManifoldTraits< MessageT
                                                     , ResultT
                                                     >
                                > {
public:
    typedef aux::ManifoldTraits<MessageT, ResultT> Traits;
    typedef BasicPipeline< aux::ManifoldTraits<MessageT, ResultT> > Parent;
    typedef typename Traits::Message   Message;
    typedef typename Traits::ProcRes   ProcRes;
    typedef typename Traits::PipelineProcRes PipelineProcRes;
    typedef typename Traits::AbstractHandler   AbstractHandler;
    typedef typename Traits::Chain     Chain;
    typedef typename Traits::ISource   ISource;
    typedef typename Traits::IArbiter  IArbiter;
    typedef Manifold<Message, PipelineProcRes> Self;

    class SingularSource : public ISource {
    private:
        bool _msgRead;
        Message & _msgRef;
    public:
        SingularSource( Message & msg ) : _msgRead(false), _msgRef(msg) {}
        virtual Message * next() override {
            if( !_msgRead ) {
                _msgRead = true;
                return &_msgRef;
            }
            return nullptr;
        }
    };  // class SingularSource
public:
    Manifold( IArbiter * a ) : Parent( a ) {}
    /// Manifold overloads the source processing method to support fork/junction
    /// processing.
    virtual PipelineProcRes process( ISource & src ) override {
        if( ! this->arbiter_ptr() ) {
            pipet_error( Uninitialized, "Arbiter object pointer is not set for "
                    "pipeline instance %p while process() was invoked.",
                    this );
        }
        IArbiter & a = *(this->arbiter_ptr());
        // Check if we actually have something to do
        if( Chain::empty() ) {
            pipet_error( EmptyManifold, "Manifold instance %p has no handlers set.",
                    this );
        }
        // The temporary sources stack keeping internal state. Has to be empty upon
        // finishing processing.
        std::stack< std::pair<ISource *, typename Chain::iterator> > sourcesStack;
        // First in stack will refer to original source.
        sourcesStack.push( std::make_pair( &src, Chain::begin() ) );
        // Begin main processing loop. Will run upon sources stack is non-empty AND
        // arbiter did not explicitly interrupt it.
        while( !sourcesStack.empty() ) {
            // Reference pointing to the current event source.
            ISource & cSrc = *sourcesStack.top().first;
            // Iterator pointing to the current handler in chain.
            typename Chain::iterator procStart =  sourcesStack.top().second;
            sourcesStack.pop();
            // Begin of loop iterating messages source.
            Message * msg;
            while(!!(msg = cSrc.next())) {
                typename Chain::iterator handlerIt;
                // Begin of loop iterating the handlers chain.
                for( handlerIt = procStart
                   ; handlerIt != Chain::end()
                   ; ++handlerIt ) {
                    // Process message with current handler and consider result.
                    if( a._V_consider_handler_result( handlerIt->process( *msg ) ) ) {
                        // _V_consider_handler_result() returned true, what means we
                        // can propagate further along the handlers chain.
                        continue;
                    }
                    // _V_consider_handler_result() returned false, that means we have
                    // to interrupt the propagation, but if we have filled the
                    // f/j handler, it must be put on top of sources stack.
                    if( a.is_fork_filled() ) {
                        // Fork was filled and junction has merged events. It
                        // means that we have to proceed with events that were put
                        // in "junction" queue as if it is an event source.
                        sourcesStack.push(
                            std::make_pair( handlerIt->junction_ptr(), ++handlerIt) );
                    }
                    break;
                }
            }
            if( a._V_next_message() ) {
                // We have to take next (newly-created) source from internal
                // queue and proceed with it.
                break;
            }
        }
        return a._V_pop_result();
    }
    /// Single message will be processed as a (temporary) messages source
    /// containing only one event.
    virtual PipelineProcRes process( Message & msg ) override {
        auto tSrc = SingularSource(msg);
        auto res = Self::process( tSrc );
        return res;
    }
};  // class ForkProcessor

# if 0
template<typename MessageT>
class SubManifold : public Manifold< MessageT
                                   , aux::ManifoldRC>
                  , public aux::iManifoldProcessor<MessageT> {
public:
    typedef typename Manifold< MessageT
                             , aux::ManifoldRC>::Traits Traits;
    class Arbiter : public Traits::IArbiter {
    protected:
        virtual aux::ManifoldRC _V_pop_result() override {
            pipet_error( NotImplemented, "Function is not yet implemented." );  // TODO
        }
    };
public:
    virtual aux::ManifoldRC operator()( MessageT & msg ) override {
        return this->process(msg);
    }
};
# endif

//template< typename MessageT
//        , typename ResultT> ResultT
//Manifold<MessageT, ResultT>::process( ISource & src ) 

}  // namespace pipet

# endif  // H_PIPE_T_MANIFOLD_H

