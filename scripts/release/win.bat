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

md build_release
cd build_release

rem set release settings
cmake -L ^
-G "Visual Studio 14 2015 Win64" ^
-T "Intel C++ Compiler 18.0" ^
-D OSPRAY_BUILD_ISA=ALL ^
-D OSPRAY_MODULE_MPI=ON ^
-D OSPRAY_MODULE_MPI_APPS=OFF ^
-D OSPRAY_SG_CHOMBO=OFF ^
-D OSPRAY_SG_OPENIMAGEIO=OFF ^
-D OSPRAY_SG_VTK=OFF ^
-D OSPRAY_ZIP_MODE=OFF ^
-D OSPRAY_INSTALL_DEPENDENCIES=ON ^
-D CMAKE_INSTALL_INCLUDEDIR=include ^
-D CMAKE_INSTALL_LIBDIR=lib ^
-D CMAKE_INSTALL_DATAROOTDIR= ^
-D CMAKE_INSTALL_DOCDIR=doc ^
-D CMAKE_INSTALL_BINDIR=bin ^
..
if %ERRORLEVEL% GEQ 1 goto abort

rem compile and create installers
rem option '--clean-first' somehow conflicts with options after '--' for msbuild
cmake --build . --config Release --target PACKAGE -- /m /nologo
if %ERRORLEVEL% GEQ 1 goto abort

rem create ZIP files
cmake -D OSPRAY_ZIP_MODE=ON ..
cmake --build . --config Release --target PACKAGE -- /m /nologo
if %ERRORLEVEL% GEQ 1 goto abort

cd ..

:abort
endlocal
:end

rem propagate any error to calling PowerShell script
exit
