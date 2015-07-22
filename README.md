% OSPRay: An Open, Scalable, Parallel, Ray Tracing Based Rendering Engine for High-Fidelity Visualization 0.8.2
% Intel Corporation

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
or Intel® Xeon Phi™ to achieve high rendering performance.


OSPRay Support and Contact
--------------------------

OSPRay is still in "pre 1.0" alpha stage, and though we do our best to
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
repository](http://github.com/ospray/ospray). The default "master"
branch should always point to the latest tested bugfix release.

Prerequisites
-------------

OSPRay currently supports both Linux and Mac\ OS\ X (a Windows version
will soon follow). In addition, before you can build OSPRay you need the
following prerequisites:

-   You can clone the latest OSPRay sources via:

        git clone https://github.com/ospray/ospray.git

-   To build OSPRay you require a copy of the [Intel® SPMD Program
    Compiler (ISPC)](http://ispc.github.io). Please obtain a copy of the
    latest binary release of ISPC (currently 1.8.2) from the [ISPC
    downloads page](https://ispc.github.io/downloads.html), and place it
    right "next to" the checked-out OSPRay sources in your directory
    tree (e.g., if OSPRay is in `~/Projects/ospray`, ISPC should be in
    `~/Projects/ispc-v1.8.2-linux`).
-   Additional packages you need are [CMake](http://www.cmake.org), any
    form of C++ compiler (we recommend using the [Intel® C++ compiler
    (icc)](https://software.intel.com/en-us/c-compilers), but also
    support GCC and clang-cc), and standard Linux development tools.
    To build the demo viewers, you should also have some version of
    OpenGL and the GL Utility Toolkit (GLUT or freeglut).
-   OSPRay also heavily uses [Embree](http://embree.github.io); however,
    OSPRay directly includes its own copy of Embree, so a special
    installation of Embree is *not* required.

Compiling OSPRay
----------------

Assume the above requisites are all fulfilled, building OSPRay through
CMake is easy:

-   Create a build directory, and go into it

        user@mymachine[~/Projects]: mkdir ospray/release
        user@mymachine[~/Projects]: cd ospray/release

    (We do recommend having separate build directories for different
    configurations such as release, debug, etc).
-   Open the CMake configuration dialog

        user@mymachine[~/Projects/ospray/release]: ccmake ..

-   Make sure to properly set build mode, desired compiler, enable the
    components you need, etc; then type 'c'onfigure and 'g'enerate. When
    back on the command prompt, build it using

        user@mymachine[~/Projects/ospray/release]: make

-   You should now have `libospray.so` as well as a set of sample
    viewers. You can test your version of OSPRay using any of the
    examples on the [OSPRay Demos and Examples](demos.html) page.

Examples
========

Tutorial
--------

A minimal working example demonstrating how to use OSPRay can be found
at `apps/ospTutorial.cpp`. Build it in the build_directory with

    g++ ../apps/ospTutorial.cpp -I ../ospray/include -I .. -I ../ospray/embree/common  ./libospray.so -o ospTutorial

Running `ospTutorial` will create two images of two triangles, rendered
with the Ambient Occlusion renderer. The first image `firstFrame.ppm` shows the
result after one call to `ospRenderFrame` -- jagged edges and noise in the
shadow can be seen. Calling `ospRenderFrame` multiple times enables
progressive refinement, resulting in antialiased edges and converged
shadows, shown after ten frames in the second image
`accumulatedFrames.png`.

![First frame.](images/tutorial_firstframe.png)

![After accumulating ten frames.](images/tutorial_accumulatedframe.png)


QT Viewer
---------

OSPRay also includes a demo viewer application `ospQTViewer`, showcasing all features
of OSPRay.

![Screenshot of `ospQTViewer`.](images/QTViewer.jpg)

[news/updates]: https://ospray.github.io/news.html
[getting OSPRay]: https://ospray.github.io/getting_ospray.html
