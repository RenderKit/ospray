# OSPRay CMake Superbuild

This CMake script will pull down OSPRay's dependencies and build OSPRay itself.
The result is an install directory, with each dependency in its own directory.

Run with:

```bash
mkdir build
cd build
cmake [path/to/this/directory]
cmake --build .
```

The resulting `install` directory (or the one set with `CMAKE_INSTALL_PREFIX`)
will have everything in it, with one subdirectory per dependency.

CMake options to note (all have sensible defaults):

- `CMAKE_INSTALL_PREFIX` will be the root directory where everything gets installed.
- `BUILD_JOBS` sets the number given to `make -j` for parallel builds.
- `BUILD_EMBREE_FROM_SOURCE` set to OFF will download a pre-built version of Embree.
- `BUILD_OIDN_FROM_SOURCE` set to OFF will download a pre-built version of OpenImageDenoise.
- `BUILD_OIDN_VERSION` determines which verison of OpenImageDenoise to pull down.
