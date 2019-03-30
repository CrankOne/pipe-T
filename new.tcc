# include <vector>
# include <list>
# include <cassert>

namespace ppt {

struct DefaultRoutingFlags {
    static constexpr int noPropFlag = 0x1
                       , noNextFlag = 0x2
                       , intactFlag = 0x4
                       ;
};

struct DefaultRoutingTraits {
    // General processor exit status (ES) type
    typedef int ResultCode;

    // Must return true if ES shall stop message propagation through the pipe
    static bool do_stop_propagation( ResultCode rc ) {
        return rc & DefaultRoutingFlags::noPropFlag;
    }
    // Must return true if ES have to stop processing the sequence
    static bool do_stop_iteration( ResultCode rc ) {
        return rc & DefaultRoutingFlags::noNextFlag;
    }
    // Must return true if ES indicates changes made with message
    static bool was_modified( ResultCode rc ) {
        return !(rc & DefaultRoutingFlags::intactFlag);
    }

    // Used by some routines to set "intact" mark on return code
    static ResultCode mark_intact( ResultCode rc ) {
        return rc | DefaultRoutingFlags::intactFlag;
    }
};

template<typename T>
struct Traits {
    typedef DefaultRoutingTraits Routing;
    typedef typename std::conditional<std::is_arithmetic<T>::value, T, const T & >::type CRef;
    typedef typename std::conditional<std::is_const<T>::value
                                     , CRef, T &>::type Ref;
    typedef T && RRef;
};

template<typename T, typename SourceT>
struct ExtractionTraits;

//
// Processors
////////////

// Processor roles:
//  - Mutator -- changes values in the message
//  - Observer -- monitors values in the message, gathers statistics
//  - Router (fork) -- pushes messages in the [parallel] queue
//  - Junction -- pulls messages from the parallel queue
//  - (un-)packer -- transforms one message type into another

// Provides basic introspection for downcasting the type: observer/mutator
template<typename T>
class AbstractProcessor {
private:
    const bool _isObserver;
protected:
    AbstractProcessor( bool isObserver ) : _isObserver(isObserver) {}
public:
    virtual ~AbstractProcessor(){}
    // Use it to downcast processor instance to common type
    template<typename PT> PT as() { return dynamic_cast<PT&>(*this); }
    // Returns, whether the processor instance is observer.
    bool is_observer() const { return _isObserver; }
};  // AbstractProcessor

// Generic processor interface
template<typename T>
class iProcessor : public AbstractProcessor<typename std::remove_const<T>::type> {
public:
    iProcessor() : AbstractProcessor<typename std::remove_const<T>::type>(
                std::is_const<T>::value ) {}
    virtual typename Traits<T>::Routing::ResultCode eval( typename Traits<T>::Ref ) = 0;
    typename Traits<T>::Routing::ResultCode operator()( typename Traits<T>::Ref m ) {
        return Traits<T>::Routing::mark_intact( eval(m) );
    }
};  // iProcessor

//
// Read-only processors (observers)

template<typename T>
class iObserver : public iProcessor<const T> { };

template<typename T, typename RCT>
class StatelessObserver : public iObserver<T> {
private:
    void (* _f)(typename Traits<T>::CRef);
public:
    StatelessObserver( void (*f)( typename Traits<T>::CRef ) ) : _f(f) {}

