#!/bin/bash
## ======================================================================== ##
## Copyright 2016-2019 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

export LD_LIBRARY_PATH=`pwd`/build/install/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=`pwd`/build/install/lib:$DYLD_LIBRARY_PATH
export PATH=`pwd`/build/install/bin:$PATH

echo "Running 'ospTutorial'..."
ospTutorial
echo "...done!"

echo "Running 'ospTutorialCpp'..."
ospTutorialCpp
echo "...done!"

echo "Running 'ospTutorialAsync'..."
ospTutorialAsync
echo "...done!"

exit $?
