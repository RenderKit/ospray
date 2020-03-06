@echo off
rem Copyright 2016-2019 Intel Corporation
rem SPDX-License-Identifier: Apache-2.0

setlocal

echo Running tests
md img

set OSP_LIBS=build\Release
set BASELINE_PATH=img\

set PATH=%PATH%;%OSP_LIBS%;%EMBREE_DIR%\bin

dir %BASELINE_PATH%

call build\regression_tests\Release\ospray_test_suite.exe --gtest_output=xml:tests.xml --baseline-dir=%BASELINE_PATH% --dump-img

exit /B %ERRORLEBEL%

endlocal
