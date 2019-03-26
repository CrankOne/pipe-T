# include "new.tcc"

# include <cmath>
# include <iostream>

// Message type defining all the family of processors within a pipeline
struct MyMessage {
    float v;
};

// Simple functional mutator
static void _simple_mutator( MyMessage & m ) {
    m.v += 10;
}

// Simple min/max observer
class ValueMin : public ppt::iObserver<MyMessage> {
private:
    float _min;
public:
    ValueMin() : _min(+INFINITY) {}
    ~ValueMin() {}

    virtual typename ppt::Traits<MyMessage>::Routing::ResultCode observe( const MyMessage & m ) override {
        if( _min > m.v || std::isnan(_min) ) _min = m.v;
        return 0;
    }

    float get_min() const { return _min; }
};

// Entry point
int
main(int argc, char * argv[]) {
    MyMessage msgs[] = { {1}, {2} };
    std::cout << "min:" << ( ( ValueMin()
                             | _simple_mutator
                             ) << msgs[0] << msgs[1])[0]->as<ValueMin>().get_min()
              << std::endl
              ;
}

