Building OSPRay from Source
===========================

The latest OSPRay sources are always available at the [OSPRay GitHub
repository](http://github.com/ospray/ospray). The default `master`
branch should always point to the latest tested bugfix release.

Prerequisites
-------------

OSPRay currently supports Linux, Mac OS\ X, and Windows. In addition,
before you can build OSPRay you need the following prerequisites:

-   You can clone the latest OSPRay sources via:

        git clone https://github.com/ospray/ospray.git

-   To build OSPRay you need [CMake](http://www.cmake.org), any form
    of C++11 compiler (we recommend using GCC, but also support Clang and the
    [Intel® C++ Compiler (icc)](https://software.intel.com/en-us/c-compilers)),
    and standard Linux development tools. To build the example viewers, you
    should also have some version of OpenGL.
-   Additionally you require a copy of the [Intel® SPMD Program
    Compiler (ISPC)](http://ispc.github.io), version 1.9.1 or later.
    Please obtain a release of ISPC from the [ISPC downloads
    page](https://ispc.github.io/downloads.html). The build
    system looks for ISPC in the `PATH` and in the directory right
    "next to" the checked-out OSPRay sources.^[For example, if OSPRay is
    in `~/Projects/ospray`, ISPC will also be searched in
    `~/Projects/ispc-v1.9.2-linux`]
    Alternatively set the CMake variable `ISPC_EXECUTABLE` to the
    location of the ISPC compiler.
-   Per default OSPRay uses the Intel® [Threading Building
    Blocks](https://www.threadingbuildingblocks.org/) (TBB) as tasking
    system, which we recommend for performance and flexibility reasons.
    Alternatively you can set CMake variable `OSPRAY_TASKING_SYSTEM` to
    `OpenMP`, `Internal`, or `Cilk` (icc only).
-   OSPRay also heavily uses [Embree], installing version 2.15 or newer
    is required. If Embree is not found by CMake its location can be
    hinted with the variable `embree_DIR`.

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
[TBB](https://github.com/01org/tbb/releases),
[ISPC](https://ispc.github.io/downloads.html) (for your Visual Studio
version) and [Embree](https://github.com/embree/embree/releases/).


Compiling OSPRay on Linux and Mac OS\ X
---------------------------------------

Assume the above requisites are all fulfilled, building OSPRay through
CMake is easy:

-   Create a build directory, and go into it

        mkdir ospray/build
        cd ospray/build

    (We do recommend having separate build directories for different
    configurations such as release, debug, etc).

-   The compiler CMake will use will default to whatever the `CC` and
    `CXX` environment variables point to. Should you want to specify a
    different compiler, run cmake manually while specifying the desired
    compiler. The default compiler on most linux machines is `gcc`, but
    it can be pointed to `clang` instead by executing the following:

        cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..

    CMake will now use Clang instead of GCC. If you are ok with using
    the default compiler on your system, then simply skip this step.
    Note that the compiler variables cannot be changed after the first
    `cmake` or `ccmake` run.

-   Open the CMake configuration dialog

        ccmake ..

-   Make sure to properly set build mode and enable the components you
    need, etc; then type 'c'onfigure and 'g'enerate. When back on the
    command prompt, build it using

        make

-   You should now have `libospray.so` as well as a set of example
    application. You can test your version of OSPRay using any of the
    examples on the [OSPRay Demos and Examples] page.


Compiling OSPRay on Windows
---------------------------

On Windows using the CMake GUI (`cmake-gui.exe`) is the most convenient
way to configure OSPRay and to create the Visual Studio solution files:

-   Browse to the OSPRay sources and specify a build directory (if it
    does not exist yet CMake will create it).

-   Click "Configure" and select as generator the Visual Studio version
    you have, for Win64 (32\ bit builds are not supported by OSPRay),
    e.g. "Visual Studio 15 2017 Win64".

-   If the configuration fails because some dependencies could not be
    found then follow the instructions given in the error message, e.g.
    set the variable `embree_DIR` to the folder where Embree was
    installed.

-   Optionally change the default build options, and then click
    "Generate" to create the solution and project files in the build
    directory.

-   Open the generated `OSPRay.sln` in Visual Studio, select the build
    configuration and compile the project.


Alternatively, OSPRay can also be built without any GUI, entirely on the
console. In the Visual Studio command prompt type:

    cd path\to\ospray
    mkdir build
    cd build
    cmake -G "Visual Studio 15 2017 Win64" [-D VARIABLE=value] ..
    cmake --build . --config Release

Use `-D` to set variables for CMake, e.g. the path to Embree with "`-D
embree_DIR=\path\to\embree`".

You can also build only some projects with the `--target` switch.
Additional parameters after "`--`" will be passed to `msbuild`. For
example, to build in parallel only the OSPRay library without the
example applications use

    cmake --build . --config Release --target ospray -- /m

