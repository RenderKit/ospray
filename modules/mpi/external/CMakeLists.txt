## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# External projects aren't required to pass -Werror
# as we don't modify their code

string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_subdirectory(snappy EXCLUDE_FROM_ALL)
set_target_properties(snappy PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

add_subdirectory(pico_bench)

