# include "new.tcc"

# include <cstring>
# include <cstdlib>
# include <iostream>

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
        return ppt::Traits<double>::Routing::mark_intact(0);
    }
};
}

struct Histogram1D : public ppt::iObserver<double> {
    unsigned int counts[10];

    Histogram1D() {
        memset( counts, 0, sizeof(counts) );
    }

protected:
    virtual typename ppt::Traits<double>::Routing::ResultCode
    _V_eval( double v ) override {
        ++counts[ int(10*(float(v) / RAND_MAX)) ];
        return ppt::Traits<double>::Routing::mark_intact(0);
    }
};

int
main(int argc, char * argv[]) {
    Histogram1D * hstPtr;
    // Initialize events to process
    Event events[10];
    for( size_t i = 0; i < sizeof(events)/sizeof(*events); ++i ) {
        ((double *) events[i].data)[0] = rand();
        ((double *) events[i].data)[1] = rand();
        ((double *) events[i].data)[2] = rand();
    }
    // Build the pipelines
    ppt::Pipe<const Event> p;  // outern
    ppt::Pipe<const double> ip;  // intern pipeline
    ip.push_back( hstPtr = new Histogram1D() );
    p.push_back( new ppt::Span<const Event, const double>(ip) );
    # ifndef PPT_DISABLE_JOUNRALING
    // Assign journal
    typename ppt::journaling::Traits<const Event>::Journal j;
    p.assign_journal(j);
    # endif
    // Process events
    for( unsigned int i = 0; i < sizeof(events)/sizeof(*events); ++i ) {
        p << events[i];
    }
    // Print the stats
    //for( size_t i = 0; i < sizeof(Histogram1D::counts)/sizeof(*hstPtr->counts); ++i ) {
    //    std::cout << hstPtr->counts[i] << std::endl;
    //}
    # ifndef PPT_DISABLE_JOUNRALING
    ppt::journaling::Traits<Event>::print_info( std::cout, p );
    j.print(std::cout);
    # endif
    return 0;
}

