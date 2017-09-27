#!/bin/bash

mkdir -p failed
rm failed/*

regression_tests/ospray_test_suite --gtest_output=xml:tests.xml --baseline-dir=regression_tests/baseline/ --failed-dir=failed
FAILED=$(echo $?)

exit $FAILED

