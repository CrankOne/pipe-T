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

# include "pipet.tcc"

# include <string>

# define BOOST_TEST_NO_MAIN
//# define BOOST_TEST_MODULE Data source with metadata
# include <boost/test/unit_test.hpp>

namespace pipet {
namespace test {

struct Message {
    // ...
};

class Source : public pipet::interfaces::Source<Message> {
    // ...
};

class Processor

}  // namespace test

namespace aux {
// Define source wrapper simply iterating over gSrcMsgs array
template<>
struct SourceTraits< pipet::test::Message *
                   , pipet::test::Message > {
    class Iterator {
    private:
        pipet::test::Message * _latest;
    public:
        Iterator(pipet::test::Message * msgs) : _latest(msgs) {}
        virtual pipet::test::Message * get() {
            if( _latest->id ) {
                return _latest++;
            }
            return nullptr;
        }
    };
};
}  // namespace aux

}  // namespace pipet

//
// Test suite

BOOST_AUTO_TEST_SUITE( Pipeline_suite )

BOOST_AUTO_TEST_CASE( LinearPipelineTC ) {
    pipet::Pipe<pipet::test::Message> p;
    pipet::test::Source src;
    pipet::test::Message msg, msg1, msg2;

    // Immediately process entire source using the given pipeline. Operator<<
    // modifies the source's internal state.
    int result = (p << src);

    // Processes few messages using the pipeline until two successfully
    // processed messages 
    (src | p) >> msg1 >> msg2;

    // Processes msg1 and assignes processing result then to msg2. NOTE:
    // operator<< will construct copy of the message, instead of modifying it.
    (p << msg1) >> msg2;
}

BOOST_AUTO_TEST_SUITE_END()


