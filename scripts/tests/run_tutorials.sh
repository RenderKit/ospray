#!/bin/bash
## Copyright 2016-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

export LD_LIBRARY_PATH=`pwd`/build/install/ospray/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=`pwd`/build/install/ospray/lib:$DYLD_LIBRARY_PATH
export PATH=`pwd`/build/install/ospray/bin:$PATH

echo "Running 'ospTutorial'..."
ospTutorial || exit 2

echo "Running 'ospTutorialCpp'..."
ospTutorialCpp || exit 2

echo "Running 'ospTutorialAsync'..."
ospTutorialAsync || exit 2
