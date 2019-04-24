# include <cstdint>
# include <iostream>

typedef uint8_t ProcessorFeatures;
constexpr ProcessorFeatures pft_Stateless = 0x1;

template<typename T>
struct DataTraits {
    typedef       T  & Ref;
    typedef const T  & CRef;
    typedef       T && RRef;
};

class AbstractProcessor {
private:
    ProcessorFeatures _fts;
protected:
    AbstractProcessor( ProcessorFeatures fts ) : _fts(fts) {}
public:
    ProcessorFeatures features() const { return _fts; }
};

template<typename ... ArgTs>
class iProcessor : public AbstractProcessor {
protected:
    virtual void _V_invoke( ArgTs ... ) = 0;
protected:
    iProcessor( ProcessorFeatures fts ) : AbstractProcessor(fts) {}
public:
    void invoke( ArgTs ... args ) { _V_invoke(args...); }
};


template<typename ... ArgTs>
class StatelessProcessor : public iProcessor<ArgTs...> {
private:
    void (*_f)(ArgTs...);
protected:
    virtual void _V_invoke( ArgTs ... args ) override {
        _f( args... );
    }
public:
    StatelessProcessor( void (*f)(ArgTs...) ) : iProcessor<ArgTs...>( pft_Stateless )
                                              , _f(f) {}
};


template<typename ... InTs> struct from {
    template<typename ... OutTs> struct to {
        //typedef std::tuple< typename DataTraits<InTs>::CRef...
        //                  , typename DataTraits<OutTs>::Ref... > ArgTuple;
        typedef StatelessProcessor<typename DataTraits<InTs>::CRef..., typename DataTraits<OutTs>::Ref...> stateless;

    };
};

int
main(int argc, char * argv[]) {
    from<float>::to<int>::stateless p([](const float & a, int & r){r = (int) a;});

    int res;
    p.invoke( 3.14, res );
    std::cout << res << std::endl;
}

