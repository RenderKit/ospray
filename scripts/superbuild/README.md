## CMake Superbuild

For convenience, OSPRay provides a CMake Superbuild script which will
pull down OSPRay’s dependencies and build OSPRay itself. By default, the
result is an install directory, with each dependency in its own
directory.

Run with:

```sh
mkdir build
cd build
cmake [<OSPRAY_SOURCE_DIR>/scripts/superbuild]
cmake --build .
```

On Windows make sure to select the non-default 64bit generator, e.g.

```sh
cmake -G "Visual Studio 15 2017 Win64" [<OSPRAY_SOURCE_DIR>/scripts/superbuild]
```

The resulting `install` directory (or the one set with
`CMAKE_INSTALL_PREFIX`) will have everything in it, with one
subdirectory per dependency.

CMake options to note (all have sensible defaults):

CMAKE_INSTALL_PREFIX
:   will be the root directory where everything gets installed.

BUILD_JOBS
:   sets the number given to `make -j` for parallel builds.

INSTALL_IN_SEPARATE_DIRECTORIES
:   toggles installation of all libraries in separate or the same
    directory.

BUILD_EMBREE_FROM_SOURCE
:   set to OFF will download a pre-built version of Embree.

BUILD_OIDN_FROM_SOURCE
:   set to OFF will download a pre-built version of Open Image Denoise.

OIDN_VERSION
:   determines which version of Open Image Denoise to pull down.

BUILD_OSPRAY_MODULE_MPI
:   set to ON to build OSPRay’s MPI module for data-replicated and
    distributed parallel rendering on multiple nodes.

BUILD_TBB_FROM_SOURCE
:   set to ON to build TBB from source (required for ARM support). The
    default setting is OFF.

For the full set of options, run:

```sh
ccmake [<OSPRAY_SOURCE_DIR>/scripts/superbuild]
```

or

```sh
cmake-gui [<OSPRAY_SOURCE_DIR>/scripts/superbuild]
```

### Cross-Compilation with the Superbuild

The superbuild can be passed a [CMake Toolchain
file](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
to configure for cross-compilation. This is done by passing the
toolchain file when running cmake. When cross compiling it is also
likely that you’ll want to build TBB and Embree from source to ensure
they’re built for the correct target, rather than the target the Github
binaries are built for. It may also be necessary to disable specific
ISAs for the target by passing `BUILD_ISA_<ISA_NAME>=OFF` as well.

```sh
mkdir build
cd build
cmake --toolchain [toolchain_file.cmake] [path/to/this/directory]
    -DBUILD_TBB_FROM_SOURCE=ON \
    -DBUILD_EMBREE_FROM_SOURCE=ON \
    <other arguments>
```

While OSPRay supports ARM natively, it may be desirable to cross-compile
it for `x86_64` to run in Rosetta depending on the application integrating
OSPRay. This can be done using the toolchain file
`toolchains/macos-rosetta.cmake`, and by disabling all non-SSE ISAs when
building. This can also be done by launching an `x86_64` bash shell and
then compiling as usual in this environment, which will cause the compilation
chain to target `x86_64`. The `BUILD_ISA_<ISA NAME>=OFF` flags should be
passed to disable all ISAs besides SSE4 for Rosetta:

```sh
arch -x86_64 bash
mkdir build
cd build
cmake [path/to/this/directory]
    -DBUILD_TBB_FROM_SOURCE=ON \
    -DBUILD_EMBREE_FROM_SOURCE=ON \
    -DBUILD_ISA_AVX=OFF \
    -DBUILD_ISA_AVX2=OFF \
    -DBUILD_ISA_AVX512=OFF \
    <other arguments>
```


