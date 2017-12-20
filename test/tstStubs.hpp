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

# ifndef H_PIPET_TEST_STUB_CLASSES_H
# define H_PIPET_TEST_STUB_CLASSES_H

# include "pipeline.tcc"

# include <set>

# define BOOST_TEST_NO_MAIN
# include <boost/test/unit_test.hpp>

namespace pipet {
namespace test {

struct Message {
    int id;
    std::vector<int> procPassed;

    Message() : id(-1) {}
    Message(int vid) : id(vid) {}
};

// Simple processor to check that all events appears in order and remember the
// latest one's id.
class OrderCheck {
private:
    int _prevNum;
    int _id;
public:
    OrderCheck() : _prevNum(0), _id(-1) {}
    OrderCheck( int id ) : _prevNum(0), _id(id) {}
    OrderCheck( const OrderCheck & ) = delete;
    bool operator()(Message & msg) {
        BOOST_CHECK_EQUAL( msg.id - 1, _prevNum );
        _prevNum = msg.id;
        msg.procPassed.push_back( _id );
        return true;
    }
    int latest_id() const { return _prevNum; }
    void reset() { _prevNum = 0; }
};

// Processor mimicking the fork/junction handler.
class ForkMimic : public interfaces::Source<Message> {
private:
    int _nAcc;
    std::vector<Message> _acc;
    Message _cMsg;
    bool _wasFull;
    int _id;
public:
    ForkMimic( int nAcc ) : _nAcc(nAcc), _wasFull(false), _id(-1)
        { _acc.reserve(_nAcc); }

    ForkMimic( int nAcc, int id ) : _nAcc(nAcc), _wasFull(false), _id(id)
        { _acc.reserve(_nAcc); }

    pipet::PipeRC operator()(Message & msg) {
        // We aborting propagation here until the fork is filled. Upon
        // latest event comes, we return 'fork filled' flag, causing iterating
        // loop to continue propagation from this fork as the messages source.
        BOOST_CHECK_LT( _acc.size(), _nAcc );
        msg.procPassed.push_back( _id );
        _acc.push_back( msg );
        if( _acc.size() >= _nAcc ) {
            _wasFull = true;
            return PipeRC::Complete;
        }
        return PipeRC::MessageKept;
    }

    virtual Message * get() override {
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

// Discriminates messages with given id.
class FilteringProcessor : public std::set<int> {
private:
    int _id;
public:
    FilteringProcessor( const std::set<int> & s, int id ) : std::set<int>(s), _id(id) {}
    FilteringProcessor( const std::set<int> & s ) : std::set<int>(s), _id(-1) {}
    bool operator()( Message & msg ) {
        msg.procPassed.push_back( _id );
        if( std::set<int>::find(msg.id) != this->end() ) {
            return false;
        }
        return true;
    }
};

// Testing source, emitting messages with id set to the simple increasing
// natural series.
class TestingSource2 {
private:
    size_t _nMsgsMax;
    Message _reentrantMessage;
public:
    TestingSource2(size_t nMsgsMax) : _nMsgsMax( nMsgsMax )
                                   , _reentrantMessage(0) {}
    virtual Message * get() {
        if( ++ _reentrantMessage.id > _nMsgsMax ) {
            return nullptr;
        }
        return &_reentrantMessage;
    }
};

}  // namespace test

namespace aux {
// Define source wrapper simply iterating over gSrcMsgs array
template<>
struct SourceTraits< test::TestingSource2
                   , test::Message> {
    class Iterator : public interfaces::Source<test::Message> {
    private:
        test::TestingSource2 & _srcRef;
    public:
        Iterator(test::TestingSource2 & src) : _srcRef(src) {}
        virtual pipet::test::Message * get() {
            return _srcRef.get();
        }
    };
};
}  // namespace aux

}  // namespace pipet

# endif  // H_PIPET_TEST_STUB_CLASSES_H

