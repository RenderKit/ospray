#!/bin/bash
## Copyright 2016-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

rm -rf img
mkdir img

export LD_LIBRARY_PATH=`pwd`/build/install/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=`pwd`/build/install/lib:$DYLD_LIBRARY_PATH
export PATH=`pwd`/build/install/bin:$PATH

ospTestSuite --dump-img --baseline-dir=img/
FAILED=$(echo $?)

exit $FAILED
