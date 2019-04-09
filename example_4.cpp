# include "new.tcc"

# include <cstring>
# include <cstdlib>

struct Event {
    unsigned char data[3*sizeof(double)];
};

namespace ppt {
// Extraction traits describe how the data may be (re-) packed.
template<>
struct ExtractionTraits<const double, const Event> {
    static Traits<Event>::Routing::ResultCode
    process( const Event & e, Pipe<const double> & p ) {
        for( unsigned int i = 0; i < 3; ++i ) {
            p << ( (double *) e.data )[i];
        }
        return 0x0;
    }
};
}

struct Histogram1D : public ppt::iObserver<double> {
    unsigned int counts[10];

    Histogram1D() {
        memset( counts, 0, sizeof(counts) );
    }

    virtual typename ppt::Traits<double>::Routing::ResultCode eval( double v ) override {
        ++counts[ int(10*(float(v) / RAND_MAX)) ];
        return ppt::Traits<double>::Routing::mark_intact(0);
    }
};

int
main(int argc, char * argv[]) {
    // Initialize events to process
    Event events[10];
    // ...
    // Build the pipelines
    ppt::Pipe<const Event> p;  // outern
    ppt::Pipe<const double> ip;  // intern pipeline
    ip.push_back( new Histogram1D() );
    p.push_back( new ppt::Span<const Event, const double>(ip) );
    // Process events
    for( unsigned int i = 0; i < sizeof(events)/sizeof(*events); ++i ) {
        p << events[i];
    }
    return 0;
}

