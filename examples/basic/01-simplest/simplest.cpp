/** \example simplest.cpp
 * This is an example of how to use the simplest possible pipeline.
 *
 * The example demonstrates processing integer-typed messages with single
 * handler.
 */


# include <pipet.tcc>

//! [Simplest handler]
void my_processor( int & msg ) {
    std::cout << "Got integer value " << msg << std::endl;
}
//! [Simplest handler]

int main( inst argc, char * argv[] ) {
    //! [Simplest pipeline]
    // Declare the pipeline instance
    pipet::Pipe<int> p;

    // Add our processing function as a pipeline handler.
    p.push_back( my_processor );

    // Process a message.
    p << 1;
    //! [Simplest pipeline]

    return 0;
}

