# include <vector>

namespace ppt {

template<typename T>
struct Traits {
    struct Routing {
        typedef int ResultCode;
        static constexpr ResultCode noPropFlag = 0x1
                                  , intactFlag = 0x2
                                  ;
        static bool do_interrupt( ResultCode rc ) { return rc & noPropFlag; }
    };
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
class Pipe : public std::vector< iProcessor<T> * > {
public:
    Pipe() {}
    Pipe( iProcessor<T> * p ) {
        this->push_back( p );
    }

    void eval_on_msg( T & m ) {
        typename Traits<T>::Routing::ResultCode rc;
        for( auto it = this->begin(); this->end() != it; ++it ) {
            if( Traits<T>::Routing::do_interrupt( rc = (*it)->eval( m ) ) ) {
                break;
            }
        }
    }
};  // Pipe

template<typename T>
class EvaluationProxy : public Pipe<T>
                      , public std::set<iProcessor<T>*> {
};

// In-place ctr of pipeline for two processors
template<typename T> Pipe<T>
operator|( iProcessor<T> & p1, iProcessor<T> & p2 ) {
    Pipe<T> p(&p1);
    p.push_back(&p2);
    return p;
}

template<typename T> Pipe<T> &
operator|( Pipe<T> & p, iProcessor<T> && proc ) {
    p.push_back( &proc );
    return p;
}

template<typename T, typename RCT> Pipe<T> &
operator|( Pipe<T> & p, RCT(*f)(T &) ) {
    return p | StatelessMutator<T, RCT>(f);
}

template<typename T, typename RCT> Pipe<T> &
operator|( Pipe<T> & p, RCT(*f)(const T &) ) {
    return p | StatelessObserver<T, RCT>(f);
}

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, T && m ) {
    p.eval_on_msg( m );
    return p;
}

}  // namespace ::ppt

