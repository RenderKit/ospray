# OSPRay CMake Superbuild

This CMake script will pull down OSPRay's dependencies and build OSPRay itself.
The result is an install directory with each dependency in its own directory.

Run with:

```bash
git clone https://github.com/jeffamstutz/superbuild_ospray
cd superbuild_ospray
mkdir build
cd build
cmake ..
make
```

The resulting `install` directory will have everything in it.

CMake options to note (all have sensible defaults):

- `CMAKE_INSTALL_PREFIX` will be the root directory where everything gets installed.
- `NUM_BUILD_JOBS` sets the number given to `make -j` for parallel builds.
- `BUILD_EMBREE_VERSION` determines which verison of Embree to pull down.
