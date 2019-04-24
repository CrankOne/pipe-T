
# include <cassert>
# include <bitset>
# include <mutex>
# include <thread>
# include <vector>
# include <iostream>
# include <condition_variable>
# include <chrono>

constexpr size_t NProcessors = 10;

template<std::size_t N>
class TierMonitor {
private:
    std::mutex _accessMtx;
    std::condition_variable _cv;
    std::bitset<N> _freeFlags;
public:
    TierMonitor() : _freeFlags(0x0) {
        for( size_t np = 0; np < NProcessors; ++np ) {  // TODO
            _freeFlags.set(np);
        }
    }

    void set_free( size_t n ) {
        std::unique_lock<std::mutex> lock(_accessMtx);
        _freeFlags.set(n);
        _cv.notify_all();
    }

    /// Blocks execution of current thread until one of the given will become
    /// available.
    size_t borrow_one( const std::bitset<N> & toProcess ) {
        size_t n;
        std::unique_lock<std::mutex> lock(_accessMtx);
        auto available = toProcess & _freeFlags;
        while( available.none() ) {
            _cv.wait(lock);  // NOTE: frees _accessMtx while waiting
            available = toProcess & _freeFlags;
        }
        // Get first freed processor number, mark it as busy and return number
        for( n = 0; n < N; ++n ) {
            if( available.test(n) ) {
                _freeFlags.reset(n);
                return n;
            }
        }
        assert(false);
    }
};

// Testing fixture
/////////////////

// Represents a worker thread operating with tier monitor.
template<size_t NTasks> void
worker( TierMonitor<NTasks> & tm
      , std::chrono::milliseconds msecDelay
      , size_t nThread ) {
    std::vector<size_t> visitedOnes;

    std::bitset<NTasks> toProcess;
    for( size_t n = 0; n < NTasks; ++n ) { toProcess.set(n); }

    while( toProcess.any() ) {
        size_t one2Procs = tm.borrow_one( toProcess );
        std::cout << "#" << nThread
                  << "(" << toProcess << ") got " << one2Procs << std::endl;
        {   // Here the real job with one2Procs-th processor in tier has to be
            // performed.
            visitedOnes.push_back( one2Procs );
            std::this_thread::sleep_for(msecDelay);
        }
        tm.set_free(one2Procs);
        toProcess.reset(one2Procs);
        std::cout << "#" << nThread
                  << "(" << toProcess << ") done with " << one2Procs << std::endl;
    }

    assert( toProcess.none() );
}

int
main(int arc, char * argv[]) {
    TierMonitor<NProcessors> tm;
    using namespace std::chrono_literals;

    size_t nThreads = atoi(argv[1]);
    std::thread ** threads;
    threads = new std::thread * [nThreads];
    for( size_t n = 0; n < nThreads; ++n ) {
        size_t msecDelay = (rand()/double(RAND_MAX))*1000;
        threads[n] = new std::thread( worker<NProcessors>
                                    , std::ref(tm)
                                    , msecDelay*1ms
                                    , n );
    }
    for( size_t n = 0; n < nThreads; ++n ) {
        threads[n]->join();
    }
}

