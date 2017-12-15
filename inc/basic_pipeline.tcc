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
# include <functional>

namespace pipet {

namespace interfaces {

/// Base source interface that has to be implemented.
template<typename MessageT>
struct Source {
    virtual MessageT * next() = 0;
};

template< typename HandlerResultT
        , typename PipelineResultT>
struct Arbiter {
    /// Shall return true if result returned by current handler allows
    /// further propagation.
    virtual bool consider_handler_result( HandlerResultT ) = 0;
    /// Shall return result considering current state and reset state.
    virtual PipelineResultT pop_result() = 0;
    /// Whether to retrieve next message from source and start
    /// event propagation.
    virtual bool next_message() = 0;
};

}  // namespace interfaces

template<typename PipelineTraitsT> class BasicPipeline;

namespace aux {

template<typename T>
using STLAllocatedVector = std::vector<T>;

// This template is splitted into two separate specializations in order to avoid
// mentioning of std::result_of<CallableT(Message &)>::type for function type.
// Even in std::conditional<> compilers will try to defer something and will get
// stuck with "function returning a function".
template< typename CallableT
        , typename MessageT
        , bool isFunction=std::is_function<CallableT>::value
        > struct CallableTraits;

template< typename CallableT
        , typename MessageT
        > struct CallableTraits<CallableT, MessageT, true> {
    typedef CallableT Callable;
    typedef MessageT Message;
    constexpr static bool isFunction = true;
    typedef typename std::function<CallableT>::result_type CallableResult;
    typedef CallableT & CallableRef;
};  // CallableTraits (functions)

template< typename CallableT
        , typename MessageT
        > struct CallableTraits<CallableT, MessageT, false> {
    typedef CallableT Callable;
    typedef MessageT Message;
    constexpr static bool isFunction = false;
    typedef typename std::result_of<CallableT(Message &)>::type CallableResult;
    typedef CallableT & CallableRef;
};  // CallableTraits (classes)

}  // namespace aux

/// The most basic pipeline handler class. Is an abstract base for linear
/// pipeline.
template< typename MessageT
        , typename ProcessingResultT>
class iBasicHandler {
public:
    typedef MessageT Message;
    typedef ProcessingResultT Result;
public:
    /// This constructor does nothing, but is required for extensibility.
    template<typename T> iBasicHandler( T & ) {}
    virtual ~iBasicHandler() {}
    virtual Result process( Message & ) = 0;

