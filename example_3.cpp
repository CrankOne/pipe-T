# include "new.tcc"

# include <iostream>
# include <iomanip>
# include <cstring>

typedef int Value;
typedef ppt::Traits<Value>::Routing ValueTraits;

struct Histogram1D : public ppt::iObserver<Value> {
    unsigned int counts[10];

    Histogram1D() {
        memset( counts, 0, sizeof(counts) );
    }

    virtual typename ValueTraits::ResultCode _V_eval( Value v ) override {
        ++counts[ int(10*(float(v) / RAND_MAX)) ];
        return ValueTraits::mark_intact(0);
    }
};

static ValueTraits::ResultCode
_simple_discriminator( Value v ) {
    if( v > RAND_MAX/2 ) {
        return ValueTraits::mark_intact( ppt::DefaultRoutingFlags::noPropFlag );
    } else {
        return ValueTraits::mark_intact( 0 );
    }
}

int
main(int argc, char * argv[]) {
    ppt::Pipe<const Value> p;

    assert( p.is_observer() );

    p.push_back( new Histogram1D() );
    p.push_back( new ppt::StatelessObserver<Value, int>(_simple_discriminator) );
    p.push_back( new Histogram1D() );

    for( unsigned int i = 0; i < 1e5; ++i ) {
        p.eval( rand() );
    }
    for( unsigned char i = 0; i < 10; ++i ) {
        std::cout << std::setw(10) << p[0]->as<Histogram1D>().counts[i] << ", "
                  << std::setw(10) << p[2]->as<Histogram1D>().counts[i] << std::endl;
    }
}

