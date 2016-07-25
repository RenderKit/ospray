#!/bin/sh

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
