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

REF_NAME=${1}

if [[ -n "${REF_NAME}" ]]; then
  echo "Triggering CI build of branch '${REF_NAME}'"
else
  echo "No branch name specified, defaulting to 'devel'"
  REF_NAME="devel"
fi

curl -X POST \
  -F token=32b3258c799f57f1c6159585ce133b \
  -F ref=${REF_NAME} \
  https://gitlab.com/api/v3/projects/998314/trigger/builds