    template<typename CallableT> typename aux::CallableTraits< CallableT
                                                             , Message
                                                             >::CallableRef
    processor();
    // ^^^ impl. needs fwd decl. of PrimitiveHandler
};  // class iBasicPipelineHandler

template< typename MessageT
        , typename ResultT
        , typename CallableT
        , template<typename, typename> class ParentTClass=iBasicHandler
        > class PrimitiveHandler;

//
template< typename MessageT
        , typename ProcessingResultT >
template< typename CallableT > typename aux::CallableTraits< CallableT
                                                           , MessageT
                                                           >::CallableRef
iBasicHandler<MessageT, ProcessingResultT>::processor() {
    typedef PrimitiveHandler< Message
            , Result
            , CallableT
            > TargetHandlerType;
    auto hPtr = dynamic_cast<TargetHandlerType *>(this);
    if( !hPtr ) {
        pipet_error( BadCast, "Handler type cast mismatch. Requesting "
                "processor handle of type %s while real handler is of type %s."
            , typeid(TargetHandlerType).name()
            , typeid(hPtr).name());
    }
    return hPtr->processor();
};

template< typename DesiredT,
          typename RealT >
struct HandlerResultConverter;  // default has to be NOT implemented!

template< typename T >
struct HandlerResultConverter<T, T> {
    T convert( const T & v ) { return v; }
};

template< typename MessageT
        , typename ResultT
        , typename CallableT
        , template<typename, typename> class ParentTClass
        >
class PrimitiveHandler : public ParentTClass<MessageT, ResultT>
                       , public HandlerResultConverter< ResultT
                                                      , typename aux::CallableTraits<CallableT, MessageT>::CallableResult
                                                      > {
public:
    typedef aux::CallableTraits<CallableT, MessageT>    TheCallableTraits;
    typedef typename TheCallableTraits::CallableRef     CallableRef;
    typedef ResultT                                     Result;
    typedef HandlerResultConverter< ResultT
                                  , typename TheCallableTraits::CallableResult
                                  > ResultConverter;
    typedef ParentTClass<MessageT, ResultT>             Parent;
private:
    CallableRef _cRef;
public:
    PrimitiveHandler( CallableRef cRef ) : Parent(cRef), _cRef( cRef ) {}
    virtual Result process( MessageT & m ) { return ResultConverter::convert(_cRef( m )); }
    CallableRef processor() { return _cRef; }
    const CallableRef processor() const { return _cRef; }
};


template< typename MessageT
        , typename HandlerResultT
        , template< typename
                  , typename > class AbstractHandlerTemplate >
struct HandlerTraits;

template< typename MessageT
        , typename HandlerResultT>
struct HandlerTraits< MessageT
                    , HandlerResultT
                    , iBasicHandler> {
    typedef MessageT Message;
    typedef HandlerResultT HandlerResult;
    typedef iBasicHandler<Message, HandlerResult>   AbstractHandler;
    typedef AbstractHandler *                       AbstractHandlerRef;
    template<typename CallableT> using Handler = PrimitiveHandler<Message, HandlerResult, CallableT>;
};

/**@brief Strightforward pipeline template primitive.
 * @class BasicPipeline
 *
 * This is the basic implementation of pipeline that performs sequential
 * invocation of processing atoms stored at ordered container, guided by
 * arbitering class.
 * */
template< typename MessageT
        , typename ResultT
        , typename PipelineResultT
        , template< typename
                  , typename > class AbstractHandlerT
        , template<typename T> class TChainT=aux::STLAllocatedVector >
class PrimitivePipe : public TChainT<typename HandlerTraits< MessageT
                                                           , ResultT
                                                           , AbstractHandlerT>::AbstractHandlerRef> {
public:
    typedef MessageT                                    Message;
    typedef ResultT                                     Result;
    typedef PipelineResultT                             PipelineResult;
    typedef HandlerTraits< Message
                         , Result
                         , AbstractHandlerT >           TheHandlerTraits;
    typedef typename TheHandlerTraits::AbstractHandler  AbstractHandler;
    typedef typename TheHandlerTraits::AbstractHandlerRef AbstractHandlerRef;
    typedef TChainT<AbstractHandlerRef>                 Chain;

    typedef interfaces::Source<Message> ISource;
    typedef interfaces::Arbiter<Result, PipelineResult> IArbiter;
private:
    IArbiter * _a;
protected:
    IArbiter * arbiter_ptr() { return _a; }
    const IArbiter * arbiter_ptr() const { return _a; }
public:
    /// Ctr. Requires an arbiter instance to act.
    PrimitivePipe( IArbiter * a ) : _a(a) {}
    /// Virtual dtr (trivial).
    virtual ~PrimitivePipe() {
        for( auto & ahPtr : *static_cast<Chain *>(this) ) {
            delete ahPtr;
        }
    }

    /// Run pipeline evaluation on source.
    virtual PipelineResult process( ISource & src ) {
        if( ! arbiter_ptr() ) {
            pipet_error( Uninitialized, "Arbiter object pointer is not set for "
                    "pipeline instance %p while process() was invoked.",
                    this );
        }
        IArbiter & a = *arbiter_ptr();
        Message * msg;
        while(!!(msg = src.next())) {
            for( AbstractHandler * h : *static_cast<Chain*>(this) ) {
                if( ! a.consider_handler_result( h->process(*msg) ) ) {
                    break;
                }
            }
            if( ! a.next_message() ) {
                break;
            }
        }
        return a.pop_result();
    }

    /// Run pipeline evaluation on single message.
    virtual PipelineResult process( Message & msg ) {
        if( ! arbiter_ptr() ) {
            pipet_error( Uninitialized, "Arbiter object pointer is not set for "
                    "pipeline instance %p while process() was invoked.",
                    this );
        }
        IArbiter & a = *arbiter_ptr();
        for( AbstractHandler * h : *static_cast<Chain*>(this) ) {
            if( ! a.consider_handler_result( h->process(msg) ) ) {
                break;
            }
        }
        return a.pop_result();
    }
    /// Shortcut for inserting processor at the back of pipeline.
    template<typename CallableArgT>
    void push_back( CallableArgT && p ) {
        typedef typename std::remove_reference<CallableArgT>::type CallableType;
        Chain::push_back(
                new typename TheHandlerTraits::template Handler<CallableType>(p) );
    }
    /// Call operator for single message --- to use pipeline as a processor.
    PipelineResult operator()( Message & msg ) {
        return process( msg );
    }
};  // class BasicPipeline

}  // namespace pipet

# endif  // H_PIPE_T_BASIC_PIPELINE_H


