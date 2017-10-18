#/bin/sh

mkdir build
cd build
rm -rf *

# NOTE(jda) - using Internal tasking system here temporarily to avoid installing TBB
cmake \
-D OSPRAY_TASKING_SYSTEM=Internal \
-D OSPRAY_MODULE_BILINEAR_PATCH=ON \
-D OSPRAY_ENABLE_TESTING=ON \
-D OSPRAY_SG_CHOMBO=OFF \
-D OSPRAY_SG_OPENIMAGEIO=OFF \
-D OSPRAY_SG_VTK=OFF \
..

make -j 4 && make test
