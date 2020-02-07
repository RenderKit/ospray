## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(GOOGLETEST_VERSION 1.10.0)

if (NOT USE_STATIC_RUNTIME)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(gtest)

mark_as_advanced(gtest_build_samples)
mark_as_advanced(gtest_build_tests)
mark_as_advanced(gtest_disable_pthreads)
mark_as_advanced(gtest_force_shared_crt)
mark_as_advanced(gtest_hide_internal_symbols)
