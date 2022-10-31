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
- `BUILD_TBB_FROM_SOURCE` set to ON to build TBB from source (required for ARM support).
   The default setting is OFF.

Actual command line used for build ospray & dependencies on Windows:
(1) Goto a command shell with nmake support:
(2) Run the following command:
\cmake322\bin\cmake  ..\OSPRay.git\scripts\superbuild_ens -G "NMake Makefiles" 
You can use -DCMAKE_BUILD_TYPE=Release for release build
(3) nmake install