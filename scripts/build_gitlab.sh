#/bin/sh

mkdir build
cd build
rm -rf *
cmake ..
make -j`nproc`
