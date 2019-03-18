# include "include/pipeline.tcc"

struct MyMsg {
};

int
main(int argc, char * const argv[] ) {
    ppt::Pipe<MyMsg> p;
    return 0;
}

