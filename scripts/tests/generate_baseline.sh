#!/bin/bash
## Copyright 2016-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# results are put in a new folder called generated_test_images

rm -rf generated_test_images
mkdir generated_test_images

ospTestSuite --dump-img --baseline-dir=generated_test_images/
FAILED=$(echo $?)

exit $FAILED
