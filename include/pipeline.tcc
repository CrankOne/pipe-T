# ifndef H_PIPE_T_PIPELINE_H
# define H_PIPE_T_PIPELINE_H

# include <list>
# include <stdexcept>

/** Assumptions
 *  - The pipeline traits are uniquely defined by:
 *      - message type
 *      - pipeline processing result type
 *  - The pipeline traits itself contains:
 *      - processor interface base type
 *      - arbitering entity expressed as a type with methods:
 *          - consider_processor_result()
 *            receiving last processor result for consideration
 *          - is_initialized()
 *            deciding, whether the pipeline have to be initialized
 *          - is_done()
 *            deciding, whether the pipeline have to be finalized
 *          - do_eval_next_processor()
 *            deciding, whether the processor iteration shall be continued
 *          - do_eval_next_message()
 *            deciding, whether the next message have to be fetched for treatment
 * */

namespace ppt {

/// Default is undefined.
template <typename SteeringCodeT> class Arbiter;

# define for_each_processor_flag(m)                                             \
    m( Intact, 0x1, "the data is considered, but did no changes were made on message object" ) \
    m( Block,  0x2, "blocks further propagation of the current event" )         \
    m( Error,  0x4, "error occured" )                                           \
    m( Abrupt, 0x8, "no more events have to be processed" )

/// Enum indicating the processor's evaluation result for generic case.
enum GenericSteering {
    # define declare_flag(nm, cd, descr) prc ## nm ## Flag = cd,
    for_each_processor_flag(declare_flag)
    # undef declare_flag

    prcOk = 0x0,
    prcIntact = prcIntactFlag,
    prcDone = prcBlockFlag | prcAbruptFlag,
    prcFlagError = prcErrorFlag | prcBlockFlag | prcAbruptFlag,
};

template<>
class Arbiter<GenericSteering> {
public:
    typedef GenericSteering ArbiteringResult;
private:
    bool _doInit;
    GenericSteering _latest;
public:
    Arbiter() : _doInit(true) {}
    void set_initialized() { _doInit = false; }
    bool is_initialized() const { return !_doInit; }
    void consider_processor_result( GenericSteering rc ) { _latest = rc; }
    bool do_eval_next_processor() const { return !(_latest & prcBlockFlag); }
    bool have_error() const { return (_latest & prcErrorFlag); }
    bool do_eval_next_message() const {
        return // no error flag is set
            !(_latest & prcErrorFlag) &&
            // abrupt flag is not set
            !(_latest & prcAbruptFlag )
            ;
    }
    bool is_done() const {
        return  // block flag is set
                (_latest & prcBlockFlag ) &&
                // abrupt flag is set
                (_latest & prcAbruptFlag)
                ;
    }
};


namespace interfaces {

/// Basic processor interface defining each processor's staging logic.
template<typename MessageT>
class Processor {
public:
    typedef GenericSteering SteeringCode;
protected:
    virtual void _V_init( const MessageT & ) {}
    virtual SteeringCode _V_eval( MessageT & ) = 0;
    virtual void _V_done() {}
public:
    void init( const MessageT & m ) { _V_init(m); }
    SteeringCode eval( MessageT & m ) { return _V_eval(m); }
    void done() { _V_done(); }
};  // class Processor

}  // ::ppt::interfaces



/// Traits defined for certain data type.
template< typename MessageT >
struct Traits {
    /// Processors type; shall define return type as public one within.
    typedef interfaces::Processor<MessageT> iProcessorType;
    /// Arbitering class. Shall be able to treat steering codes returned by
    /// processor eval() method.
    typedef Arbiter<typename iProcessorType::SteeringCode> ArbiterType;
};  // struct Traits



template< typename MessageT >
class Pipe : public std::list< typename Traits<MessageT>::iProcessorType * > {
};  // struct Pipe


template<typename MessageT>
class EvaluationProxy {
private:
    Pipe<MessageT> & pipe;
    Arbiter<typename Traits<MessageT>::iProcessorType::SteeringCode> & arbiter;
protected:
    EvaluationProxy( Pipe<MessageT> & p
                   , Arbiter<typename Traits<MessageT>::iProcessorType::SteeringCode> & a ) : pipe(p), arbiter(a) {}
    ~EvaluationProxy() {
        if( arbiter.is_initialized() ) {
            for( auto it = pipe->begin(); pipe->end() != it; ++it ) {
                it->done();
            }
        }
    }

    bool eval( MessageT & m ) {
        if( ! arbiter.do_eval_next_message() ) {
            // TODO: own exception with details. The arbiter + pipeline
            // combination is not capable to perform subsequent messages
            // treatment, apparently due to an error. This indicates
            // that FSM is in state when new messages treatment is forbidden.
            throw std::runtime_error( "Wrong FSM state: unable to treat new message." );
        }
        // If arbiter orders us to initialize, trigger all processors.
        if( !arbiter.is_initialized() ) {
            for( auto it = pipe->begin(); pipe->end() != it; ++it ) {
                it->init( m );
            }
        }
        // Feed the message to all the processors within the pipeline until
        // the arbiter stops us or all the processors will be passed.
        for( auto it = pipe->begin(); pipe->end() != it; ++it ) {
            arbiter.consider_processor_result( it->eval( m ) );
            if( ! arbiter.do_eval_next_processor() ) break;
        }
        // If arbiter allows us to finalize, trigger all processors being done.
        if( arbiter.is_done() ) {
            for( auto it = pipe->begin(); pipe->end() != it; ++it ) {
                it->done();
            }
        }
        return arbiter.do_eval_next_message();
    }
};

}  // ::ppt

template<typename MessageT> ::ppt::EvaluationProxy<MessageT> &
    operator<<( ::ppt::EvaluationProxy<MessageT> & ep, MessageT & m ) {
    ep.eval(m);
    return ep;
}

# endif  // H_PIPE_T_PIPELINE_H

