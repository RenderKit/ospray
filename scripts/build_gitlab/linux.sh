#/bin/sh

mkdir build
cd build
rm -rf *
cmake \
  -D OSPRAY_BUILD_ISA=ALL \
  -D OSPRAY_APPS_PARTICLEVIEWER=ON \
  -D OSPRAY_MODULE_TACHYON=ON \
..
make -j`nproc`