    virtual typename Traits<T>::Routing::ResultCode eval( typename Traits<T>::CRef m ) override {
        _f( m );
        return Traits<T>::Routing::mark_intact( 0 );
    }
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, void> : public iObserver<T> {
private:
    void (* _f)(typename Traits<T>::CRef);
public:
    StatelessObserver( void (*f)(typename Traits<T>::CRef) ) : _f(f) {}

    virtual typename Traits<T>::Routing::ResultCode eval( typename Traits<T>::CRef m ) override {
        _f( m );
        return Traits<T>::Routing::mark_intact( 0 );
    }
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, typename Traits<T>::Routing::ResultCode> : public iObserver<T> {
private:
    typename Traits<T>::Routing::ResultCode (* _f)(typename Traits<T>::CRef);
public:
    StatelessObserver( typename Traits<T>::Routing::ResultCode (*f)(typename Traits<T>::CRef) ) : _f(f) {}
    virtual typename Traits<T>::Routing::ResultCode eval( typename Traits<T>::CRef m ) override {
        return _f( m );
    }
};  // StatelessObserver


//
// Processors affecting the data (mutators)

template<typename T>
class iMutator : public iProcessor<T> {
};  // iMutator

template<typename T, typename RCT>
class StatelessMutator : public iMutator<T> {
private:
    RCT (* _f)(typename Traits<T>::Ref);
public:
    StatelessMutator( RCT (*f)(typename Traits<T>::Ref) ) : _f(f) {}
    virtual RCT eval( typename Traits<T>::Ref m ) override {
        return _f( m );
    }
};  // StatelessMutator

template<typename T>
class StatelessMutator<T, typename Traits<T>::Routing::ResultCode> : public iMutator<T> {
private:
    typename Traits<T>::Routing::ResultCode (* _f)(typename Traits<T>::Ref);
public:
    StatelessMutator( typename Traits<T>::Routing::ResultCode (*f)(typename Traits<T>::Ref) ) : _f(f) {}
    virtual typename Traits<T>::Routing::ResultCode eval(typename Traits<T>::Ref m) override {
        return _f( m );
    }
};  // StatelessMutator

template<typename T>
class StatelessMutator<T, void> : public iMutator<T> {
private:
    void (* _f)(typename Traits<T>::Ref);
public:
    StatelessMutator( void (*f)(typename Traits<T>::Ref) ) : _f(f) {}
    virtual typename Traits<T>::Routing::ResultCode eval(typename Traits<T>::Ref m) override {
        _f( m );
        return 0;
    }
};  // StatelessMutator

//
// Pipelines
///////////

// Represents a linear processors chain
template<typename T>
class Pipe : public std::conditional< std::is_const<T>::value
                                    , iObserver<T>
                                    , iMutator<T> >::type
           , public std::vector<AbstractProcessor<typename std::remove_const<T>::type>*> {
public:
    Pipe() {}
    Pipe( const Pipe & o ) : std::vector<iProcessor<T>*>(o) {}

    virtual typename Traits<T>::Routing::ResultCode
    eval( typename Traits<T>::Ref m ) override { return eval_pipe_on( this, m ); }
};  // Pipe

// Eval pipe with mutators
template<typename T> typename std::enable_if< ! std::is_const<T>::value
                                            , typename Traits<T>::Routing::ResultCode >::type
eval_pipe_on( Pipe<T> * p, typename Traits<T>::Ref m ) {
    typename Traits<T>::Routing::ResultCode rc;
    bool modified = false;
    for( auto it = p->begin(); p->end() != it; ++it ) {
        rc = (*it)->is_observer()
           ? static_cast<iProcessor<const T>*>(*it)->eval(m)
           : static_cast<iProcessor<      T>*>(*it)->eval(m)
           ;
        if(Traits<T>::Routing::do_stop_propagation( rc )) {
            return modified
                 ? rc
                 : Traits<T>::Routing::mark_intact( rc )
                 ;
        }
        if( Traits<T>::Routing::was_modified( rc ) ) {
            assert( !(*it)->is_observer() );
            modified = true;
        }
    }
    return modified
         ? 0
         : Traits<T>::Routing::mark_intact( 0 )
         ;
}

// Eval pipe with observers only
template<typename T> typename std::enable_if< std::is_const<T>::value
                                            , typename Traits<T>::Routing::ResultCode >::type
eval_pipe_on( Pipe<T> * p, typename Traits<T>::Ref m ) {
    typename Traits<T>::Routing::ResultCode rc;
    for( auto it = p->begin(); p->end() != it; ++it ) {
        assert( (*it)->is_observer() );
        rc = static_cast<iProcessor<const T>*>(*it)->eval(m);
        if(Traits<T>::Routing::do_stop_propagation( rc )) {
            return Traits<T>::Routing::mark_intact( rc );
        }
        assert( !Traits<T>::Routing::was_modified( rc ) );
    }
    return Traits<T>::Routing::mark_intact( 0 );
}

template< typename T
        , typename SourceT> typename Traits<SourceT>::Routing::ResultCode
eval_on_source( Pipe<T> * p, SourceT src ) {
    bool modified = false;
    typename Traits<T>::Routing::ResultCode rc;
    for( auto it = ExtractionTraits<T, SourceT>::begin(src)
       ; it != ExtractionTraits<T, SourceT>::end(src); ++it ) {
        rc = process_message(ExtractionTraits<T, SourceT>::get_message_ref(it));
        if( Traits<T>::Routing::was_modified(rc) ) {
            modified = true;
        }
        if( Traits<T>::Routing::do_stop_iteration(rc) ) {
            auto srcRc = ExtractionTraits<T, SourceT>::translate_results(rc);
            return modified ? srcRc
                            : Traits<SourceT>::mark_intact(srcRc)
                            ;
        }
    }
    auto srcRc = ExtractionTraits<T, SourceT>::translate_results(0);
    return modified ? srcRc
                    : Traits<SourceT>::mark_intact(srcRc)
                    ;
}

//
// (Un-)packing sub-pipeline: transforms one message type (OutT to another
// (InT), runs the pipeline and, if changes were made, packs it back.
template< typename OutT  //< outern (encompassing) type
        , typename InT  //< intern (target) type
        >
class Span : public iProcessor<OutT>
           , public Pipe<InT> {
public:
    virtual typename Traits<OutT>::Routing::ResultCode
    eval( typename Traits<OutT>::Ref m ) override {
        return eval_on_source<InT, OutT>( *this, m );
    }
};

//
// Synctactic sugar
//////////////////

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, T & m ) {
    p(m);
    return p;
}

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, T && m ) {
    p(m);
    return p;
}

}  // namespace ::ppt

