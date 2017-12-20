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

# ifndef H_PIPE_T_PIPELINE_H
# define H_PIPE_T_PIPELINE_H

# include "basic_pipeline.tcc"

# include <stack>
# include <utility>
# include <type_traits>

namespace pipet {

/// @brief Pipeline evaluation steering codes.
/// Possible result flags and composite shortcuts that may be returned by
/// procedure. Note, that flags itself may be counter-intuitive, so better
/// stand for shortcuts (the have RC_ prefix). Priority of treatment:
/// 1. Global abort --- stop processing, gently finalize all the processing
/// handlers (flush buffers, build plots, close files, etc).
/// 2. Event abort  --- no further treatment of current shall be performed.
/// No modification have to be considered.
/// 3. Modification --- shall propagate modified flag to caller.
/// Note: all modification will be refused upon abort/discrimination.
enum struct PipeRC : int8_t {
    AbortAll  = 0x0,
    f_NextMessage = 0x1,
    f_NextHandler = 0x2,
    f_MessageHold = 0x4,
    Continue    = f_NextMessage | f_NextHandler,
    MessageKept = f_NextMessage | f_MessageHold,
    Complete    = f_NextHandler | f_MessageHold
};

inline bool operator & (PipeRC lhs, PipeRC rhs) {
    return (static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

/// This template provides extended base for handlers inside the pipeline
/// assembly.
template< typename MessageT
        , typename ResultT>
class iPipeHandler : public iBasicHandler<MessageT, ResultT> {
public:
    typedef interfaces::Source<MessageT> ISource;
    typedef iBasicHandler<MessageT, ResultT> Parent;
    typedef typename Parent::Message Message;
private:
    ISource * _castCache;
protected:
    template< typename T>
    typename std::enable_if<std::is_polymorphic<T>::value, ISource *>::type _junction_ptr( T & srcRef ) {
        return dynamic_cast<ISource*>(& srcRef);
    }
    template< typename T>
    typename std::enable_if<!std::is_polymorphic<T>::value, ISource *>::type _junction_ptr( T & ) {
        return nullptr;
    }
public:
    iPipeHandler() : _castCache(nullptr) {}
    template<typename T> iPipeHandler( T & cRef )
                    : Parent(cRef)
                    , _castCache( _junction_ptr( cRef ) ) {
    }

    /// Returns nullptr for handlers that aren't fork/junction processors.
    virtual ISource * junction_ptr() {
        return _castCache;
    }
};  // class iPipeHandler

template<typename ResT>
class GenericArbiter : public interfaces::Arbiter<PipeRC, ResT> {
public:
    typedef ResT LoopResult;
private:
    bool _doAbort
       , _doSkip
       , _forkFilled
       ;
protected:
    virtual void _reset_flags() {
        _doAbort = _doSkip = _forkFilled = false;
    }
public:
    GenericArbiter() {
        _reset_flags();
    }
    /// Causes transitions
    virtual bool consider_handler_result( PipeRC fs ) override {
        _doAbort = !((PipeRC::f_NextMessage & fs) | (PipeRC::f_NextHandler & fs));
        _doSkip = !(PipeRC::f_NextMessage & fs);
        _forkFilled = (PipeRC::f_MessageHold & fs) && (PipeRC::f_NextHandler & fs);
        return PipeRC::f_NextHandler & fs;
    }
    virtual bool next_message() override {
        return !_doSkip;
    }
    virtual bool previous_is_full() const {
        return _forkFilled;
    }
    /// This is the single method that has to be overriden by particular
    /// descendant.
    virtual ResT pop_result() override {
        if( !_doAbort ) {
            return ResT( 0 );
        } else {
            return ResT( -1 );
        }
    }

    bool do_skip() const { return _doSkip; }
    bool do_abort() const { return _doAbort; }
};

template<typename MessageT>
struct HandlerTraits< MessageT
                    , PipeRC
                    , iPipeHandler > {
    typedef MessageT                                    Message;
    typedef PipeRC                                      HandlerResult;
    typedef iPipeHandler<Message, HandlerResult>        AbstractHandler;
    typedef AbstractHandler *                           AbstractHandlerRef;

    template<typename CallableT>
    class Handler : public PrimitiveHandler<Message
                                           , HandlerResult
                                           , CallableT
                                           , iPipeHandler> {
    public:
        typedef PrimitiveHandler<Message
                                , HandlerResult
                                , CallableT
                                , iPipeHandler> Parent;
        typedef typename Parent::CallableRef CallableRef;
    public:
        Handler( CallableRef pRef ) : Parent( pRef ) {}
    };

    template< template <typename...> class ChainT
            , typename LoopResultT
            , typename SourceT
            , typename ... ChainTArgs
            >
    static LoopResultT process( GenericArbiter<LoopResultT> &a
                              , ChainT<AbstractHandlerRef, ChainTArgs...> & chain
                              , SourceT && src );

    template< template <typename...> class ChainT
            , typename LoopResultT
            , typename SourceT
            , typename ... ChainTArgs
            >
    static LoopResultT pull_one( GenericArbiter<LoopResultT> &a
                               , ChainT<AbstractHandlerRef, ChainTArgs...> & chain
                               , SourceT && src
                               , MessageT & targetMessage );
    template<typename LoopResultT> using IArbiter = GenericArbiter<LoopResultT>;
};

template<>
struct HandlerResultConverter<PipeRC, void> {
    virtual PipeRC convert( void ) {
        return PipeRC::Continue;
    }
};

template<>
struct HandlerResultConverter<PipeRC, bool> {
    virtual PipeRC convert( bool v ) {
        return v ? PipeRC::Continue : PipeRC::f_NextMessage;
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// Processing routines specialization for HandlerTraits specialized for
// iPipeHandler:

/// Processing function for non-linear pipeline evaluating on source (complex
/// case, for iPipeHandler)
template< typename MessageT>
template< template <typename...> class ChainT
        , typename LoopResultT
        , typename SourceT
        , typename ... ChainTArgs
        > LoopResultT
HandlerTraits< MessageT
             , PipeRC
             , iPipeHandler>::process( GenericArbiter< LoopResultT > & a
                                    , ChainT< AbstractHandlerRef, ChainTArgs... > & chain
                                    , SourceT && src ) {
    // Deduced chain (pipeline's iterable container) type
    typedef ChainT< AbstractHandlerRef, ChainTArgs... > Chain;
    // Messages source traits
    typedef aux::SourceTraits< typename std::remove_reference<SourceT>::type
                             , MessageT> SrcTraits;
    // Handlers chain iterator (references particular handler in chain)
    typedef typename ChainT<AbstractHandlerRef, ChainTArgs...>::iterator ChainIterator;
    // Chain iteration state
    typedef std::pair< interfaces::Source<MessageT> *
                     , ChainIterator > SourceState;
    typename SrcTraits::Iterator lowestSourceIt(src);
    // The temporary sources stack keeping internal state. Has to be empty upon
    // finishing processing.
    std::stack<SourceState> sourcesStack;
    // First in stack will refer to original source.
    sourcesStack.push( SourceState( &lowestSourceIt, chain.begin() ) );
    // Begin main processing loop. Will run upon sources stack is non-empty AND
    // arbiter did not explicitly interrupt it.
    while( !sourcesStack.empty() ) {
        // Reference pointing to the current event source.
        interfaces::Source<MessageT> & cSrc = *sourcesStack.top().first;
        // Iterator pointing to the current handler in chain.
        typename Chain::iterator procStart =  sourcesStack.top().second;
        // Begin of loop iterating messages source.
        Message * msg;  // while(!!(msg = cSrc.next()))
        while( !! (msg = cSrc.get()) ) {
            typename Chain::iterator handlerIt;
            // Begin of loop iterating the handlers chain.
            for( handlerIt = procStart
               ; handlerIt != chain.end()
               ; ++handlerIt ) {
                AbstractHandler & h = **handlerIt;
                // Process message with current handler and consider result.
                if( a.consider_handler_result( h.process( *msg ) ) ) {
                    // consider_handler_result() returned true, what means we
                    // can propagate further along the handlers chain.
                    if( a.previous_is_full() ){
                        // Fork was filled and junction has merged events. It
                        // means that we have to proceed with events that were put
                        // in "junction" queue as if it is an event source.
                        # ifndef NDEBUG
                        if( !h.junction_ptr() ) {
                            // This excepetion matters since we're dealing with
                            // potentially customized behaviour (f/j behaviour
                            // may be re-defined at upper levels).
                            pipet_error( Malfunction, "Handler %p (%zu in "
                                    "chain) can not act as an event source, "
                                    "but had returned the "
                                    "\"fork finalized\" code.",
                                    &(*handlerIt), handlerIt - chain.begin() );
                            // TODO: ^^^ try to get class name
                        }
                        # endif
                        sourcesStack.push(
                                std::make_pair(h.junction_ptr(), handlerIt+1));
                        break;  // get to the source-iterating loop
                    }
                    continue;  // next handler
                }
                // _V_consider_handler_result() returned false, that means we have
                // to interrupt the propagation, but if we have filled the
                // f/j handler, it must be put on top of sources stack.
                break;
            }  // handler iteration loop
            if( !a.next_message() ) {
                // We have to take next (newly-created) source from internal
                // queue and proceed with it.
                break;
            }
        }
        // 
        if(!msg) {
            sourcesStack.pop();
        }
    }
    if( !a.next_message() || chain.empty() ) {
        return a.pop_result();
    }
    // Try to locate f/j handler inside the chain to use it as an event source.
    // Return loop result, if end of chain is reached.
    ChainIterator fjIt = chain.begin();
    while( !((*fjIt)->junction_ptr()) ) {
        if( ++fjIt == chain.end() ) {
            return a.pop_result();
        }
    }
    Chain subChain( fjIt + 1, chain.end() );
    return process( a, subChain, *((*fjIt)->junction_ptr()) );
}

template< typename MessageT>
template< template <typename...> class ChainT
        , typename LoopResultT
        , typename SourceT
        , typename ... ChainTArgs
        > LoopResultT
HandlerTraits< MessageT
             , PipeRC
             , iPipeHandler>::pull_one( GenericArbiter< LoopResultT > & a
                                      , ChainT< AbstractHandlerRef, ChainTArgs... > & chain
                                      , SourceT && src
                                      , MessageT & targetMessage ) {
    // Deduced chain (pipeline's iterable container) type
    typedef ChainT< AbstractHandlerRef, ChainTArgs... > Chain;
    do {
        std::stack<typename Chain::reverse_iterator> tStack;
        Message * msg = nullptr;
        // Iterate back from chain end
        for( auto it = chain.rbegin(); it != chain.rend(); ++it ) {
            interfaces::Source<Message> * srcPtr;
            // If handler may act like source
            if( !! (srcPtr = (*it)->junction_ptr()) ) {
                // If handler is able to emit a message
                if( !! (msg = srcPtr->get()) ) {
                    // We've got one message --- break the iteration loop
                    break;
                }
            }
            tStack.push( it );
        }
        if( !msg ) {
            if( ! (msg = src.get()) ) {
                // Even given source is unable to return the event --- abort.
                // return a.pop_result();
                // ^^^^^^ We may further use "noexcept" behaviour for
                // applications. Should it be controlled by arbiter (todo)?
                throw pipet::errors::UnableToPull( &src );
            }
        }
        while( !tStack.empty() ) {
            if( a.consider_handler_result( (*tStack.top())->process( *msg ) ) ) {
                tStack.pop();
                // ok, invoke next handler
                continue;
            }
            // (Dev note) this commented code block should give a gist of
            // "greed" strategy aspect for future development.
            //if( a.fork_filling() && a.is_greedy() ) {
            //    // Abort caused by accumulating handler and greedy strategy
            //    // is chosen. Current loop shall request next message.
            //    it = tStack.rbegin();
            //} else {
            //    break;  // restart chain iteration
            //}
            // Current message propagation has to be aborted
            break;
        }
        if( tStack.empty() ) {
            // Means, the message has passed all the chain and may be
            // considered as a result.
            targetMessage = *msg;
            break;
        }
    } while( a.next_message() );
    return a.pop_result();
}



template<typename MessageT> using Pipe = Pipeline< iPipeHandler
                                                 , MessageT
                                                 , PipeRC >
                                                 ;

}  // namespace pipet

# if 0
template< typename PipelineT
        , typename MessageT
        , typename ArbiterT=pipet::GenericArbiter<int>
        >
typename ArbiterT::LoopResult
operator<<( PipelineT & p,  & src ) {
    ArbiterT a;
    return PipelineT::TheHandlerTraits::process(
            a, p.upcast(), src );
}
# endif

# endif  // H_PIPE_T_PIPELINE_H

