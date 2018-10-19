@echo off
rem ======================================================================== rem
rem Copyright 2015-2018 Intel Corporation                                    rem
rem                                                                          rem
rem Licensed under the Apache License, Version 2.0 (the "License");          rem
rem you may not use this file except in compliance with the License.         rem
rem You may obtain a copy of the License at                                  rem
rem                                                                          rem
rem     http://www.apache.org/licenses/LICENSE-2.0                           rem
rem                                                                          rem
rem Unless required by applicable law or agreed to in writing, software      rem
rem distributed under the License is distributed on an "AS IS" BASIS,        rem
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. rem
rem See the License for the specific language governing permissions and      rem
rem limitations under the License.                                           rem
rem ======================================================================== rem

setlocal

echo Running tests
md failed

set OSP_LIBS=build\Release
set BASELINE_DIR=build\regression_tests\baseline\

set PATH=%PATH%;%OSP_LIBS%;%embree_DIR%\bin

cd build
cmake -G "Visual Studio 15 2017 Win64" ^
-D OSPRAY_ENABLE_TESTING=ON ^
..

cmake --build . --config Release --target ospray_test_data -- /m /nologo
cd ..

call build\regression_tests\Release\ospray_test_suite.exe --gtest_output=xml:tests.xml --baseline-dir=%BASELINE_DIR%

:abort
endlocal
:end

rem propagate any error to calling PowerShell script:
exit
