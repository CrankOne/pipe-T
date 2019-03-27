# include "new.tcc"

# include <cmath>
# include <iostream>

struct MyEvent {
    float r[3];
    float norm;
};

static ppt::Traits<MyEvent>::Routing::ResultCode
_conditional_mutator( MyEvent & eve ) {
    if( eve.r[0] > 0 ) {
        eve.norm = sqrt( eve.r[0]*eve.r[0]
                   + eve.r[1]*eve.r[1]
                   + eve.r[2]*eve.r[2]
                   );
        return 0;
    }
    return ppt::Traits<MyEvent>::Routing::intactFlag;
}

static void
_print_event( const MyEvent & eve ) {
    std::cout << "norm: " << eve.norm << std::endl;
}

int
main(int argc, char * argv[]) {
    ppt::Pipe<MyEvent> p;
    p.push_back( new ppt::StatelessMutator<MyEvent, int>(_conditional_mutator) );
    p.push_back( new ppt::StatelessObserver<MyEvent, void>(_print_event) );
    p << MyEvent{  1, 1, 1, NAN }
      << MyEvent{ -1, 1, 1, NAN }
      ;
}

