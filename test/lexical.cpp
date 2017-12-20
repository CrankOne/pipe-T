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

namespace pipet {
namespace test {

static void * gSourceExpectedToFailPtr = nullptr;

bool check_source_addr( pipet::errors::UnableToPull const & ex ) {
    return ex.source_pointer() == gSourceExpectedToFailPtr;
}

}  // namespace test
}  // namespace pipet

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
    int rc = p <= src;
}

// Processes few messages using the pipeline until two successfully
// processed messages 
BOOST_AUTO_TEST_CASE( ProxyEventExtraction ) {
    pipet::test::TestingSource2 src(4);
    pipet::Pipe<pipet::test::Message> p;

    pipet::test::Message msg1, msg2;

    pipet::test::OrderCheck o;
    pipet::test::FilteringProcessor fp( {2} );

    p.push_back(o);
    p.push_back(fp);

    (src | p) >> msg1 >> msg2;

    BOOST_CHECK_EQUAL( 1, msg1.id );
    BOOST_CHECK_EQUAL( 3, msg2.id );
}

// Processes msg1 and assignes processing result then to msg2. NOTE:
// operator<< will construct copy of the message, instead of modifying it.
BOOST_AUTO_TEST_CASE( TemporarySourceProcessing ) {
    using namespace ::pipet;
    using namespace ::pipet::test;
    Pipe<Message> p;
    Message msg1(1), msg2(2), msg3(-1), msg4(-1);

    OrderCheck o(1);
    ForkMimic fm(2, 2);
    FilteringProcessor fp( {2}, 3 );

    (p |= o) |= fm;
    p |= fp;

    auto proxy = (p << msg1 << msg2 >> msg3);

    gSourceExpectedToFailPtr = &proxy;

    BOOST_CHECK_EXCEPTION( operator>>(std::move(proxy), msg4)
                         , ::pipet::errors::UnableToPull
                         , ::pipet::test::check_source_addr );

    BOOST_CHECK_EQUAL( msg1.id,  1 );
    BOOST_CHECK_EQUAL( msg1.procPassed.size(),  0 );
    BOOST_CHECK_EQUAL( msg2.id,  2 );
    BOOST_CHECK_EQUAL( msg3.id,  1 );
    BOOST_CHECK_EQUAL( msg3.procPassed.size(),  3 );
    BOOST_CHECK_EQUAL( msg4.id, -1 );  // this event was discriminated

    std::vector<int> cv = { 1, 2, 3 };
    BOOST_CHECK_EQUAL_COLLECTIONS( msg3.procPassed.begin(), msg3.procPassed.end()
                                 , cv.begin(), cv.end()
                                 );
}

BOOST_AUTO_TEST_SUITE_END()


