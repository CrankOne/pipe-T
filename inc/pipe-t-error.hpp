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

# ifndef H_PIPE_T_EXCEPTION_H
# define H_PIPE_T_EXCEPTION_H

# include <stdexcept>

# define PIPET_EMERGENCY_BUFLEN 256

# define pipet_error( c, ... ) while(true){                         \
    char bf[PIPET_EMERGENCY_BUFLEN];                                \
    snprintf(bf, PIPET_EMERGENCY_BUFLEN, __VA_ARGS__ );             \
    throw pipet::errors:: c (bf);}

namespace pipet {
namespace errors {

/// User code did not initialize some crucial variable.
class Uninitialized : public std::runtime_error {
public:
    Uninitialized( const std::string & s ) : std::runtime_error(s) {}
};  // class Uninitialized


/// Type mismatch while runtime type casting.
class BadCast : public std::runtime_error {
public:
    BadCast( const std::string & s ) : std::runtime_error(s) {}
};  // class Uninitialized

/// Manifold or pipeline primitive is empty and can not perform any work. That's
/// an error because there has no "default result" to be returned by pipeline
/// or manifold in trivial case.
class EmptyManifold : public std::runtime_error {
public:
    EmptyManifold( const std::string & s ) : std::runtime_error(s) {}
};  // class EmptyManifold


/// Indicates runtime error due to erroneous FSM state (user code may
/// potentially break it). Note that NDEBUG macro can disable some checks (for
/// the sake of performance) suppressing this exception.
class Malfunction : public std::runtime_error {
public:
    Malfunction( const std::string & s ) : std::runtime_error(s) {}
};  // class Malfunction


/// Development stub. Indicates code that is not being currently implemented but
/// is provisioned by general architecture.
class NotImplemented : public std::runtime_error {
public:
    NotImplemented( const std::string & s ) : std::runtime_error(s) {}
};  // class NotImplemented

}  // namespace errors
}  // namespace pipet

# endif  // H_PIPE_T_EXCEPTION_H

