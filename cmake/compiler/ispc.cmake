## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# ISPC versions to look for, in descending order (newest first)
set(ISPC_VERSION_WORKING "1.18.0")
list(GET ISPC_VERSION_WORKING -1 ISPC_VERSION_REQUIRED)

find_package(ispcrt REQUIRED)
