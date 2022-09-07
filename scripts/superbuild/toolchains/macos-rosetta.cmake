## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# This toolchain file is used to cross-compile OSPRay for x86_64 on ARM-based
# MacOS systems (i.e., to run in Rosetta). Although OSPRay itself builds a
# native ARM/M1 binary by default, compiling for x86_64 can be useful on such
# systems when integrating OSPRay with applications that do not run natively on
# ARM yet.
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CLANG_TRIPLE x86_64-apple-darwin)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${CLANG_TRIPLE})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${CLANG_TRIPLE})

