# include <vector>
# include <list>
# include <cassert>
# include <thread>
# include <condition_variable>
# include <iomanip>
# include <string>

# ifndef PPT_DISABLE_JOUNRALING
#   include "rapidxml-1.13/rapidxml.hpp"
    // NOTE: customization to make rapidxml work with modern compilers. See:
    // https://stackoverflow.com/questions/14113923/rapidxml-print-header-has-undefined-methods
#   include "rapidxml-1.13/rapidxml_ext.hpp"
# endif

namespace ppt {

// fwd
# ifndef PPT_DISABLE_JOUNRALING
namespace journaling { template<typename T> class Journal; }
# endif
template<typename T> class AbstractProcessor;
template<typename T> class Pipe;

/**@brief Constants for common execution status report.
 *
 * This is a bit flags, reflecting processing result code. Shall satisfy most
 * of the practical cases.
 * */
struct DefaultRoutingFlags {
    static constexpr int noPropFlag = 0x1
                       , noNextFlag = 0x2
                       , intactFlag = 0x4
                       ;
};

/**@brief Common execution status report.
 *
 * Default routing traits shall satisfy most of the practical cases.
 * */
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

/**@brief General traits defining pipeline.
 *
 * This structure holds most of the types relevant for particular message type.
 * */
template<typename T>
struct Traits {
    typedef DefaultRoutingTraits Routing;
    typedef typename std::conditional<std::is_arithmetic<T>::value, T, const T & >::type CRef;
    typedef T &  Ref;
    typedef T && RRef;

    typedef unsigned long MessageID;
    static MessageID message_id( CRef m ) { return (MessageID) (&m); }
};

// const T and non-const T traits are equivalent
template<typename T> struct Traits<const T> : public Traits<T> {};

# ifndef PPT_DISABLE_JOUNRALING
//
// Journaling
////////////

namespace journaling {

template<typename T> struct Traits;

template<typename T>
struct RapidXMLTraits {
    typedef ::ppt::journaling::Journal<T> Journal;
    typedef std::pair< rapidxml::xml_document<> *
                     , rapidxml::xml_node<> * > NodeRef;
    /// Adds named sub-node of certain type within the given node.
    template<typename FT>
    static void add_field( NodeRef nr
                         , const char * name
                         , const char * value ) {
        rapidxml::xml_node<> * n = nr.first->allocate_node( rapidxml::node_element
                                               , name
                                               , nr.first->allocate_string(value) );
        // NOTE: type are mangled in GCC
        //rapidxml::xml_attribute<> * typeAttr =
        //    nr.first->allocate_attribute("type", typeid(T).name() );
        //n->append_attribute( typeAttr );
        nr.second->append_node(n);
    }
    /// Adds named list sub-node within the given node.
    static rapidxml::xml_node<> * add_list( NodeRef nr, const char * name ) {
        auto nn = nr.first->allocate_node( rapidxml::node_element, name );
        nr.second->append_node(nn);
        return nn;
    }
    /// Shall return new list node, ready for writing. Name has to be related
    /// to type (node type name).
    static NodeRef new_list_node( rapidxml::xml_node<> * ln
                                , NodeRef nr
                                , const char * name ) {
        rapidxml::xml_node<> * n =
            nr.first->allocate_node( rapidxml::node_element, name );
        ln->append_node( n );
        return NodeRef(nr.first, n);
    }
    static void print_info( std::ostream & os, AbstractProcessor<T> & p ) {
        rapidxml::xml_document<> doc;
        rapidxml::xml_node<> * rootNode = doc.allocate_node( rapidxml::node_element, "processor" );
        p.info( typename journaling::Traits<T>::NodeRef(&doc, rootNode) );
        doc.append_node(rootNode);
        rapidxml::print(os, doc, 0);
    }
};

/// Set RapidXML to be default codec.
template<typename T> struct Traits : public RapidXMLTraits<T> {};
template<typename T> struct Traits<const T> : public Traits<T> {};

/// Journaling entry type code.
enum EntryType {
    unspecified = 0,
    procBgn, procEnd,
    // ...
};

/// Journaling entry type parameterised by message ID.
template<typename MsgIDT>
struct Entry {
    std::clock_t time;
    void * issuer;
    EntryType type;
    MsgIDT msgID;
    // ...
};

/// A container for pipeline journal.
template<typename T>
class Journal : public std::vector< Entry<typename ppt::Traits<T>::MessageID> > {
public:
    typedef Entry<typename ppt::Traits<T>::MessageID> ThisEntry;
private:
    /// Journal blocking mutex -- prevents from simulaneous access to entries
    /// container.
    std::mutex _mtx;
public:
    /// Creates new journal entry with 0 message ID.
    void new_entry( EntryType et, void * p ) {
        std::unique_lock<std::mutex> lock(_mtx);
        std::vector<ThisEntry>::push_back( ThisEntry{ std::clock(), p, et, 0 } );
    }
    /// Creates new journal entry tagged with given message ID.
    void new_entry( EntryType et, void * p, typename ppt::Traits<T>::MessageID mid ) {
        std::unique_lock<std::mutex> lock(_mtx);
        std::vector<ThisEntry>::push_back( ThisEntry{ std::clock(), p, et, mid } );
    }

