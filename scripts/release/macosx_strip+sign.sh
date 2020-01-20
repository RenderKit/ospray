#!/bin/bash
## Copyright 2016-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

file=${!#}
dir=`dirname $0`

echo stripping $file
strip $@

# per-file signing
[ -x "$SIGN_FILE_MAC" ] && "$SIGN_FILE_MAC" -o runtime -e "$dir"/ospray.entitlements "$file"
