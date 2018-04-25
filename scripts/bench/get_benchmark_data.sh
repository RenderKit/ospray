#!/bin/sh
## ======================================================================== ##
## Copyright 2016-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

data_dir="test_data"

# Create data directory and fill it
if [ ! -d ${data_dir} ]; then
  mkdir ${data_dir}
fi

cd ${data_dir}

# FIU
fiu="fiu.bz2"
if [ ! -e ${fiu} ]; then
  wget http://sdvis.org/~jdamstut/test_data/${fiu}
  tar -xaf ${fiu}
fi

# HEPTANE
csafe="csafe.bz2"
if [ ! -e ${csafe} ]; then
  wget http://sdvis.org/~jdamstut/test_data/${csafe}
  tar -xaf ${csafe}
fi

# MAGNETIC
magnetic="magnetic-512-volume.bz2"
if [ ! -e ${magnetic} ]; then
  wget http://sdvis.org/~jdamstut/test_data/${magnetic}
  tar -xaf ${magnetic}
fi

# LLNL ISO
llnl_iso="llnl-2048-iso.tar.bz2"
if [ ! -e ${llnl_iso} ]; then
  wget http://sdvis.org/~jdamstut/test_data/${llnl_iso}
  tar -xaf ${llnl_iso}
fi

sponza="crytek-sponza.tgz"
if [ ! -e ${sponza} ]; then
  wget http://www.sdvis.org/ospray/download/demos/${sponza}
  tar -xaf ${sponza}
fi

sanmiguel="san-miguel.tgz"
if [ ! -e ${sanmiguel} ]; then
  wget http://www.sdvis.org/ospray/download/demos/${sanmiguel}
  tar -xaf ${sanmiguel}
fi

sibenik="sibenik.tgz"
if [ ! -e ${sibenik} ]; then
  wget http://www.sdvis.org/ospray/download/demos/${sibenik}
  tar -xaf ${sibenik}
fi
