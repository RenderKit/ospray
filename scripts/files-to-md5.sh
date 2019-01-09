#!/bin/bash
## ======================================================================== ##
## Copyright 2017-2019 Intel Corporation                                    ##
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

# This script should be called from build directory (OSPRay root should be in ../)
#
# Usage:
# CI_TARGET_MACHINE_PATH=user@machine-name:/target/path BASELINE_MD5_DIR=/dir/with/md5/hashes
# BASELINE_DIR=/dir/with/files/to/be/synced ../scripts/files-to-md5.sh


md5 () {
    md5sum $1 | awk '{print $1 }'
}

if [[ ! $CI_TARGET_MACHINE_PATH ]]
then
  echo "Please set CI_TARGET_MACHINE_PATH env variable"
  exit 1
fi

if [[ ! $BASELINE_MD5_DIR ]]
then
  echo "Please set BASELINE_MD5_DIR env variable"
  exit 1
fi

if [[ ! $BASELINE_DIR ]]
then
  echo "Please set BASELINE_DIR env variable"
  exit 1
fi

LOCAL_TMP_DIR=/tmp/ospray-tmp-data

# Remove any old files (whole directory)
rm -rf $LOCAL_TMP_DIR

# Create this directory again
mkdir $LOCAL_TMP_DIR

# From now on any command should stop executing script
set -e
for FILE in $BASELINE_DIR/*; do
    # Copy file to local tmp directory with new name (based on md5 from this img)
    cp $FILE $LOCAL_TMP_DIR/`md5 $FILE`
    # Create/Filll metadata file in ospray repo
    # so we can link img in remote repo by this md5 string
    md5 $FILE > $BASELINE_MD5_DIR/`basename $FILE`.md5
done
# Copy all files (only when there is need to update) to target CI machine
rsync --progress -au --no-o --no-g $LOCAL_TMP_DIR/* $CI_TARGET_MACHINE_PATH

# Another cleanup - we don't want to leave any files locally
rm -rf $LOCAL_TMP_DIR/*

