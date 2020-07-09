## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(raw_to_amr STATIC rawToAMR.cpp)
target_link_libraries(raw_to_amr PUBLIC rkcommon::rkcommon)
target_include_directories(raw_to_amr INTERFACE ${CMAKE_CURRENT_LIST_DIR})
