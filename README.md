OSPRay
======

This is release v0.10.1 of OSPRay. For changes and new features see the
[changelog](CHANGELOG.md). Also visit http://www.ospray.org for more
information.

OSPRay Overview
===============

OSPRay is an **o**pen source, **s**calable, and **p**ortable **ray**
tracing engine for high-performance, high-fidelity visualization on
Intel® Architecture CPUs. OSPRay is released under the permissive
[Apache 2.0 license](http://www.apache.org/licenses/LICENSE-2.0).

The purpose of OSPRay is to provide an open, powerful, and easy-to-use
rendering library that allows one to easily build applications that use
ray tracing based rendering for interactive applications (including both
surface- and volume-based visualizations). OSPRay is completely
CPU-based, and runs on anything from laptops, to workstations, to
compute nodes in HPC systems.

OSPRay internally builds on top of [Embree](https://embree.github.io/)
and [ISPC (Intel® SPMD Program Compiler)](https://ispc.github.io/),
and fully utilizes modern instruction sets like Intel® SSE, AVX, AVX2,
and AVX-512 to achieve high rendering performance.


OSPRay Support and Contact
--------------------------

OSPRay is still in beta stage, and though we do our best to
guarantee stable release versions a certain number of bugs,
as-yet-missing features, inconsistencies, or any other issues are
unavoidable at this stage. Should you find any such issues please report
them immediately via [OSPRay's GitHub Issue
Tracker](https://github.com/ospray/OSPRay/issues) (or, if you should
happen to have a fix for it,you can also send us a pull request); for
missing features please contact us via email at
<ospray@googlegroups.com>.

For recent news, updates, and announcements, please see our complete
[news/updates] page.

Join our [mailing
list](https://groups.google.com/forum/#!forum/ospray-announce/join) to
receive release announcements and major news regarding OSPRay.

Building OSPRay from Source
===========================

The latest OSPRay sources are always available at the [OSPRay GitHub
repository](http://github.com/ospray/ospray). The default `master`
branch should always point to the latest tested bugfix release.

Prerequisites
-------------

OSPRay currently supports both Linux and Mac OS X (and experimentally
Windows). In addition, before you can build OSPRay you need the
following prerequisites:

-   You can clone the latest OSPRay sources via:

        git clone https://github.com/ospray/ospray.git

-   To build OSPRay you need [CMake](http://www.cmake.org), any
    form of C++ compiler (we recommend using the [Intel® C++ compiler
    (icc)](https://software.intel.com/en-us/c-compilers), but also
    support GCC and clang-cc), and standard Linux development tools.
    To build the demo viewers, you should also have some version of
    OpenGL and the GL Utility Toolkit (GLUT or freeglut), as well as
    Qt 4.6 or higher.
-   Additionally you require a copy of the [Intel® SPMD Program
    Compiler (ISPC)](http://ispc.github.io). Please obtain a copy of the
    latest binary release of ISPC (currently 1.9.0) from the [ISPC
    downloads page](https://ispc.github.io/downloads.html). The build
    system looks for ISPC in the `PATH` and in the directory right
    "next to" the checked-out OSPRay sources.^[For example, if OSPRay is
    in `~/Projects/ospray`, ISPC will also be searched in
    `~/Projects/ispc-v1.9.0-linux`] Alternatively set the CMake
    variable `ISPC_EXECUTABLE` to the location of the ISPC compiler.
-   Per default OSPRay uses the Intel® Threading Building Blocks (TBB)
    as tasking system, which we recommend for performance and
    flexibility reasons. Alternatively you can set CMake variable
    `OSPRAY_TASKING_SYSTEM` to `OpenMP`.
-   OSPRay also heavily uses [Embree](http://embree.github.io); however,
    OSPRay directly includes its own copy of Embree, so a special
    installation of Embree is *not* required.

Depending on your Linux distribution you can install these dependencies
using `yum` or `apt-get`. Some of these packages might already be
installed or might have slightly different names.

Type the following to install the dependencies using `yum`:

    sudo yum install cmake.x86_64
    sudo yum install tbb.x86_64 tbb-devel.x86_64
    sudo yum install freeglut.x86_64 freeglut-devel.x86_64
    sudo yum install qt-devel.x86_64

Type the following to install the dependencies using `apt-get`:

    sudo apt-get install cmake-curses-gui
    sudo apt-get install libtbb-dev
    sudo apt-get install freeglut3-dev
    sudo apt-get install libqt4-dev

Under Mac OS X these dependencies can be installed using
[MacPorts](http://www.macports.org/):

    sudo port install cmake tbb freeglut qt4


Compiling OSPRay
----------------

Assume the above requisites are all fulfilled, building OSPRay through
CMake is easy:

-   Create a build directory, and go into it

        user@mymachine[~/Projects]: mkdir ospray/release
        user@mymachine[~/Projects]: cd ospray/release

    (We do recommend having separate build directories for different
    configurations such as release, debug, etc).

-   The compiler CMake will use will default to whatever the `CC` and
    `CXX` environment variables point to. Should you want to specify a
    different compiler, run cmake manually while specifying the desired
    compiler. The default compiler on most linux machines is 'gcc', but
    it can be pointed to 'clang' instead by executing the following:

        user@mymachine[~/Projects/ospray/release]: cmake 
            -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..

    CMake will now use clang instead of gcc. If you are ok with using
    the default compiler on your system, then simply skip this step.
    Note that the compiler variables cannot be changed after the first
    `cmake` or `ccmake` run. 

-   Open the CMake configuration dialog

        user@mymachine[~/Projects/ospray/release]: ccmake ..

-   Make sure to properly set build mode and enable the components you
    need, etc; then type 'c'onfigure and 'g'enerate. When back on the 
    command prompt, build it using

        user@mymachine[~/Projects/ospray/release]: make

-   You should now have `libospray.so` as well as a set of sample
    viewers. You can test your version of OSPRay using any of the
    examples on the [OSPRay Demos and Examples] page.

Examples
========

Tutorial
--------

A minimal working example demonstrating how to use OSPRay can be found
at `apps/ospTutorial.cpp`^[A C99 version is available at
`apps/ospTutorial.c`.]. On Linux build it in the build_directory with

    g++ ../apps/ospTutorial.cpp -I ../ospray/include -I .. -I ../ospray/embree/common \
      ./libospray.so -Wl,-rpath,. -o ospTutorial

On Windows build it in the build_directory\\$Configuration with

    cl ..\..\apps\ospTutorial.cpp /EHsc -I ..\..\ospray\include -I ..\..  ^
      -I ..\..\ospray\embree\common ospray.lib

Running `ospTutorial` will create two images of two triangles, rendered
with the Scientific Visualization renderer with full Ambient Occlusion.
The first image `firstFrame.ppm` shows the result after one call to
`ospRenderFrame` -- jagged edges and noise in the shadow can be seen.
Calling `ospRenderFrame` multiple times enables progressive refinement,
resulting in antialiased edges and converged shadows, shown after ten
frames in the second image `accumulatedFrames.png`.

![First frame.][imgTutorial1]

![After accumulating ten frames.][imgTutorial2]


Qt Viewer
---------

OSPRay also includes a demo viewer application `ospQtViewer`, showcasing all features
of OSPRay.

![Screenshot of `ospQtViewer`.][imgQtViewer]


Volume Viewer
-------------

Additionally, OSPRay includes a demo viewer application
`ospVolumeViewer`, which is specifically tailored for volume rendering.

![Screenshot of `ospVolumeViewer`.][imgVolumeViewer]


Demos
-----

Several ready-to-run demos, models and data sets for OSPRay can be found
at the [OSPRay Demos and Examples] page.

[news/updates]: https://ospray.github.io/news.html
[getting OSPRay]: https://ospray.github.io/getting_ospray.html
[OSPRay Demos and Examples]: https://ospray.github.io/demos.html
[imgTutorial1]: https://ospray.github.io/images/tutorial_firstframe.png
[imgTutorial2]: https://ospray.github.io/images/tutorial_accumulatedframe.png
[imgQtViewer]: https://ospray.github.io/images/QtViewer.jpg
[imgVolumeViewer]: https://ospray.github.io/images/VolumeViewer.png
