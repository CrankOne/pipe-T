/** \example discriminate.cpp
 * This is an example of how to use the simplest possible pipeline.
 *
 * The example demonstrates processing integer-typed messages with single
 * handler.
 */

# include <pipet.tcc>

//! [A bit more complicated message type]
struct MyMessage {
    int a;
};
//! [A bit more complicated message type]

//! [Simple discriminating handler]
// This time we will return a boolean value, instructing the pipeline whether
// to propagate message further. We will discriminate the negative values.
bool my_discriminate_negative( MyMessage & msg ) {
    if( msg.a < 0 )
        return false;
    return true;
}
//! [Simple discriminating handler]

//! [Dumping handler]
void my_processor( MyMessage & msg ){
    std::cout << "Value is: " << msg.a << std::endl;
    msg.a--;
};
//! [Dumping handler]

//! [Discrimination test]
int main( inst argc, char * argv[] ) {
    // Declare the pipeline instance
    pipet::Pipe<MyMessage> p;

    // Add processing functions here:
    p.push_back( my_discriminate_negative );
    p.push_back( my_processor );
    p.push_back( my_discriminate_negative );
    p.push_back( my_processor );

    // Process a message.
    MyMessage msg[3] = { {-1}, {0}, {1} };

    p << msg[0]  // will print nothing (abort propagation on first processor)
      << msg[1]  // will print only 0 (abort propagation on third processor)
      << msg[2]  // will print 1 and 0 (propagation won't be aborted)
      ;

    return 0;
}
//! [Discrimination test]
