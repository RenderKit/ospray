@echo off
rem Copyright 2016-2020 Intel Corporation
rem SPDX-License-Identifier: Apache-2.0

setlocal

md generated_test_images

ospTestSuite --dump-img --baseline-dir=generated_test_images

exit /B %ERRORLEBEL%

endlocal
