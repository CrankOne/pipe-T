# include "new.tcc"

# include <cmath>
# include <iostream>

// Message type defining all the family of processors within a pipeline
struct MyMessage {
    float v;
};

// Simple functional mutator
static ppt::Traits<MyMessage>::Routing::ResultCode _simple_mutator( MyMessage & m ) {
    m.v += 10;
    return 0;
}

// Simple min/max observer
class ValueMin : public ppt::iObserver<MyMessage> {
private:
    float _min;
public:
    ValueMin() : _min(+INFINITY) {}
    ~ValueMin() {}

    virtual typename ppt::Traits<MyMessage>::Routing::ResultCode
    eval( const MyMessage & m ) override {
        if( _min > m.v || std::isnan(_min) ) _min = m.v;
        return ppt::Traits<MyMessage>::Routing::mark_intact( 0 );
    }

    float get_min() const { return _min; }
};

// Entry point
int
main(int argc, char * argv[]) {
    ppt::Pipe<MyMessage> p;
    p.push_back( new ValueMin() );
    p.push_back( new ppt::StatelessMutator<MyMessage, int>(_simple_mutator) );
    MyMessage msgs[] = { {1}, {2} };
    std::cout << "min:" << ( p << msgs[0] << msgs[1])[0]->as<ValueMin>().get_min()
              << std::endl
              ;
}

