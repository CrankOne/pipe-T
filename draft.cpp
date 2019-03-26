
// Processor roles:
//  - Mutator -- changes values in the message
//  - Observer -- monitors values in the message, gathers statistics
//  - Router (fork) -- pushes messages in the [parallel] queue
//  - Junction -- pulls messages from the parallel queue
//  - (un-)packer -- transforms one message type into another

// Message type defining all the family of processors within a pipeline
struct MyMessage {
    float v;
};

// Simple functional mutator
static void _simple_mutator( MyMessage & m ) {
    m.v += 10;
}

// Simple min/max observer
class ValueMinMax : public ppt::Observer<MyMessage> {
private:
    float _min, _max;
protected:
    virtual void _V_init( const MyMessage & m ) {
        _min = _max = m.v;
    }
    // Generic method
    //virtual ppt::Traits<MyMessage> _V_run();
    virtual void _V_observe( const Message & m ) {
        if( _min > m.v ) _min = m.v;
        if( _max < m.v ) _max = m.v;
    }
public:
    ValueMinMax() : _min(std::nan)
                  , _max(std::nan) {}
    ~ValueMinMax() {}

    std::pair<float> get_bounds() const { return {_min, _max}; }
};

// Entry point
int
main(int argc, char * argv[]) {
    ppt::Pipe<MyMessage> p = ( ValueMinMax() | _simple_mutator );
    std::cout << "min:" << (p << MyMessage{ 1 } << MyMessage{ 2 })[1].first
              << std::endl
              ;
}

