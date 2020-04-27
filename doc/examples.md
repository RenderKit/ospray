Examples
========

Tutorial
--------

A minimal working example demonstrating how to use OSPRay can be found
at `apps/tutorials/ospTutorial.c`^[A C++ version that uses the C++
convenience wrappers of OSPRay's C99 API via
`include/ospray/ospray_cpp.h` is available at
`apps/tutorials/ospTutorial.cpp`.].

An example of building `ospTutorial.c` with CMake can be found in
`apps/tutorials/ospTutorialFindospray/`.

To build the tutorial on Linux, build it in a build directory with

    gcc -std=c99 ../apps/ospTutorial/ospTutorial.c \
    -I ../ospray/include -L . -lospray -Wl,-rpath,. -o ospTutorial

On Windows build it can be build manually in a
"build_directory\\$Configuration" directory with

    cl ..\..\apps\ospTutorial\ospTutorial.c -I ..\..\ospray\include -I ..\.. ospray.lib

Running `ospTutorial` will create two images of two triangles, rendered
with the Scientific Visualization renderer with full Ambient Occlusion.
The first image `firstFrame.ppm` shows the result after one call to
`ospRenderFrame` – jagged edges and noise in the shadow can be seen.
Calling `ospRenderFrame` multiple times enables progressive refinement,
resulting in antialiased edges and converged shadows, shown after ten
frames in the second image `accumulatedFrames.ppm`.

![First frame.][imgTutorial1]

![After accumulating ten frames.][imgTutorial2]


ospExamples
-----------

Apart from tutorials, `OSPRay` comes with a C++ app called `ospExamples`
which is an elaborate easy-to-use tutorial, with a single interface to
try various `OSPRay` features. It is aimed at providing users with
multiple simple scenes composed of basic geometry types, lights, volumes
etc. to get started with OSPRay quickly.

`ospExamples` app runs a `GLFWOSPRayWindow` instance that manages
instances of the camera, framebuffer, renderer and other OSPRay objects
necessary to render an interactive scene. The scene is rendered on a
`GLFW` window with an `imgui` GUI controls panel for the user to
manipulate the scene at runtime.

The application is located in `apps/ospExamples/` directory and can be
built with CMake. It can be run from the build directory via:
```
./ospExamples <command-line-parameter>
```
The command line parameter is optional however.

![`ospExamples` application with default `boxes` scene.][ospExamples]

### Scenes

Different scenes can be selected from the `scenes` dropdown and each
scene corresponds to an instance of a special `detail::Builder` struct.
Example builders are located in `apps/common/ospray_testing/builders/`.
These builders provide a usage guide for the OSPRay scene hierarchy and
OSPRay API in the form of `cpp` wrappers. They instantiate and manage
objects for the specific scene like `cpp::Geometry`, `cpp::Volume`,
`cpp::Light` etc.

The `detail::Builder` base struct is mostly responsible for setting up
OSPRay `world` and objects common in all scenes (for example lighting
and ground plane), which can be conveniently overridden in the derived
builders.

Given below are different scenes listed with their string identifiers:

boxes
: A simple scene with `box` geometry type.

cornell_box
: A scene depicting a classic cornell box with `quad` mesh geometry type
for rendering two cubes and a `quad` light type.

curves
: A simple scene with `curve` geometry type and options to change
`curveBasis`. For details on different basis' please check documentation
of [curves].

gravity_spheres_volume
: A scene with `structuredRegular` type of [volume].

gravity_spheres_isosurface
: A scene depicting iso-surface rendering of `gravity_spheres_volume`
using geometry type `isosurface`.

perlin_noise_volumes
: An example scene with `structuredRegular` volume type depicting perlin
noise.

random_spheres
: A simple scene depicting `sphere` geometry type.

streamlines
: A scene showcasing streamlines geometry derived from `curve` geometry
type.

subdivision_cube
: A scene with a cube of `subdivision` geometry type to showcase
subdivision surfaces.

unstructured_volume
: A simple scene with a volume of `unstructured` volume type.

### Renderer

This app comes with three [renderer] options: `scivis`, `pathtracer` and
`debug`. The app provides some common rendering controls like
`pixelSamples` and other more specific to the renderer type like
`aoIntensity` for `scivis` renderer.

The sun-sky lighting can be used in a sample scene by enabling the
`renderSunSky` option of the `pathtracer` renderer. It allows the user
to change `turbidity` and `sunDirection`. 

![Rendering an evening sky with the `renderSunSky` option.][renderSunSky]
