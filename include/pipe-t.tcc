//# pragma once

# include <cstdint>
# include <tuple>
# include <mutex>

namespace ppt {

template<typename T>
struct Traits {
    typedef T & Ref;
    typedef std::pair<Ref, std::mutex *> Handle;

    static bool try_lock( Handle h ) { return h.second.try_lock(); }
    static void unlock( Handle h ) { h.second.unlock(); }
};
template<typename T>
struct Traits<const T> : public Traits<T> {};

/** Link represents a 
 *      * thread-local */
template<typename T>
struct Link : public std::pair<AbstractProcessor *, AbstractProcessor *> {
    typename Traits<T>::Handle value;
};

template<typename ... InTs> class From {
    template<typename ... OutTs> class To : public AbstractProcessor {
    protected:
        CallableHanle<InTs..., OutTs...> _c;
    public:
        void eval() {
            // TODO: lock output links and do the job.
            for( auto l :  ) {
            }
            //_V_process(...);
            //https://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
        }
    };
};

}  // namespace ::ppt

int
main(int argc, char * argv[]) {
    ppt::From<int, float>::To<double, double> p;
}

