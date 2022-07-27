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
# argument 4: ISA (optional)
#

isas="AVX2 AVX512SKX"
md5=`md5sum $2/$1.png | awk '{print $1 }'`
echo $md5

if [ -n "$4" ]; then
  # if ISA specified place reference image only in the ISA specific directories
  isas=$4
fi
for isa in $isas
do
  echo $md5 > $3/test_image_data/baseline/$isa/$1.png.md5
done

remote=/var/www/html/ospray/download/baseline/test-data/MD5/$md5
rsync -e ssh -g -p --chmod=F664 --chown=:web $2/$1.png sdvis.org:$remote
