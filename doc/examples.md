Examples
========

Simple Tutorial
---------------

A minimal working example demonstrating how to use OSPRay can be found
at `apps/tutorials/ospTutorial.c`^[A C++ version that uses the C++ convenience
wrappers of OSPRay's C99 API via `include/ospray/ospray_cpp.h` is
available at `apps/tutorials/ospTutorial.cpp`.].

An example of building `ospTutorial.c` with CMake can be found in
`apps/tutorials/ospTutorialFindospray/`.

To build the tutorial on Linux, build it in a build directory with

    gcc -std=c99 ../apps/ospTutorial.c -I ../ospray/include -I .. \
    ./libospray.so -Wl,-rpath,. -o ospTutorial

On Windows build it can be build manually in a
"build_directory\\$Configuration" directory with

    cl ..\..\apps\ospTutorial.c -I ..\..\ospray\include -I ..\.. ospray.lib

Running `ospTutorial` will create two images of two triangles, rendered
with the Scientific Visualization renderer with full Ambient Occlusion.
The first image `firstFrame.ppm` shows the result after one call to
`ospRenderFrame` – jagged edges and noise in the shadow can be seen.
Calling `ospRenderFrame` multiple times enables progressive refinement,
resulting in antialiased edges and converged shadows, shown after ten
frames in the second image `accumulatedFrames.ppm`.

![First frame.][imgTutorial1]

![After accumulating ten frames.][imgTutorial2]


Mini-App Tutorials
------------------

OSPRay also ships various mini-apps to showcase OSPRay features. These
apps are all prefixed with `ospTutorial` and can be found in the
`apps/tutorials/` directory of the OSPRay source tree.
