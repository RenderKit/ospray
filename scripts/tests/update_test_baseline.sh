#!/bin/bash
## Copyright 2016 Intel Corporation
## SPDX-License-Identifier: Apache-2.0
#author=Carson Brownlee
#generates md5 checksums for generated regression test image
# and uploads checksum and image to sdvis server
#
# argument 1: name of test
# argument 2: directory of test result images
# argument 3: path of ospray source
# argument 4: path to ospray-test-data
# argument 5: ISA (optional)
#

isas="AVX2 AVX512SKX"
md5=`md5sum $2/$1.png | awk '{print $1 }'`
echo $md5

if [ -n "$5" ]; then
  # if ISA specified place reference image only in the ISA specific directories
  isas=$5
fi
for isa in $isas
do
  echo $md5 > $3/test_image_data/baseline/$isa/$1.png.md5
done

cp $2/$1.png $4/MD5/$md5
cd $4
git add MD5/$md5
