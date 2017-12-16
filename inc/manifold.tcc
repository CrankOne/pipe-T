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

# include <cstdio>  // XXX
# include <iostream>  // XXX

# include <stack>
# include <utility>

namespace pipet {

template< typename MessageT
        , typename ManifoldResultT
        , template<typename T> class TChainT=aux::STLAllocatedVector>
class Manifold;

namespace aux {

/// @brief Manifold evaluation steering codes.
/// Possible result flags and composite shortcuts that may be returned by
/// procedure. Note, that flags itself may be counter-intuitive, so better
/// stand for shortcuts (the have RC_ prefix). Priority of treatment:
/// 1. Global abort --- stop processing, gently finalize all the processing
/// handlers (flush buffers, build plots, close files, etc).
/// 2. Event abort  --- no further treatment of current shall be performed.
/// No modification have to be considered.
/// 3. Modification --- shall propagate modified flag to caller.
/// Note: all modification will be refused upon abort/discrimination.
enum ManifoldRC : int8_t {
    RC_AbortAll  = 0x0,
    kNextMessage = 0x1,
    kNextHandler = 0x2,
    kForkFill    = 0x4,
    RC_Continue    = kNextMessage | kNextHandler,
    RC_ForkFilling = kNextMessage | kForkFill,
    RC_ForkFilled  = kNextHandler | kForkFill
};

}  // namespace aux

/// This template provides extended base for handlers inside the manifold
/// pipeline assembly.
template< typename MessageT
        , typename ResultT>
class iManifoldHandler : public iBasicHandler<MessageT, ResultT> {
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
        //printf( ">>> %s\n", __PRETTY_FUNCTION__ );
        // This is a function, can not be casted to ISource type anyway.
        return nullptr;
    }
public:
    iManifoldHandler() : _castCache(nullptr) {}
    template<typename T> iManifoldHandler( T & cRef )
                    : Parent(cRef)
                    , _castCache( _junction_ptr( cRef ) ) {
    }

    /// Returns nullptr for handlers that aren't fork/junction processors.
    virtual ISource * junction_ptr() {
        return _castCache;
    }
};  // class iManifoldHandler

namespace interfaces {

template<typename ManifoldResT>
struct ManifoldArbiter : public Arbiter<aux::ManifoldRC, ManifoldResT> {
private:
    bool _doAbort
       , _doSkip
       , _forkFilled
       , _forkFilling
       ;
protected:
    virtual void _reset_flags() {
        _doAbort = _forkFilling = _doSkip = _forkFilled = false;
    }
public:
    /// Causes transitions
    virtual bool consider_handler_result( aux::ManifoldRC fs ) override {
        _doAbort = !((aux::kNextMessage & fs) | (aux::kNextHandler & fs));
        _doSkip = !(aux::kNextMessage & fs);
        _forkFilling = aux::kForkFill & fs;
        _forkFilled = (aux::kForkFill & fs) && (aux::kNextHandler & fs);
        std::cout << "--- --- arb got handler RC="
                  << std::hex << (int) fs
                  << " flags: doAbort=" << (_doAbort ? "true" : "false")
                  << ", doSkip=" << (_doSkip ? "true" : "false")
                  << ", forkFilling=" << (_forkFilling ? "true" : "false")
                  << ", forkFilled=" << (_forkFilled ? "true" : "false")
                  << std::endl
                ;
        return aux::kNextHandler & fs;
    }
    virtual bool next_message() override {
        return !_doSkip;
    }
    virtual bool is_fork_filled() const {
        return _forkFilled;
    }
    virtual bool is_fork_filling() const {
        return _forkFilling;
    }
    // This is the single method that has to be overriden by particular
    // descendant.
    //virtual ManifoldResT pop_result() = (0, inherited);
public:
    bool do_skip() const { return _doSkip; }
    bool do_abort() const { return _doAbort; }
};

}

template<typename MessageT>
struct HandlerTraits<MessageT, aux::ManifoldRC, iManifoldHandler> {
    typedef MessageT Message;
    typedef aux::ManifoldRC HandlerResult;
    typedef iManifoldHandler<Message, HandlerResult> AbstractHandler;
    typedef AbstractHandler *AbstractHandlerRef;

    //template<typename CallableT> using Handler = PrimitiveHandler< Message
    //                                                             , HandlerResult
    //                                                             , CallableT
    //                                                             , iManifoldHandler >;
    template<typename CallableT>
    class Handler : public PrimitiveHandler<Message
                                           , HandlerResult
                                           , CallableT
                                           , iManifoldHandler> {
    public:
        typedef PrimitiveHandler<Message
                                , HandlerResult
                                , CallableT
                                , iManifoldHandler> Parent;
        typedef typename Parent::CallableRef CallableRef;
    public:
        Handler( CallableRef pRef ) : Parent( pRef ) {}
    };
};

