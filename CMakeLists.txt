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

#
# Common CMake
#############

cmake_minimum_required( VERSION 2.6 )
project( pipe-T )

set( pipeT_VERSION_MAJOR 0  )
set( pipeT_VERSION_MINOR 1  )
set( pipeT_VERSION_PATCH dev )
set( pipeT_VERSION ${pipeT_VERSION_MAJOR}.${pipeT_VERSION_MINOR}.${pipeT_VERSION_PATCH} )

option( DOC_HTML "Whether to generate doxygen HTML pages." OFF )
option( UNIT_TESTS "Whether to build unit tests (requires boost)." OFF )

if( DOC_HTML )
    find_package( Doxygen REQUIRED )
endif( DOC_HTML )

set( pipeT_LIB pipeT )

add_library( ${pipeT_LIB} INTERFACE )  # ${pipeT_LIB_SOURCES} ?

target_include_directories( ${pipeT_LIB}
    INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
            $<INSTALL_INTERFACE:include/pipeT> )

#install( TARGETS ${pipeT_LIB} EXPORT pipeT LIBRARY DESTINATION lib/pipeT )
install( DIRECTORY inc/ DESTINATION include/pipeT FILES_MATCHING PATTERN
    "*.h" PATTERN "*.hpp" PATTERN "*.tcc" PATTERN "*.itcc" )

#install( FILES ${CMAKE_CURRENT_BINARY_DIR}/inc/sV_config.h
#         DESTINATION include/pipeT )

if( DOC_HTML )
    configure_file (
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    )
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        #WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen ..." VERBATIM
    )
endif( DOC_HTML )

#
# Py-bindings
if( PYTHON_BINDINGS )
    add_subdirectory( py )
    # Can fix possible issue with target visibility:
    set_source_files_properties( ${PY_MSG_MODULES} PROPERTIES GENERATED TRUE )
    # ^^^ TODO
endif( PYTHON_BINDINGS )

add_subdirectory( test )

#configure_file (
#    "${CMAKE_CURRENT_SOURCE_DIR}/pipeTConfig.cmake.in"
#    "${CMAKE_CURRENT_BINARY_DIR}/pipeTConfig.cmake"
#    @ONLY
#)

#include(CMakePackageConfigHelpers)
#write_basic_package_version_file(
#    ${CMAKE_CURRENT_BINARY_DIR}/pipeTConfigVersion.cmake
#    VERSION ${pipeT_VERSION}
#  COMPATIBILITY AnyNewerVersion
#)
#export( EXPORT pipeT
#    FILE ${CMAKE_CURRENT_BINARY_DIR}/pipeTTargets.cmake
#)
#install( FILES
#    ${CMAKE_CURRENT_BINARY_DIR}/pipeTConfig.cmake
#    ${CMAKE_CURRENT_BINARY_DIR}/pipeTTargets.cmake
#    ${CMAKE_CURRENT_BINARY_DIR}/pipeTConfigVersion.cmake
#    cmake/FindCtemplate.cmake
#    cmake/sV_find_pkgs.cmake
#    cmake/ROOTAuxDicts.cmake
#    DESTINATION lib/cmake/pipeT )

#install( FILES
#         "${CMAKE_CURRENT_SOURCE_DIR}/event.proto"
#         DESTINATION share/pipeT )

#install(
#    CODE "file( GLOB _GeneratedROOTDicts \"${CMAKE_CURRENT_BINARY_DIR}/*.pcm\" )"
#    CODE "file( INSTALL \${_GeneratedROOTDicts} DESTINATION \"${pipeT_ROOT_PLUGINS_INSTALL_PATH}\" )"
#)

#install( EXPORT pipeT DESTINATION lib/cmake/pipeT
#    FILE pipeTTargets.cmake )


