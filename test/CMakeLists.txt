# Copyright (c) 2016 Renat R. Dusaev <crank@qcrypt.org>
# Author: Renat R. Dusaev <crank@qcrypt.org>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required( VERSION 2.6 )
project( pipeT_ut )

find_package( Boost ${Boost_FORCE_VERSION}
              COMPONENTS unit_test_framework
              REQUIRED )

add_executable( pipeT_ut
                main.cpp handler.cpp basic.cpp forkJunction.cpp )

target_compile_features( pipeT_ut PUBLIC
            c_variadic_macros
            cxx_constexpr
            cxx_final
            cxx_delegating_constructors
            cxx_variadic_macros
            cxx_variadic_templates
            cxx_template_template_parameters )

set( CMAKE_CXX_CFLAGS "${CMAKE_CXX_CFLAGS} -Wfatal-errors" )

target_link_libraries( pipeT_ut ${Boost_LIBRARIES} )
target_link_libraries( pipeT_ut ${pipeT_LIB} )

