Building and Finding OSPRay
===========================

The latest OSPRay sources are always available at the [OSPRay GitHub
repository](http://github.com/ospray/ospray). The default `master`
branch should always point to the latest bugfix release.

Prerequisites
-------------

OSPRay currently supports Linux, Mac OS\ X, and Windows. In addition,
before you can build OSPRay you need the following prerequisites:

-   You can clone the latest OSPRay sources via:

        git clone https://github.com/ospray/ospray.git

-   To build OSPRay you need [CMake](http://www.cmake.org), any form of
    C++11 compiler (we recommend using GCC, but also support Clang,
    MSVC, and [Intel® C++ Compiler
    (icc)](https://software.intel.com/en-us/c-compilers)), and standard
    Linux development tools.
-   Additionally you require a copy of the [Intel® Implicit SPMD Program
    Compiler (ISPC)](http://ispc.github.io), version 1.20.0 or later.
    Please obtain a release of ISPC from the [ISPC downloads
    page](https://ispc.github.io/downloads.html).
    If ISPC is not found by CMake its location can be hinted with the
    variable `ispcrt_DIR`.
-   OSPRay builds on top of the [Intel oneAPI Rendering Toolkit common
    library (rkcommon)](https://www.github.com/ospray/rkcommon). The
    library provides abstractions for tasking, aligned memory
    allocation, vector math types, among others. For users who also need
    to build rkcommon, we recommend the default the Intel [Threading
    Building Blocks (TBB)](https://www.threadingbuildingblocks.org/) as
    tasking system for performance and flexibility reasons.
    TBB must be built from source when targeting ARM CPUs, or can
    be built from source as part of the [superbuild](#cmake-superbuild).
    Alternatively you can set CMake variable `RKCOMMON_TASKING_SYSTEM`
    to `OpenMP` or `Internal`.
-   OSPRay also heavily uses Intel [Embree], installing version 4.0.0
    or newer is required. If Embree is not found by CMake its location
    can be hinted with the variable `embree_DIR`.
-   OSPRay support volume rendering (enabled by default via
    `OSPRAY_ENABLE_VOLUMES`), which heavily uses Intel [Open
    VKL](https://www.openvkl.org/), version 1.3.2 or newer is required.
    If Open VKL is not found by CMake its location can be hinted with
    the variable `openvkl_DIR`, or disable `OSPRAY_ENABLE_VOLUMES`.
-   OSPRay also provides an optional module implementing the `denoiser`
    image operation, which is enabled by `OSPRAY_MODULE_DENOISER`. This
    module requires Intel [Open Image Denoise] in version 2.0.0 or
    newer. You may need to hint the location of the library with the
    CMake variable `OpenImageDenoise_DIR`.
-   For the optional MPI modules (enabled by `OSPRAY_MODULE_MPI`), which
    provide the `mpiOffload` and `mpiDistributed` devices, you need an
    MPI library and [Google Snappy](https://github.com/google/snappy).
-   The optional example application, the test suit and benchmarks need
    some version of OpenGL and GLFW as well as
    [GoogleTest](https://github.com/google/googletest) and [Google
    Benchmark](https://github.com/google/benchmark/)

Depending on your Linux distribution you can install these dependencies
using `yum` or `apt-get`. Some of these packages might already be
installed or might have slightly different names.

Type the following to install the dependencies using `yum`:

    sudo yum install cmake.x86_64
    sudo yum install tbb.x86_64 tbb-devel.x86_64

Type the following to install the dependencies using `apt-get`:

    sudo apt-get install cmake-curses-gui
    sudo apt-get install libtbb-dev

Under Mac OS\ X these dependencies can be installed using
[MacPorts](http://www.macports.org/):

    sudo port install cmake tbb

Under Windows please directly use the appropriate installers for
[CMake](https://cmake.org/download/),
[TBB](https://github.com/oneapi-src/oneTBB/releases),
[ISPC](https://ispc.github.io/downloads.html) (for your Visual Studio
version) and [Embree](https://github.com/embree/embree/releases/).

