## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

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
