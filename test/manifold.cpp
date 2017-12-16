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

# if 1
# include "manifold.tcc"

# define BOOST_TEST_NO_MAIN
//# define BOOST_TEST_MODULE Data source with metadata
# include <boost/test/unit_test.hpp>

/**This unit test checks the basic manifold event iteration capabilites. The
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

struct Message {
    int id;
};

typedef pipet::Manifold<Message, int> TestingManifold;

// Simple processor to check that all events appears in order and remember the
// latest one's id.
class OrderCheck {
private:
    int _prevNum;
public:
    OrderCheck() : _prevNum(0) {}
    OrderCheck( const OrderCheck & ) = delete;
    bool operator()(Message & msg) {  // TODO: bool -> void
        BOOST_CHECK_EQUAL( msg.id - 1, _prevNum );
        _prevNum = msg.id;
        return true;  // XXX
    }
    int latest_id() const { return _prevNum; }
    void reset() { _prevNum = 0; }
};

// Processor mimicking the fork/junction manifold.
class ForkMimic : public TestingManifold::ISource {
private:
    int _nAcc;
    std::vector<Message> _acc;
    Message _cMsg;
    bool _wasFull;
public:
    ForkMimic( int nAcc ) : _nAcc(nAcc), _wasFull(false)
        { _acc.reserve(_nAcc); }

    pipet::aux::ManifoldRC operator()(Message & msg) {
        // We aborting propagation here until the fork is filled. Upon
        // latest event comes, we return 'fork filled' flag, causing iterating
        // loop to continue propagation from this fork as the messages source.
        BOOST_CHECK_LT( _acc.size(), _nAcc );
        _acc.push_back( msg );
        if( _acc.size() == _nAcc ) {
            _wasFull = true;
            return pipet::aux::RC_ForkFilled;
        }
        return pipet::aux::RC_ForkFilling;
    }

    virtual Message * next() override {
        if( !_acc.empty() ) {
            _cMsg = *_acc.begin();
            _acc.erase(_acc.begin());
            return &_cMsg;
        }
        return nullptr;
    }

    void reset() {
        _acc.clear();
        _wasFull = false;
    }

    bool was_full() const { return _wasFull; }
};

class ManifoldArbiter : public TestingManifold::IArbiter {
protected:
    virtual int pop_result() override {
        Parent::_reset_flags();
        // ?
        return 0;
    }
public:
    typedef TestingManifold::IArbiter Parent;
};

// Testing source, emitting messages with id set to the simple increasing
// natural series.
class TestingSource2 : public TestingManifold::ISource {
private:
    size_t _nMsgsMax;
    Message _reentrantMessage;
public:
    TestingSource2(size_t nMsgsMax) : _nMsgsMax( nMsgsMax )
                                   , _reentrantMessage {0} {}
    virtual Message * next() override {
        if( ++ _reentrantMessage.id > _nMsgsMax ) {
            return nullptr;
        }
        return &_reentrantMessage;
    }
};

class ManifoldTestingFixture {
protected:
    ManifoldArbiter _a;
    OrderCheck _oc[4];
    ForkMimic _fork2
            , _fork3
            , _fork4
            ;
public:
    ManifoldTestingFixture() : _fork2(2)
                             , _fork3(3)
                             , _fork4(4)
                             {}
    ~ManifoldTestingFixture() {}
};

}  // namespace test
}  // namespace pipet

//
// Test suite

BOOST_FIXTURE_TEST_SUITE( manifoldSuite, pipet::test::ManifoldTestingFixture )

// Simplest test case. Just composes manifold of 3 trivial always-forwarding
// processors to check that main Manifold processing loop actually able to
// propagate messages.
BOOST_AUTO_TEST_CASE( simplePropagation ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _oc[1] );
    mf.push_back( _oc[2] );
    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed throught the manifold.
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
                    , *boost::unit_test::depends_on("manifoldSuite/simplePropagation") ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _fork2 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 2; nMsgsMax < 10; nMsgsMax += 2 ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed through the manifold.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK( _fork2.was_full() );
        _oc[0].reset();
        _oc[1].reset();
        _fork2.reset();
    }
}

BOOST_AUTO_TEST_CASE( simpleFork3
                    , *boost::unit_test::depends_on("manifoldSuite/simplePropagation") ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _fork3 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 3; nMsgsMax < 15; nMsgsMax += 3 ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed through the manifold.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK( _fork3.was_full() );
        _oc[0].reset();
        _oc[1].reset();
        _fork3.reset();
    }
}

BOOST_AUTO_TEST_CASE( simpleFork4
                    , *boost::unit_test::depends_on("manifoldSuite/simplePropagation") ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 4; nMsgsMax < 20; nMsgsMax += 4 ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed through the manifold.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK( _fork4.was_full() );
        _oc[0].reset();
        _oc[1].reset();
        _fork4.reset();
    }
}

BOOST_AUTO_TEST_CASE( singleFork
                    /*, *boost::unit_test::depends_on("manifoldSuite/simpleFork4")*/ ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    for( size_t nMsgsMax = 1; nMsgsMax < 2; ++nMsgsMax ) {  // todo: < 12
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed through the manifold.
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

BOOST_AUTO_TEST_CASE( forks3to2
                    , *boost::unit_test::depends_on("manifoldSuite/singleFork") ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _fork3 );
    mf.push_back( _fork2 );
    mf.push_back( _oc[1] );

    for( size_t nMsgsMax = 1; nMsgsMax < 4; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed throught the manifold.
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
            BOOST_CHECK( !_fork2.was_full() );
        }
        _oc[0].reset();
        _oc[1].reset();
        _fork2.reset();
        _fork3.reset();
    }
}

BOOST_AUTO_TEST_CASE( forks2to3
                    , *boost::unit_test::depends_on("manifoldSuite/singleFork") ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _fork2 );
    mf.push_back( _fork3 );
    mf.push_back( _oc[1] );

    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed throught the manifold.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        _oc[0].reset();
        _oc[1].reset();
        _fork3.reset();
        _fork2.reset();
    }
}

BOOST_AUTO_TEST_CASE( combinedForks
                    , *boost::unit_test::depends_on("manifoldSuite/singleFork") ) {
    pipet::test::TestingManifold mf(&_a);
    mf.push_back( _oc[0] );
    mf.push_back( _fork4 );
    mf.push_back( _oc[1] );
    mf.push_back( _fork2 );
    mf.push_back( _oc[2] );
    mf.push_back( _fork3 );
    mf.push_back( _oc[3] );

    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource2 src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed throught the manifold.
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[0].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[1].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[2].latest_id() );
        BOOST_CHECK_EQUAL( nMsgsMax, _oc[3].latest_id() );
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
# endif