    /// Writes the events journal.
    void dump( typename journaling::Traits<T>::NodeRef nr ) {
        char bf[32];
        for( auto & e : *this ) {
            auto lnr = journaling::Traits<T>::new_list_node( nr.second, nr, "event" );
            snprintf( bf, sizeof(bf), "%#lx", (long unsigned int) e.time );
            journaling::Traits<T>::template add_field<clock_t>( lnr, "time",   bf );
            snprintf( bf, sizeof(bf), "%p", e.issuer );
            journaling::Traits<T>::template add_field<void *>(  lnr, "issuer", bf );
            snprintf( bf, sizeof(bf), "%x", e.type );
            journaling::Traits<T>::template add_field<int>(     lnr, "type",   bf );
            if( e.msgID ) {
                snprintf( bf, sizeof(bf), "%#lx", (long unsigned int) e.msgID );
                journaling::Traits<T>::template add_field<int>( lnr, "msgID", bf );
            }
        }
    }

    /// Prints the journal according to journaling traits.
    void print( std::ostream & os ) {
        rapidxml::xml_document<> doc;
        rapidxml::xml_node<> * listNode
            = doc.allocate_node( rapidxml::node_element, "processingHistory" );
        dump( typename journaling::Traits<T>::NodeRef(&doc, listNode) );
        doc.append_node(listNode);
        rapidxml::print(os, doc, 0);
    }

    /// Performs a plain ASCII print of the journal.
    void print_plain_ascii( std::ostream & os ) {
        for( auto & e : *this ) {
            os << e.time
               << ":" << std::hex << std::setw(16) << e.issuer << std::dec
               << " " << e.type
               << " " << e.msgID
               << std::endl
               ;
        }
    }
};

}  // namespace journaling
# define JOURNAL_ENTRY( tp, procPtr ) if(this->has_journal()) this->journal().new_entry(::ppt::journaling::tp, procPtr);
# else
# define JOURNAL_ENTRY( tp, procPtr ) /* journaling disabled */
# endif

template<typename T, typename SourceT> struct ExtractionTraits;
# if 0
{
    // Performs the loop-like processing of the given source with given
    // pipeline.
    static typename Traits<SourceT>::Routing::ResultCode process( SourceT & src, Pipe<T> & p );
    // Might be defined to pack back the modified messages.
    //pack()  ... TODO
};
# endif

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
template< typename T >
class AbstractProcessor {
private:
    const bool _isObserver;
    bool _isVacant;  ///< Used in addition with CV to prevent SWU
    std::mutex _busyMtx;
    std::condition_variable _busyCV;
    # ifndef PPT_DISABLE_JOUNRALING
    typename journaling::Traits<T>::Journal * _jPtr;
    # endif
protected:
    void _set_vacant(bool v) { _isVacant = v; }
    AbstractProcessor( bool isObserver ) : _isObserver(isObserver)
                                         , _isVacant(true)
                                         , _jPtr(nullptr) {}
    AbstractProcessor( const AbstractProcessor<T> & o ) : _isObserver(o._isObserver)
                                                        , _isVacant(true)
                                                        , _busyCV()
                                                        , _jPtr(nullptr) {}
public:
    virtual ~AbstractProcessor(){}
    /// Use it to downcast processor instance to common type
    template<typename PT> PT as() { return dynamic_cast<PT&>(*this); }
    /// Returns, whether the processor instance is observer.
    bool is_observer() const { return _isObserver; }
    /// Returns true if processor instance does not perform evaluation
    /// currently
    bool is_vacant() const { return _isVacant; }
    /// Returns "busy" condition variable bound to processor instance
    std::condition_variable & busy_CV() { return _busyCV; }
    /// Returns mutex guarding "busy" condition variable
    std::mutex & busy_mutex() { return _busyMtx; }

