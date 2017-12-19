/*
 * Copyright (c) 2016 Renat R. Dusaev <crank@qcrypt.org>
 * Author: Renat R. Dusaev <crank@qcrypt.org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

# include "tstStubs.hpp"
# include "pipet.tcc"
# include <set>

# include <iostream>  // XXX

namespace pipet {
namespace test {

class FilteringProcessor : public std::set<int> {
public:
    FilteringProcessor( const std::set<int> & s ) : std::set<int>(s) {}
    bool operator()( Message & msg ) {
        if( std::set<int>::find(msg.id) != this->end() ) {
            return false;
        }
        return true;
    }
};

}
} // namespace pipet

BOOST_AUTO_TEST_SUITE( Lexical_suite )

BOOST_AUTO_TEST_CASE( EvaluationProxyConstruction ) {
    pipet::Pipe<pipet::test::Message> p;
    pipet::test::TestingSource2 src(3);

    auto ep = src | p;

    BOOST_CHECK_EQUAL( &(ep.pipeline()), &p );
    BOOST_CHECK_EQUAL( &(ep.source()), &src );
}

// Processes few messages using the pipeline until two successfully
// processed messages 
BOOST_AUTO_TEST_CASE( ProcessingSyntax ) {
    pipet::test::TestingSource2 src(3);
    pipet::Pipe<pipet::test::Message> p;

    pipet::test::OrderCheck o;
    p.push_back( o );
    int rc = p << src;
}

// Processes few messages using the pipeline until two successfully
// processed messages 
BOOST_AUTO_TEST_CASE( ProxyEventExtraction ) {
    pipet::test::TestingSource2 src(4);
    pipet::Pipe<pipet::test::Message> p;

    pipet::test::Message msg1, msg2;

    pipet::test::OrderCheck o;
    pipet::test::FilteringProcessor fp( {2} );

    //p.push_back(o);
    p.push_back(fp);

    (src | p) >> msg1 >> msg2;

    BOOST_CHECK_EQUAL( 1, msg1.id );
    BOOST_CHECK_EQUAL( 3, msg2.id );
}

# if 0
// Immediately process entire source using the given pipeline. Operator<<
// modifies the source's internal state.
BOOST_AUTO_TEST_CASE( EntireSourceProcessing ) {
    pipet::Pipe<pipet::test::Message> p;
    pipet::test::TestingSource2 src(3);

    int result = (p << src);
    // ...
}

// Processes msg1 and assignes processing result then to msg2. NOTE:
// operator<< will construct copy of the message, instead of modifying it.
BOOST_AUTO_TEST_CASE( TemporarySourceProcessing ) {
    pipet::Pipe<pipet::test::Message> p;
    pipet::test::TestingSource2 src(3);
    pipet::test::Message msg1, msg2. msg3;

    (p << msg1 << msg2) >> msg2;
    // ...
}
# endif

BOOST_AUTO_TEST_SUITE_END()


