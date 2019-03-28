# include "new.tcc"

# include <iostream>
# include <cstring>

typedef int Value;
typedef ppt::Traits<Value>::Routing ValueTraits;

struct Histogram1D : public ppt::iObserver<Value> {
    unsigned int counts[10];

    Histogram1D() {
        memset( counts, 0, sizeof(counts) );
    }

    virtual typename ValueTraits::ResultCode eval( const Value & v ) override {
        ++counts[ int( float(v) / RAND_MAX ) ];
        return ValueTraits::mark_intact(0);
    }
};

static ValueTraits::ResultCode
_simple_discriminator( const Value & v ) {
    if( v > RAND_MAX/2 ) {
        return ValueTraits::mark_intact( ppt::DefaultRoutingFlags::noPropFlag );
    } else {
        return ValueTraits::mark_intact( 0 );
    }
}

int
main(int argc, char * argv[]) {
    ppt::Pipe<const Value> p;
    p.push_back( new Histogram1D() );
    p.push_back( new ppt::StatelessObserver<Value, int>(_simple_discriminator) );
    p.push_back( new Histogram1D() );

    for( unsigned int i = 0; i < 1e5; ++i ) {
        p.eval( rand() );
    }
}

