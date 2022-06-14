#!/bin/bash
## Copyright 2014 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e # terminate if some error occurs

PACKAGE=$1

primary_bundle_id="com.intel.ospray"
user=$MACOS_NOTARIZATION_USER
password=$MACOS_NOTARIZATION_PASSWORD

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
