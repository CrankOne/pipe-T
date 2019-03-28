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
};

template<typename T, typename SourceT>
struct SourceTraits {
    static bool has_message( const SourceT & src ) { return src.has_message(); }
    static T & get_message_ref( SourceT & src ) { return src.next_message(); }
};


//
// Processors
////////////

// Processor roles:
//  - Mutator -- changes values in the message
//  - Observer -- monitors values in the message, gathers statistics
//  - Router (fork) -- pushes messages in the [parallel] queue
//  - Junction -- pulls messages from the parallel queue
//  - (un-)packer -- transforms one message type into another

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

template<typename T>
class iProcessor : public AbstractProcessor<typename std::remove_const<T>::type> {
public:
    iProcessor() : AbstractProcessor<typename std::remove_const<T>::type>(
                std::is_const<T>::value ) {}
    virtual typename Traits<T>::Routing::ResultCode eval( T & ) = 0;
    typename Traits<T>::Routing::ResultCode operator()( T & m ) {
        return Traits<T>::Routing::mark_intact( eval(m) );
    }
};  // iProcessor

//
// Read-only processors

template<typename T>
class iObserver : public iProcessor<const T> { };

template<typename T, typename RCT>
class StatelessObserver : public iObserver<T> {
private:
    void (* _f)(const T &);
public:
    StatelessObserver( void (*f)(const T &) ) : _f(f) {}

    virtual typename Traits<T>::Routing::ResultCode eval( const T & m ) override {
        _f( m );
        return Traits<T>::Routing::mark_intact( 0 );
    }
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, void> : public iObserver<T> {
private:
    void (* _f)(const T &);
public:
    StatelessObserver( void (*f)(const T &) ) : _f(f) {}

    virtual typename Traits<T>::Routing::ResultCode eval( const T & m ) override {
        _f( m );
        return Traits<T>::Routing::mark_intact( 0 );
    }
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, typename Traits<T>::Routing::ResultCode> : public iObserver<T> {
private:
    typename Traits<T>::Routing::ResultCode (* _f)(const T &);
public:
    StatelessObserver( typename Traits<T>::Routing::ResultCode (*f)(const T &) ) : _f(f) {}
    virtual typename Traits<T>::Routing::ResultCode eval( const T & m ) override {
        return _f( m );
    }
};  // StatelessObserver


//
// Mutating processors

template<typename T>
class iMutator : public iProcessor<T> {
};  // iMutator

template<typename T, typename RCT>
class StatelessMutator : public iMutator<T> {
private:
    RCT (* _f)( T &);
public:
    StatelessMutator( RCT (*f)(T &) ) : _f(f) {}
    virtual RCT eval( T & m ) override {
        return _f( m );
    }
};  // StatelessMutator

template<typename T>
class StatelessMutator<T, typename Traits<T>::Routing::ResultCode> : public iMutator<T> {
private:
    typename Traits<T>::Routing::ResultCode (* _f)( T &);
public:
    StatelessMutator( typename Traits<T>::Routing::ResultCode (*f)(T &) ) : _f(f) {}
    virtual typename Traits<T>::Routing::ResultCode eval( T & m ) override {
        return _f( m );
    }
};  // StatelessMutator

template<typename T>
class StatelessMutator<T, void> : public iMutator<T> {
private:
    void (* _f)( T &);
public:
    StatelessMutator( void (*f)(T &) ) : _f(f) {}
    virtual typename Traits<T>::Routing::ResultCode eval( T & m ) override {
        _f( m );
        return 0;
    }
};  // StatelessMutator

//
// Pipelines
///////////

// Represents a linear processors chain
template<typename T>
class Pipe : public iProcessor<T>
           , public std::vector<AbstractProcessor<typename std::remove_const<T>::type>*> {
public:
    Pipe() {}
    Pipe( const Pipe & o ) : std::vector<iProcessor<T>*>(o) {}

    virtual typename Traits<T>::Routing::ResultCode
    eval( T & m ) override {
        typename Traits<T>::Routing::ResultCode rc;
        bool modified = false;
        for( auto it = this->begin(); this->end() != it; ++it ) {
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

    # if 0
    template<typename SourceT>
    void process_source( T * s, SourceT src ) {
        typename Traits<T>::Routing::ResultCode rc;
        while(SourceTraits<T, SourceT>::has_message(src)) {
            rc = process_message(SourceTraits<T, SourceT>::get_message_ref(src));
            if( Traits<T>::Routing::do_stop_iteration(rc) ) {
                return rc;
            }
        }
    }
    # endif
};  // Pipe

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, T & m ) {
    p.eval(m);
    return p;
}

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, T && m ) {
    p.eval(m);
    return p;
}

}  // namespace ::ppt

