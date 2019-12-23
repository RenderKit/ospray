#!/bin/bash
## ======================================================================== ##
## Copyright 2014-2019 Intel Corporation                                    ##
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

set -e # terminate if some error occurs

PACKAGE=$1

primary_bundle_id="com.intel.ospray"
user=$notarization_user
password="@env:notarization_password"

xcrun altool --notarize-app --asc-provider 'IntelCorporationApps' --primary-bundle-id "$primary_bundle_id" --username "$user" --password "$password" --file $PACKAGE 2>&1 | tee notarization_request.log

# get UUID of notarization request
uuid=`cat notarization_request.log | sed -n 's/^[ ]*RequestUUID = //p'`

# query status until no longer in progress
status="in progress"

while [ "$status" = "in progress" ]; do
    
  sleep 60

  xcrun altool --notarization-info "$uuid" --username "$user" --password "$password" 2>&1 | tee notarization_status.log

  status=`cat notarization_status.log | sed -n 's/^[ ]*Status: //p'`

done

logfile=`cat notarization_status.log | sed -n 's/^[ ]*LogFileURL: //p'`
wget -O notarization_logfile.log $logfile
cat notarization_logfile.log

if [ "$status" != "success" ]; then
   echo "Notarization failed!"
   exit 1
fi

issues=`cat notarization_logfile.log | sed -n 's/^[ ]*\"issues\": //p'`

if [ "$issues" != "null" ]; then
  echo "Notarization found issues!"
  exit 1
fi

filename=${PACKAGE%.*}
extension=${PACKAGE##*.}

#disable stapling for now due to bug
#if [ "$extension" = "pkg" ]; then
#  xcrun stapler staple -v $PACKAGE
#fi
