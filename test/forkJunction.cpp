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

/**This unit test checks the pipeline message iteration capabilites. The
 * purpose is to check whether the abort/fork-filling logic works as expected.
 *
 * In order to mimic the fork-filling, we use a fake processor, that
 * accumulates few messages before them may be further propagated. The topology
 * may be illustrated with the following ASCII diagram:
 *
 *
 * S - o1 - F(3) - F(2) - o2
 *
 * Where the legend:
 *  S --- messages source
 *  o1 --- processor assuring that messages were extracted in correct order
 *     prior to filling the fork
 *  F(3) ---
 *
 *  ...
 *
 *  Order of message appearing on o1/o2 is just a natural series: 1, 2, 3, ...
 * */

namespace pipet {
namespace test {

class PipelineTestingFixture {
protected:
    pipet::GenericArbiter<int> _a;
    OrderCheck _oc[4];
    ForkMimic _fork2
            , _fork3
            , _fork4
            ;
public:
    PipelineTestingFixture() : _fork2(2)
                             , _fork3(3)
                             , _fork4(4)
                             {}
    ~PipelineTestingFixture() {}
};

}  // namespace test
}  // namespace pipet

//
// Test suite

BOOST_FIXTURE_TEST_SUITE( forkJunctionLogicSuite, pipet::test::PipelineTestingFixture )

// Border case --- an empty pipeline. Shall do nothing except of iterating the
// source sepleting it.
BOOST_AUTO_TEST_CASE( emptyPipeline ) {
    pipet::Pipe<pipet::test::Message> mf;

    pipet::test::TestingSource2 src(10);
    pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
        _a, mf.upcast(), src );

    // Check that source is empty.
    BOOST_CHECK( ! src.get() );
}

// Border case --- an empty pipeline. Shall do nothing except of pulling one
// message from source.
BOOST_AUTO_TEST_CASE( emptyPipelinePull ) {
    pipet::Pipe<pipet::test::Message> mf;

    pipet::test::TestingSource2 src(10);
    pipet::test::Message msg;
    pipet::Pipe<pipet::test::Message>::TheHandlerTraits::pull_one(
        _a, mf.upcast(), src, msg );

    BOOST_CHECK_EQUAL( 1, msg.id );
    BOOST_CHECK_EQUAL( 2, src.get()->id );
}

// Border case --- a pipeline consisting of single handler.
BOOST_AUTO_TEST_CASE( singularPropagation
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/emptyPipeline") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        // Check that ALL the messages have passed throught the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        _oc[0].reset();
    }
}

// Border case --- a pipeline consisting of single handler, pull_one() check.
BOOST_AUTO_TEST_CASE( singularPropagationPull
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/emptyPipelinePull") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        for( size_t n = 0; n < nMsgsMax; ++n ) {
            pipet::test::Message msg;
            pipet::Pipe<pipet::test::Message>::TheHandlerTraits::pull_one(
                _a, mf.upcast(), src, msg );
            // ...
        }
        // Check that ALL the messages have passed throught the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        _oc[0].reset();
    }
}

// Simplest test case. Just composes pipeline of 3 trivial always-forwarding
// processors to check that main pipe processing loop actually able to
// propagate messages.
BOOST_AUTO_TEST_CASE( simplePropagation
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/singularPropagation") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _oc[1] );
    mf.push_back( _oc[2] );
    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        // Check that ALL the messages have passed throught the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[2].latest_id() );
        _oc[0].reset();
        _oc[1].reset();
        _oc[2].reset();
    }
}

// A bit more elaborated (than simplePropagation) test case. Puts a simple
// fork/junction handler of capacity 2 between two message counters to check
// that f/j handler is able to do its job.
BOOST_AUTO_TEST_CASE( simpleFork2
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/simplePropagation") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork2 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 2; nMsgsMax < 10; nMsgsMax += 2 ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        // Check that ALL the messages have passed through the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK( _fork2.was_full() );
        _oc[0].reset();
        _oc[1].reset();
        _fork2.reset();
    }
}

// Same as simpleFork2, but for triple fork.
BOOST_AUTO_TEST_CASE( simpleFork3
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/simplePropagation") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork3 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 3; nMsgsMax < 15; nMsgsMax += 3 ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        // Check that ALL the messages have passed through the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK( _fork3.was_full() );
        _oc[0].reset();
        _oc[1].reset();
        _fork3.reset();
    }
}

// Same as simpleFork2, but for four-fork.
BOOST_AUTO_TEST_CASE( simpleFork4
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/simplePropagation") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 4; nMsgsMax < 20; nMsgsMax += 4 ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK( _fork4.was_full() );
        _oc[0].reset();
        _oc[1].reset();
        _fork4.reset();
    }
}

// This test case checks that fork processing algorithm is able to handle
// source with number of messages that is aliquant to the fork's number.
BOOST_AUTO_TEST_CASE( singleFork
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/simpleFork4") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 1; nMsgsMax < 12; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        if( nMsgsMax >= 4 ) {
            BOOST_CHECK( _fork4.was_full() );
        } else {
            BOOST_CHECK( !_fork4.was_full() );
        }
        _oc[0].reset();
        _oc[1].reset();
        _fork4.reset();
    }
}

