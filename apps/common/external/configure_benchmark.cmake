## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE) # don't build tests
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE) # don't install bench lib

mark_as_advanced(BENCHMARK_ENABLE_TESTING)
mark_as_advanced(BENCHMARK_BUILD_32_BITS)
mark_as_advanced(BENCHMARK_CXX_FLAGS_COVERAGE)
mark_as_advanced(BENCHMARK_DOWNLOAD_DEPENDENCIES)
mark_as_advanced(BENCHMARK_ENABLE_ASSEMBLY_TESTS)
mark_as_advanced(BENCHMARK_ENABLE_EXCEPTIONS)
mark_as_advanced(BENCHMARK_ENABLE_GTEST_TESTS)
mark_as_advanced(BENCHMARK_ENABLE_INSTALL)
mark_as_advanced(BENCHMARK_ENABLE_LTO)
mark_as_advanced(BENCHMARK_ENABLE_TESTING)
mark_as_advanced(BENCHMARK_EXE_LINKER_FLAGS_COVERAGE)
mark_as_advanced(BENCHMARK_SHARED_LINKER_FLAGS_COVERAGE)
mark_as_advanced(BENCHMARK_USE_LIBCXX)

add_subdirectory(benchmark)
