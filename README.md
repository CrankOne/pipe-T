# C++ Template Pipeline Library

Since this is an utility tool designed for developers we're omitting redundant
explainations here starting with few simple examples to explain a purpose.

This example is pretty meaningless, but it demonstrates basic requirements to
run a trivial pipeline:

```C++
# include <pipet.tcc>

void my_processor( int & msg ) {
    std::cout << "Got integer value " << msg << std::endl;
}

int main( inst argc, char * argv[] ) {
    // Declare the pipeline instance
    pipet::Manifld<int> p;

    // Add our processing function as a pipeline handler.
    p.push_back( my_processor );

    // Process a message.
    p << 1;

    return 0;
}
```

(sure, we could equivalently just pass the `msg` instance into the
`my_processor()` for this example to gain an equivalent result)

The next example demonstrates a `true` pipeline --- a conveyor of processors
transforming given `MyMessage` instance.

```C++
# include <pipet.tcc>

struct MyMessage {
    int a;
};

// This time we will return a boolean value, instructing the pipeline whether
// to propagate message further. We will discriminate the negative values.
bool discriminate_negative( MyMessage & msg ) {
    if( msg.a < 0 )
        return false;
    return true;
}

void my_processor( MyMessage & msg ){
    std::cout << "Value is: " << msg.a << std::endl;
    msg.a--;
};

int main( inst argc, char * argv[] ) {
    // Declare the pipeline instance
    pipet::Manifld<MyMessage> p;

    // Add processing functions here:
    p.push_back( discriminate_negative );
    p.push_back( my_processor );
    p.push_back( discriminate_negative );
    p.push_back( my_processor );

    // Process a message.
    MyMessage msg[3] = { {-1}, {0}, {1} };

    p << msg[0]  // will print nothing (abort propagation on first processor)
      << msg[1]  // will print only 0 (abort propagation on third processor)
      << msg[2]  // will print 1 and 0 (propagation won't be aborted)
      ;

    return 0;
}
```

This simple concept has proven to be powerful for data trewatment applications
in which processors are no more stateless functions, but rather class
instances. The may accumulate statistics, store message, broadcast message by
network, and even spawn additional pipelines to perform subsequent operations
on a smaller parts of message data on multithreaded basis.

This library declares a set of template classes which are capable to handle
arbitrary callable and message types:

* `Manifold` class allowing one to build custom hierarchical pipeline
assembly from simple processors;
* `IArbiter` Interface for arbitering and monitoring message propagation. It
allows to handle user return results in a custom manner and organize
fork/junction behaviour for multithreading/multiprocessing behaviour.

