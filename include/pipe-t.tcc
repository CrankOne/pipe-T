//# pragma once

# include <cstdint>
# include <tuple>
# include <mutex>

namespace ppt {

# if 0
typedef uint8_t ProcessorTraits;

constexpr uint8_t pt_multipleInputs  = 0x1;
constexpr uint8_t pt_multipleOutputs = 0x2;
constexpr uint8_t pt_observing       = 0x4;
constexpr uint8_t pt_mutating        = 0x8;
constexpr uint8_t pt_reading         = 0x10;
constexpr uint8_t pt_referencing     = 0x20;
constexpr uint8_t pt_stateless       = 0x40;

class AbstractProcessor {
private:
    ProcessorTraits _traits;
protected:
    AbstractProcessor( ProcessorTraits t ) : _traits(t) {}
    AbstractProcessor() : _traits(0x0) {}  // XXX
public:
    ProcessorTraits traits() const { return _traits; }

    bool has_multiple_inputs() const { return _traits & pt_multipleInputs; }
    bool has_multiple_outputs() const { return _traits & pt_multipleOutputs; }
    bool is_observer() const { return (_traits & pt_observing) && !(_traits & pt_mutating); }
    bool is_mutator() const { return (!(_traits & pt_observing)) && (_traits & pt_mutating); }
    // ...
};
# endif

class AbstractProcessor {
protected:
    virtual ~AbstractProcessor() {}
};

/// Default handle, keeps copy of data in thread-local storage.
template<typename T> class DefaultHandle {
private:
    std::mutex _m;
    /*thread_local*/ T _data;
public:
    // ...
};

template<typename T>
struct Traits {
    typedef DefaultHandle<T> Handle;
};
template<typename T>
struct Traits<const T> : public Traits<T> {};

template<typename T>
struct Link : public std::pair<AbstractProcessor *, AbstractProcessor *> {
    typename Traits<T>::Handle value;
};

template<typename ... InTs> class From {
    template<typename ... OutTs> class To : public AbstractProcessor {
    private:
        std::tuple<Link<InTs>...>  _inputs;
        std::tuple<Link<OutTs>...> _outputs;
    public:
        void process() {
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

