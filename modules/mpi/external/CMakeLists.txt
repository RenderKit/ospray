## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_subdirectory(snappy EXCLUDE_FROM_ALL)
set_target_properties(snappy PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

add_subdirectory(pico_bench)