    # ifndef PPT_DISABLE_JOUNRALING
    /// Returns true if there is a journal associated with processor instance
    bool has_journal() const { return _jPtr; }
    /// Returns reference to journal instance associated. Must be guarded with
    /// `has_journal()' check, unless null pointer dereferencing is possible.
    typename journaling::Traits<T>::Journal & journal() { return *_jPtr; }
    /// Sets journal instance.
    virtual void assign_journal( typename journaling::Traits<T>::Journal & j ) { _jPtr = &j; }
    /// Appends given document with fields specific for this processor
    /// instance.
    virtual void info( typename journaling::Traits<T>::NodeRef d ) const {
        char bf[32];
        snprintf( bf, sizeof(bf), "%p", (void *) this );
        journaling::Traits<T>::template add_field<void *>( d
                , "address", bf );
        journaling::Traits<T>::template add_field<bool>( d
                , "isObesrver", is_observer() ? "true" : "false" );
    }
    # endif
};  // AbstractProcessor

// Generic processor interface
template<typename T>
class iProcessor : public AbstractProcessor<typename std::remove_const<T>::type> {
public:
    typedef typename Traits<T>::Ref RefType;
protected:
    virtual typename Traits<T>::Routing::ResultCode _V_eval( RefType ) = 0;
public:
    iProcessor() : AbstractProcessor<typename std::remove_const<T>::type>(
                std::is_const<T>::value ) {}
    virtual typename Traits<T>::Routing::ResultCode eval( RefType m ) {
        assert( this->is_vacant() );
        std::unique_lock<std::mutex> lock( this->busy_mutex() );
        this->_set_vacant(false);
        JOURNAL_ENTRY( procBgn, this )
        auto rc = _V_eval( m );
        JOURNAL_ENTRY( procEnd, this )
        this->_set_vacant(true);
        return rc;
    }
    typename Traits<T>::Routing::ResultCode operator()( RefType m ) {
        return eval(m);
    }
};  // iProcessor

template<typename T>
class iProcessor<const T> : public AbstractProcessor<typename std::remove_const<T>::type> {
public:
    typedef typename Traits<T>::CRef RefType;
protected:
    virtual typename Traits<T>::Routing::ResultCode _V_eval( RefType ) = 0;
public:
    iProcessor() : AbstractProcessor<typename std::remove_const<T>::type>( true ) {}
    virtual typename Traits<T>::Routing::ResultCode eval( RefType m ) {
        assert( this->is_vacant() );
        std::unique_lock<std::mutex> lock( this->busy_mutex() );
        this->_set_vacant(false);
        JOURNAL_ENTRY( procBgn, this )
        auto rc = _V_eval( m );
        JOURNAL_ENTRY( procEnd, this )
        this->_set_vacant(true);
        return rc;
    }
    typename Traits<T>::Routing::ResultCode operator()( RefType m ) {
        return eval(m);
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
protected:
    virtual typename Traits<T>::Routing::ResultCode _V_eval( typename Traits<T>::CRef m ) override {
        _f( m );
        return Traits<T>::Routing::mark_intact( 0 );
    }
public:
    StatelessObserver( void (*f)( typename Traits<T>::CRef ) ) : _f(f) {}

    virtual void info( typename journaling::Traits<T>::NodeRef d ) const override {
        iObserver<T>::info(d);
        journaling::Traits<T>::template add_field<bool>( d, "isStateless", "true" );
    }
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, void> : public iObserver<T> {
private:
    void (* _f)(typename Traits<T>::CRef);
protected:
    virtual typename Traits<T>::Routing::ResultCode _V_eval( typename Traits<T>::CRef m ) override {
        _f( m );
        return Traits<T>::Routing::mark_intact( 0 );
    }
public:
    StatelessObserver( void (*f)(typename Traits<T>::CRef) ) : _f(f) {}

    # ifndef PPT_DISABLE_JOUNRALING
    virtual void info( typename journaling::Traits<T>::NodeRef d ) const override {
        iObserver<T>::info(d);
        journaling::Traits<T>::template add_field<bool>( d, "isStateless", "true" );
        journaling::Traits<T>::template add_field<bool>( d, "noResultCode", "true" );
    }
    # endif
};  // StatelessObserver

template<typename T>
class StatelessObserver<T, typename Traits<T>::Routing::ResultCode> : public iObserver<T> {
private:
    typename Traits<T>::Routing::ResultCode (* _f)(typename Traits<T>::CRef);
protected:
    virtual typename Traits<T>::Routing::ResultCode _V_eval( typename Traits<T>::CRef m ) override {
        return _f( m );
    }
public:
    StatelessObserver( typename Traits<T>::Routing::ResultCode (*f)(typename Traits<T>::CRef) ) : _f(f) {}

    # ifndef PPT_DISABLE_JOUNRALING
    virtual void info( typename journaling::Traits<T>::NodeRef d ) const override {
        iObserver<T>::info(d);
        journaling::Traits<T>::template add_field<bool>( d, "isStateless", "true" );
    }
    # endif
};  // StatelessObserver


//
// Processors affecting the data (mutators)

template<typename T>
class iMutator : public iProcessor<T> { };  // iMutator

template<typename T, typename RCT>
class StatelessMutator : public iMutator<T> {
private:
    RCT (* _f)(typename Traits<T>::Ref);
protected:
    virtual RCT _V_eval( typename Traits<T>::Ref m ) override {
        return _f( m );
    }
public:
    StatelessMutator( RCT (*f)(typename Traits<T>::Ref) ) : _f(f) {}

    # ifndef PPT_DISABLE_JOUNRALING
    virtual void info( typename journaling::Traits<T>::NodeRef d ) const override {
        iObserver<T>::info(d);
        journaling::Traits<T>::template add_field<bool>( d, "isStateless", "true" );
    }
    # endif
};  // StatelessMutator

template<typename T>
class StatelessMutator<T, typename Traits<T>::Routing::ResultCode> : public iMutator<T> {
private:
    typename Traits<T>::Routing::ResultCode (* _f)(typename Traits<T>::Ref);
protected:
    virtual typename Traits<T>::Routing::ResultCode _V_eval(typename Traits<T>::Ref m) override {
        return _f( m );
    }
public:
    StatelessMutator( typename Traits<T>::Routing::ResultCode (*f)(typename Traits<T>::Ref) ) : _f(f) {}
    
    # ifndef PPT_DISABLE_JOUNRALING
    virtual void info( typename journaling::Traits<T>::NodeRef d ) const override {
        iMutator<T>::info( d );
        journaling::Traits<T>::template add_field<bool>( d, "isStateless", "true" );
    }
    # endif
};  // StatelessMutator

template<typename T>
class StatelessMutator<T, void> : public iMutator<T> {
private:
    void (* _f)(typename Traits<T>::Ref);
protected:
    virtual typename Traits<T>::Routing::ResultCode _V_eval(typename Traits<T>::Ref m) override {
        _f( m );
        return 0;
    }
public:
    StatelessMutator( void (*f)(typename Traits<T>::Ref) ) : _f(f) {}

    # ifndef PPT_DISABLE_JOUNRALING
    virtual void info( typename journaling::Traits<T>::NodeRef d ) const override {
        iMutator<T>::info(d);
        journaling::Traits<T>::template add_field<bool>( d, "isStateless", "true" );
        journaling::Traits<T>::template add_field<bool>( d, "noResultCode", "true" );
    }
    # endif
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
    typedef typename iProcessor<T>::RefType RefType;
    typedef typename std::conditional< std::is_const<T>::value
                                    , iObserver<T>
                                    , iMutator<T> >::type Parent;
protected:
    typename Traits<T>::Routing::ResultCode _rc;

    virtual typename Traits<T>::Routing::ResultCode
    _V_eval( RefType m ) override {
        return _eval_pipe_on( this, m, _rc ); }
public:
    Pipe() {}
    Pipe( const Pipe & o ) : std::vector<AbstractProcessor<typename std::remove_const<T>::type>*>(o) {}
    typename Traits<T>::Routing::ResultCode lates_result_code() const {
        return _rc; }

    virtual void assign_journal( typename journaling::Traits<T>::Journal & j ) override {
        AbstractProcessor<typename std::remove_const<T>::type>::assign_journal(j);
        for( auto it = this->begin(); this->end() != it; ++it ) {
            (*it)->assign_journal(j);
        }
    }

    # ifndef PPT_DISABLE_JOUNRALING
    virtual void info( typename journaling::Traits<T>::NodeRef d ) const override {
        Parent::info(d);
        auto procList = journaling::Traits<T>::add_list( d, "pipeline" );
        for( auto it = this->begin(); this->end() != it; ++it ) {
            (*it)->info( journaling::Traits<T>::new_list_node( procList, d, "processor" ) );
        }
    }
    # endif
};  // Pipe

// Eval pipe with mutators
template<typename T> typename std::enable_if< ! std::is_const<T>::value
                                            , typename Traits<T>::Routing::ResultCode >::type
_eval_pipe_on( Pipe<T> * p
             , typename Traits<T>::Ref m
             , typename Traits<T>::Routing::ResultCode & rc ) {
    bool modified = false;
    for( auto it = p->begin(); p->end() != it; ++it ) {
        {
            // whait processor becomes available
            std::unique_lock<std::mutex> lock((*it)->busy_mutex());
            while( !(*it)->is_vacant() ) {
                (*it)->busy_CV().wait( lock );
            }
        }
        // eval
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
_eval_pipe_on( Pipe<T> * p
             , typename Traits<T>::CRef m
             , typename Traits<T>::Routing::ResultCode & rc ) {
    for( auto it = p->begin(); p->end() != it; ++it ) {
        assert( (*it)->is_observer() );
        rc = static_cast<iProcessor<const T>*>(*it)->eval(m);
        if(Traits<T>::Routing::do_stop_propagation( rc )) {
            return Traits<T>::Routing::mark_intact( rc );
        }
        assert( ! Traits<T>::Routing::was_modified( rc ) );  // transformations forbidden
    }
    return Traits<T>::Routing::mark_intact( 0 );
}

# if 0
template< typename T
        , typename SourceT> typename Traits<SourceT>::Routing::ResultCode
eval_on_source( Pipe<T> * p, SourceT src ) {
    bool modified = false;
    typename Traits<T>::Routing::ResultCode rc;
    for( auto it = ExtractionTraits<T, SourceT>::begin(src)
       ; it != ExtractionTraits<T, SourceT>::end(src)
       ; ++it ) {
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
# endif

//
// Repacking pipeline: transforms one message type (OutT to another
// (InT), runs the pipeline and, if changes were made, packs it back, the InT
// to OutT. Typical use case is the de-compression.
template< typename OutT  //< outern (encompassing) type
        , typename InT  //< intern (target) type
        >
class Span : public iMutator<OutT>
           , public Pipe<InT> {
public:
    // Helper temporary process, intrinsicly added to the end of pipeline copy
    // made
    class Recorder : public iProcessor<InT> {
    private:
        Pipe<InT> * _p;
        typename Traits<OutT>::Ref _container;
    protected:
        typename Traits<InT>::Routing::ResultCode _V_eval( typename Traits<InT>::Ref m ) override {
            if( Traits<InT>::Routing::was_modified( _p->lates_result_code() ) ) {
                return ExtractionTraits<InT, OutT>::pack( _container, m );
            }
            return Traits<InT>::Routing::mark_intact(0x0);
        }
    public:
        Recorder( Pipe<InT> * p
                , typename Traits<OutT>::Ref container ) : _p(p), _container(container) {}
    };
protected:
    virtual typename Traits<OutT>::Routing::ResultCode
    _V_eval( typename Traits<OutT>::Ref m ) override {
        Recorder r(this, m);
        // Make a temporary copy of the pipe with recorder processor appended.
        Pipe<InT> pCopy(*this);
        pCopy.push_back( &r );
        return ExtractionTraits<InT, OutT>::process( m, pCopy );
    }
public:
    Span( const Pipe<InT> & p ) : Pipe<InT>(p) {}
};

template< typename OutT
        , typename InT
        >
class Span<const OutT, InT> : public iObserver<OutT>
                            , public Pipe<InT> {
    // This assert shall check the case when outern type is supposed to be
    // const, while intern type is supposed to be mutable
    static_assert( std::is_const<InT>::value
                 , "Spanning observer with mutable internal part." );
protected:
    virtual typename Traits<const OutT>::Routing::ResultCode
    _V_eval( typename Traits<const OutT>::CRef m ) override {
        return ExtractionTraits<InT, const OutT>::process( m, *this );
    }
public:
    Span( const Pipe<InT> & p ) : Pipe<InT>(p) {}
};

//
// Synctactic sugar
//////////////////

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, typename Traits<T>::Ref m ) {
    p(m);
    return p;
}

template<typename T> Pipe<T> &
operator<<( Pipe<T> & p, typename Traits<T>::RRef m ) {
    p(m);
    return p;
}

}  // namespace ::ppt

