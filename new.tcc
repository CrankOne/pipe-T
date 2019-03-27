# include <vector>
# include <list>

namespace ppt {

template<typename T>
struct Traits {
    struct Routing {
        typedef int ResultCode;
        static constexpr ResultCode noPropFlag = 0x1
                                  , noNextFlag = 0x2
                                  , intactFlag = 0x4
                                  ;
        static bool do_stop_propagation( ResultCode rc ) { return rc & noPropFlag; }
        static bool do_stop_iteration( ResultCode rc ) { return rc & noNextFlag; }
    };
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
class iProcessor {
public:
    virtual ~iProcessor(){}
    virtual typename Traits<T>::Routing::ResultCode eval( T & ) = 0;
    typename Traits<T>::Routing::ResultCode operator()( T & m ) { return eval(m); }
    template<typename PT> PT as() { return dynamic_cast<PT&>(*this); }
};  // iProcessor


//
// Read-only processors

template<typename T>
class iObserver : public iProcessor<T> {
public:
    virtual typename Traits<T>::Routing::ResultCode observe( const T & m ) = 0;
    virtual typename Traits<T>::Routing::ResultCode eval( T & m ) override {
        return observe( m ) | Traits<T>::Routing::intactFlag;
    }
};  // iObserver

template<typename T, typename RCT>
class StatelessObserver : public iObserver<T> {
private:
    void (* _f)(const T &);
public:
    StatelessObserver( void (*f)(const T &) ) : _f(f) {}

    virtual typename Traits<T>::Routing::ResultCode observe( const T & m ) override {
        _f( m );
        return 0;
    }
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, void> : public iObserver<T> {
private:
    void (* _f)(const T &);
public:
    StatelessObserver( void (*f)(const T &) ) : _f(f) {}

    virtual typename Traits<T>::Routing::ResultCode observe( const T & m ) override {
        _f( m );
        return 0;
    }
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, typename Traits<T>::Routing::ResultCode> : public iObserver<T> {
private:
    typename Traits<T>::Routing::ResultCode (* _f)(const T &);
public:
    StatelessObserver( typename Traits<T>::Routing::ResultCode (*f)(const T &) ) : _f(f) {}
    virtual typename Traits<T>::Routing::ResultCode observe( const T & m ) override {
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

template<typename T>
class Pipe : public std::vector<iProcessor<T>*> {
public:
    Pipe() {}
    Pipe( const Pipe & o ) : std::vector<iProcessor<T>*>(o) {}

    typename Traits<T>::Routing::ResultCode
    process_message( T & m ) {
        typename Traits<T>::Routing::ResultCode rc;
        for( auto it = this->begin(); this->end() != it; ++it ) {
            if( Traits<T>::Routing::do_stop_propagation( rc = (*it)->eval( m ) ) ) {
                return rc;
            }
        }
    }

    template<typename SourceT>
    void process_source( T * s, SourceT src ) {
        typename Traits<T>::Routing::ResultCode rc;
        while(SourceTraits<T, SourceT>::has_message(src)) {
            if( Traits<T>::Routing::do_stop_iteration( rc = process_message(
                            SourceTraits<T, SourceT>::get_message_ref(src) ) )
              ) {
                return rc;
            }
        }
    }
};  // Pipe

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, T & m ) {
    p.process_message(m);
    return p;
}

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, T && m ) {
    p.process_message(m);
    return p;
}

}  // namespace ::ppt