// This test case checks that pull_one algorithm is able to handle
// source with number of messages that is aliquant to the fork's number.
BOOST_AUTO_TEST_CASE( singleForkPull
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/singularPropagationPull") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 1; nMsgsMax < 12; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        for( size_t n = 0; n < nMsgsMax; ++n ) {
            pipet::test::Message msg;
            pipet::Pipe<pipet::test::Message>::TheHandlerTraits::pull_one(
                _a, mf.upcast(), src, msg );
            // ...
        }
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        // TODO: depends on wether the f/j can emit messages before it becomes
        // full. SEARCH_PATTERN: NOTE-I-1
        # if 0
        if( nMsgsMax >= 4 ) {
            BOOST_CHECK( _fork4.was_full() );
        } else {
            BOOST_CHECK( !_fork4.was_full() );
        }
        # endif
        _oc[0].reset();
        _oc[1].reset();
        _fork4.reset();
    }
}

// Like singularPropagation TC but with fork. Checks that single f/j node can
// be adequately processed.
BOOST_AUTO_TEST_CASE( standaloneFork
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/simpleFork4") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _fork4 );
    for( size_t nMsgsMax = 1; nMsgsMax < 11; ++nMsgsMax ) {  // todo: < 12
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        if( nMsgsMax >= 4 ) {
            BOOST_CHECK( _fork4.was_full() );
        } else {
            BOOST_CHECK( !_fork4.was_full() );
        }
        _fork4.reset();
    }
}

// Checks pipeline with 3-forking and then 2-forking handle (tapering pipe).
BOOST_AUTO_TEST_CASE( forks3to2
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/singleFork") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork3 );
    mf.push_back( _fork2 );
    mf.push_back( _oc[1] );

    for( size_t nMsgsMax = 1; nMsgsMax < 4; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        if( nMsgsMax >= 2 ) {
            BOOST_CHECK( _fork2.was_full() );
        } else {
            BOOST_CHECK( !_fork2.was_full() );
        }
        if( nMsgsMax >= 3 ) {
            BOOST_CHECK( _fork3.was_full() );
        } else {
            BOOST_CHECK( !_fork3.was_full() );
        }
        _oc[0].reset();
        _oc[1].reset();
        _fork2.reset();
        _fork3.reset();
    }
}

// Checks pipeline with 2-forking and then 3-forking handle (expanding pipe).
BOOST_AUTO_TEST_CASE( forks2to3
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/singleFork") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork2 );
    mf.push_back( _fork3 );
    mf.push_back( _oc[1] );

    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        // Check that ALL the messages have passed throught the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        _oc[0].reset();
        _oc[1].reset();
        _fork3.reset();
        _fork2.reset();
    }
}

// Test for mixed topology (tapering and then expanding) to check that there is
// no obvious multiplicity effects causing messages reordering or loss.
BOOST_AUTO_TEST_CASE( combinedForks
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/singleFork") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    mf.push_back( _fork2 );
    mf.push_back( _oc[2] );
    mf.push_back( _fork3 );
    mf.push_back( _oc[3] );

    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        // Check that ALL the messages have passed throught the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[2].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[3].latest_id() );
        if( nMsgsMax >= 2 ) {
            BOOST_CHECK( _fork2.was_full() );
        } else {
            BOOST_CHECK( !_fork2.was_full() );
        }
        if( nMsgsMax >= 3 ) {
            BOOST_CHECK( _fork3.was_full() );
        } else {
            BOOST_CHECK( !_fork3.was_full() );
        }
        if( nMsgsMax >= 4 ) {
            BOOST_CHECK( _fork4.was_full() );
        } else {
            BOOST_CHECK( !_fork4.was_full() );
        }
        _oc[0].reset();
        _oc[1].reset();
        _oc[2].reset();
        _oc[3].reset();
        _fork2.reset();
        _fork3.reset();
        _fork4.reset();
    }
}

// Test for mixed topology (tapering and then expanding) to check that there is
// no obvious multiplicity effects causing messages reordering or loss.
BOOST_AUTO_TEST_CASE( combinedForksPull
                    , *boost::unit_test::depends_on("forkJunctionLogicSuite/singleFork") ) {
    pipet::Pipe<pipet::test::Message> mf;
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    mf.push_back( _fork2 );
    mf.push_back( _oc[2] );
    mf.push_back( _fork3 );
    mf.push_back( _oc[3] );

    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        # if 0
        pipet::Pipe<pipet::test::Message>::TheHandlerTraits::process(
            _a, mf.upcast(), src );
        # endif
        for( size_t n = 0; n < nMsgsMax; ++n ) {
            pipet::test::Message msg;
            pipet::Pipe<pipet::test::Message>::TheHandlerTraits::pull_one(
                _a, mf.upcast(), src, msg );
            // ...
        }
        // Check that ALL the messages have passed throught the pipe.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[2].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[3].latest_id() );
        // TODO: depends on wether the f/j can emit messages before it becomes
        // full. SEARCH_PATTERN: NOTE-I-1
        # if 0
        if( nMsgsMax >= 2 ) {
            BOOST_CHECK( _fork2.was_full() );
        } else {
            BOOST_CHECK( !_fork2.was_full() );
        }
        if( nMsgsMax >= 3 ) {
            BOOST_CHECK( _fork3.was_full() );
        } else {
            BOOST_CHECK( !_fork3.was_full() );
        }
        if( nMsgsMax >= 4 ) {
            BOOST_CHECK( _fork4.was_full() );
        } else {
            BOOST_CHECK( !_fork4.was_full() );
        }
        # endif
        _oc[0].reset();
        _oc[1].reset();
        _oc[2].reset();
        _oc[3].reset();
        _fork2.reset();
        _fork3.reset();
        _fork4.reset();
    }
}

BOOST_AUTO_TEST_SUITE_END()
