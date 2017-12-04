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

# include "basic_pipeline.tcc"

// Unit test, performing checks for template type-deducing mechanics.

# include <string>

# define BOOST_TEST_NO_MAIN
//# define BOOST_TEST_MODULE Data source with metadata
# include <boost/test/unit_test.hpp>

namespace pipet {
namespace test {

// Pair of nonwrapping processors:
int trivial_processor( double & ) { return 0; }

struct ProcessorClass {
    int _some;
    ProcessorClass() : _some(0) {}
    int operator()( double & val ) { _some++; return val > 0 ? 1 : -1; }
};

struct ComplicatedHandlerResult {
    bool a, b;
};

}  // namespace test

namespace aux {

template<>
struct HandlerResultConverter<test::ComplicatedHandlerResult, int> {
    virtual test::ComplicatedHandlerResult convert_result( int n ) { return {n > 0, n < 0}; }
};

}  // namespace aux
}  // namespace pipet

//
// Test suite

BOOST_AUTO_TEST_SUITE( Pipeline_types )

// This case checks, whether the both, function and functor managing handlers
// are properly created for the case when wrapped processors returns result of
// type expected by the pipeline.
BOOST_AUTO_TEST_CASE( NonwrappingHandlers ) {
    double someVal = 1.23;
    pipet::aux::PipelineHandler< double
                               , int
                               , int
                               , int(&)(double &) > h1( pipet::test::trivial_processor );
    // Check, that handler #1 is really invokable:
    BOOST_CHECK( h1.process( someVal ) == 0 );

    pipet::test::ProcessorClass p1;
    pipet::aux::PipelineHandler< double
                               , int
                               , int
                               , pipet::test::ProcessorClass > h2( p1 );

    // Check, that handler #2 is really invokable:
    BOOST_CHECK( h2.process( someVal ) == 1 );
    BOOST_CHECK( h2.process( someVal ) == 1 );
    // Check, that handler #2 is really referencing to particular processor
    // instance (not its copy)
    BOOST_CHECK( p1._some == 2 );
}

// This case checks, whether the both, function and functor managing handlers
// are properly created for the case when wrapped processors returns result of
// type that is NOT expected by the pipeline (namely,
// test::ComplicatedHandlerResult, while processors return integer values).
// At this case, the HandlerResultConverter<> of certain implementation is
// invoked to provide conversion method.
BOOST_AUTO_TEST_CASE( WrappingHandlers ) {
    double someVal = 1.23;
    pipet::aux::PipelineHandler< double
                               , pipet::test::ComplicatedHandlerResult
                               , int
                               , int(&)(double &) > h1( pipet::test::trivial_processor );
    // Check, that handler #1 is really invokable:
    pipet::test::ComplicatedHandlerResult r = h1.process( someVal );
    BOOST_CHECK( !r.a );
    BOOST_CHECK( !r.a );
    pipet::test::ProcessorClass p1;
    pipet::aux::PipelineHandler< double
                               , pipet::test::ComplicatedHandlerResult
                               , int
                               , pipet::test::ProcessorClass > h2( p1 );

    // Check, that handler #2 is really invokable:
    r = h2.process( someVal );
    BOOST_CHECK(  r.a );
    BOOST_CHECK( !r.b );
    someVal = 0;
    r = h2.process( someVal );
    BOOST_CHECK( !r.a );
    BOOST_CHECK(  r.b );
    // Check, that handler #2 is really referencing to particular processor
    // instance (not its copy)
    BOOST_CHECK( p1._some == 2 );
}

BOOST_AUTO_TEST_SUITE_END()



