#!/bin/bash
## Copyright 2016 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

export LD_LIBRARY_PATH=`pwd`/build/install/ospray/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=`pwd`/build/install/ospray/lib:$DYLD_LIBRARY_PATH
export PATH=`pwd`/build/install/ospray/bin:$PATH

exitCode=0

echo -e "\n\nRunning 'ospTutorial'..."
ospTutorial
let exitCode+=$?

echo -e "\n\nRunning 'ospTutorialCpp'..."
ospTutorialCpp
let exitCode+=$?

echo -e "\n\nRunning 'ospTutorialAsync'..."
ospTutorialAsync
let exitCode+=$?

if [[ -x `pwd`/build/install/ospray/bin/ospTutorialGLM ]] ; then 
  echo -e "\n\nRunning 'ospTutorialGLM'..."
  ospTutorialGLM
  let exitCode+=$?
fi

exit $exitCode
