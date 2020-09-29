#!/bin/bash
## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e
KW_CRITICAL_OUTPUT_PATH=/tmp/critical
export KW_BUILD_NUMBER=$(cat $CI_PROJECT_DIR/kw_build_number)
echo "Checking for critical issues in $KW_BUILD_NUMBER ..."
no_proxy=$KW_SERVER_IP curl -f --data "action=search&project=$KW_PROJECT_NAME&query=build:'$KW_BUILD_NUMBER'%20severity:Critical%20status:Analyze,Fix&user=$KW_USER&ltoken=$KW_LTOKEN" http://$KW_SERVER_IP:$KW_SERVER_PORT/review/api -o $KW_CRITICAL_OUTPUT_PATH
getCriticalCount() {
        cat $KW_CRITICAL_OUTPUT_PATH | wc -l
}
if [ -f $KW_CRITICAL_OUTPUT_PATH ]; then
        echo "Critical issues found - $(getCriticalCount) in $KW_BUILD_NUMBER";
        python -m json.tool $KW_CRITICAL_OUTPUT_PATH
        exit 1;
else
        echo "No critical issues were found in $KW_BUILD_NUMBER"
fi
