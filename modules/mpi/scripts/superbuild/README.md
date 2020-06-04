# OSPRay Module MPI CMake Superbuild

This CMake script will pull down module MPI's dependencies and build module MPI itself.
The result is an install directory, with the dependencies installed together as a single package.

Run with:

```bash
mkdir build
cd build
cmake [path/to/this/directory]
cmake --build .
```

The resulting `install` directory (or the one set with `CMAKE_INSTALL_PREFIX`)
will have everything in it.

CMake options to note (all have sensible defaults), these are forwarded to OSPRay's superbuild:

- `CMAKE_INSTALL_PREFIX` will be the root directory where everything gets installed.
- `BUILD_JOBS` sets the number given to `make -j` for parallel builds.
- `BUILD_EMBREE_FROM_SOURCE` set to OFF will download a pre-built version of Embree.
- `BUILD_OIDN_FROM_SOURCE` set to OFF will download a pre-built version of OpenImageDenoise.
- `BUILD_OIDN_VERSION` determines which verison of OpenImageDenoise to pull down.