template<>
struct HandlerResultConverter<aux::ManifoldRC, void> {
    virtual aux::ManifoldRC convert( void ) {
        return aux::RC_Continue;
    }
};

template<>
struct HandlerResultConverter<aux::ManifoldRC, bool> {
    virtual aux::ManifoldRC convert( bool v ) {
        return v ? aux::RC_Continue : aux::kNextMessage;
    }
};

/**@brief Assembly of pipelines with elaborated composition management.
 * @class Manifold
 *
 * The Manifold class manages assembly of pipelines, performing fork/junction
 * (de-)multiplexing, changing the event types and automated insertion of
 * auxilliary handlers.
 * */
template< typename MessageT
        , typename ManifoldResultT
        , template<typename T> class TChainT>
class Manifold : public PrimitivePipe< MessageT
                                     , aux::ManifoldRC
                                     , ManifoldResultT
                                     , iManifoldHandler
                                     , TChainT
                                     > {
public:
    typedef PrimitivePipe< MessageT
                         , aux::ManifoldRC
                         , ManifoldResultT
                         , iManifoldHandler
                         , TChainT> Parent;
    typedef typename Parent::TheHandlerTraits   TheHandlerTraits;
    typedef typename Parent::Message            Message;
    typedef typename Parent::ISource            ISource;
    typedef interfaces::ManifoldArbiter<ManifoldResultT> IArbiter;
    typedef typename Parent::Chain              Chain;
    typedef typename TheHandlerTraits::AbstractHandler AbstractHandler;
    // ...

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
    virtual ManifoldResultT process( ISource & src ) override {
        if( ! this->arbiter_ptr() ) {
            pipet_error( Uninitialized, "Arbiter object pointer is not set for "
                    "manifold instance %p while process() was invoked.",
                    this );
        }
        IArbiter & a = static_cast<IArbiter&>(*(this->arbiter_ptr()));
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
            // Begin of loop iterating messages source.
            Message * msg;  // while(!!(msg = cSrc.next()))
            for( msg = cSrc.next(); msg; msg = cSrc.next() ) {
                typename Chain::iterator handlerIt;
                std::cout << "xxx ... ... Operating with message " << msg
                          << " of source " << &cSrc
                          << std::endl;
                // Begin of loop iterating the handlers chain.
                for( handlerIt = procStart
                   ; handlerIt != Chain::end()
                   ; ++handlerIt ) {
                    AbstractHandler & h = **handlerIt;
                    // Process message with current handler and consider result.
                    if( a.consider_handler_result( h.process( *msg ) ) ) {
                        // consider_handler_result() returned true, what means we
                        // can propagate further along the handlers chain.
                        std::cout << "xxx ... ... ... Propagating after handler " << &h
                                  << " (" << handlerIt - Chain::begin()
                                  << ")" << std::endl;
                        if( a.is_fork_filled() ){
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
                                        this, handlerIt - Chain::begin() );
                                // TODO: ^^^ try to get class name
                            }
                            # endif
                            sourcesStack.push(
                                    std::make_pair(h.junction_ptr(), handlerIt+1));
                            std::cout << "xxx ... ... ... Fork handler " << &h
                                      << " (" << (handlerIt - Chain::begin())
                                      << ") has been filled : PUSH" << std::endl;
                            break;  // get to the source-iterating loop
                        }
                        continue;  // next handler
                    }
                    // _V_consider_handler_result() returned false, that means we have
                    // to interrupt the propagation, but if we have filled the
                    // f/j handler, it must be put on top of sources stack.
                    std::cout << "xxx ... ... ... Breaking after handler " << &h
                              << " (" << handlerIt - Chain::begin()
                              << ")." << std::endl;
                    break;
                }  // handler iteration loop
                if( !a.next_message() ) {
                    std::cout << "xxx ... ... Arbiter stop with source " << &cSrc
                              << std::endl;
                    // We have to take next (newly-created) source from internal
                    // queue and proceed with it.
                    break;
                }
            }
            if(!msg) {
                sourcesStack.pop();
                std::cout << "xxx ... Source "
                          << (void *) &cSrc
                          << " on handler " << (void *) &(*procStart) << " ("
                          << procStart - Chain::begin()
                          << ") "
                          << " POP" << std::endl;
            }
        }
        std::cout << "xxx * Done with source " << (void *) &src << std::endl;
        return a.pop_result();
    }
    /// Single message will be processed as a (temporary) messages source
    /// containing only one event.
    virtual ManifoldResultT process( Message & msg ) override {
        auto tSrc = SingularSource(msg);
        auto res = process( tSrc );
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

