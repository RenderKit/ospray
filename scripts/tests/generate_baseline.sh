#!/bin/bash

mkdir -p img
rm img/*

regression_tests/ospray_test_suite --dump-img --baseline-dir=img/
FAILED=$(echo $?)

exit $FAILED
