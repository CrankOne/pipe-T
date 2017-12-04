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
    bool operator()(Message & msg) {  // TODO: bool -> void
        BOOST_CHECK( msg.id - 1 == _prevNum );
        _prevNum = msg.id;
        return true;  // XXX
    }
    int latest_id() const { return _prevNum; }
    void reset() { _prevNum = 0; }
};

// Processor mimicing the fork/junction manifold.
class ForkMimic : public TestingManifold::ISource {
private:
    int _nAcc;
    std::vector<Message> _acc;
    std::vector<Message>::iterator _it;
public:
    ForkMimic( int nAcc ) : _nAcc(nAcc)
                          , _it(_acc.end())
        { _acc.reserve(_nAcc); }

    pipet::aux::ManifoldRC operator()(Message & msg) {
        // We aborting propagation here until the fork is filled. Upon
        // latest event comes, we return 'fork filled' flag, causing iterating
        // loop to continue propagation from this fork as the messages source.
        BOOST_CHECK( _acc.size() < _nAcc );
        _acc.push_back( msg );
        if( _acc.size() == _nAcc ) {
            _it = _acc.begin();
            return pipet::aux::kForkFilled;
        }
        return pipet::aux::kNextMessage;
    }

    virtual Message * next() override {
        if( _it != _acc.end() ) {
            return &(*(_it++));
        }
        _acc.clear();
        _it = _acc.end();
        return nullptr;
    }
};

class ManifoldArbiter : public TestingManifold::IArbiter {
protected:
    virtual int _V_pop_result() override {
        Parent::_reset_flags();
        // ?
        return 0;
    }
public:
    typedef TestingManifold::IArbiter Parent;
};

// Testing source, emitting messages with id set to the simple increasing
// natural series.
class TestingSource : public TestingManifold::ISource {
private:
    size_t _nMsgsMax;
    Message _reentrantMessage;
public:
    TestingSource(size_t nMsgsMax) : _nMsgsMax( nMsgsMax )
                                   , _reentrantMessage {0} {}
    virtual Message * next() override {
        if( ++ _reentrantMessage.id > _nMsgsMax ) {
            return nullptr;
        }
        return &_reentrantMessage;
    }
};

}  // namespace test
}  // namespace pipet

//
// Test suite

BOOST_AUTO_TEST_SUITE( Pipeline_suite )

BOOST_AUTO_TEST_CASE( ManifoldPipelineTC ) {

    pipet::test::ManifoldArbiter a;
    pipet::test::TestingManifold mf(&a);

    pipet::test::OrderCheck oc1, oc2;
    pipet::test::ForkMimic fm1(3), fm2(2);

    mf.push_back( oc1 );
    mf.push_back( fm1 );
    mf.push_back( fm2 );
    mf.push_back( oc2 );

    for( size_t nMsgsMax = 1; nMsgsMax < 30; ++nMsgsMax ) {
        pipet::test::TestingSource src(nMsgsMax);
        mf.process( src );
        // Check that ALL the messages have passed throught the manifold.
        BOOST_CHECK( nMsgsMax - 1 == oc1.latest_id() );
        BOOST_CHECK( nMsgsMax - 1 == oc2.latest_id() );
        oc1.reset();
        oc2.reset();
    }
}

BOOST_AUTO_TEST_SUITE_END()

