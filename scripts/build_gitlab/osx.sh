#/bin/sh

mkdir build
cd build
rm -rf *

# NOTE(jda) - using Internal tasking system here temporarily to avoid installing TBB
cmake \
-D OSPRAY_TASKING_SYSTEM=Internal \
-D OSPRAY_MODULE_BILINEAR_PATCH=ON \
..

make -j 4
