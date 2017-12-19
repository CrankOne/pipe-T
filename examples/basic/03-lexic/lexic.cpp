/** \example lexic.cpp
 * Example demonstrating some lexical capabilities.
 */

# include <pipet.tcc>



int main( inst argc, char * argv[] ) {
    // Declare the pipeline instance
    pipet::Pipe<int> p;
    Source src;

    int msg1, msg2;

    // Immediately process entire source using the given pipeline. Operator<<
    // modifies the source's internal state.
    int result = (p << src);

    // Processes few messages using the pipeline until two successfully
    // processed messages 
    (src | p) >> msg1 >> msg2;

    // Processes msg1 and assignes processing result then to msg2. NOTE:
    // operator<< will construct copy of the message, instead of modifying it.
    (p << msg1) >> msg2;

    return 0;
}


