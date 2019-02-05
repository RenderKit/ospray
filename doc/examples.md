Examples
========

Simple Tutorial
---------------

A minimal working example demonstrating how to use OSPRay can be found
at `apps/ospTutorial.c`^[A C++ version that uses the C++ convenience
wrappers of OSPRay's C99 API via `include/ospray/ospray_cpp.h` is
available at `apps/ospTutorial.cpp`.]. On Linux build it in the build
directory with

    gcc -std=c99 ../apps/ospTutorial.c -I ../ospray/include -I .. \
    ./libospray.so -Wl,-rpath,. -o ospTutorial

On Windows build it in the "build_directory\\$Configuration" with

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
`tutorials/` directory of the OSPRay source tree. Current tutorials
include:

- structured volumes
- unstructured volumes
- spheres
- animated spheres
- subdivision surfaces

More apps will be created in future releases to further demonstrate
other interesting OSPRay features.


Example Viewer
--------------

![Screenshot of using ospExampleViewer with a scene graph.][imgExampleViewer]

OSPRay includes an exemplary viewer application `ospExampleViewer`,
showcasing most features of OSPRay which can be run as
`./ospExampleViewer [options] <filename>`. The Example Viewer uses the
ImGui library for user interface controls and is based on a prototype
OSPRay [scene graph] interface where nodes can be viewed and edited
interactively. Updates to scene graph nodes update OSPRay state
automatically through the scene graph viewer which is enabled by
pressing 'g'.

### Exploring the Scene

The GUI shows the entire state of the program under the root scene graph
node. Expanding nodes down to explore and edit the scene is possible,
for example a material parameter may be found under
renderer→world→mesh→material→Kd. Updates to values will be automatically
propagated to the next render. Individual nodes can be easily found
using the "Find Node" section, which will find nodes with a given name
based on the input string. Scene objects can also be selected with the
mouse by shift-left clicking in the viewer.

Click on nodes to expand their children, whose values can be set by
dragging or double clicking and typing in values. You can also add new
nodes where appropriate: for example, when "lights" is expanded right
clicking on "lights" and selecting create new node and typing in a light
type, such as "PointLight", will add it to the scene. Similarly, right
clicking on "world" and creating an "Importer" node will add a new scene
importer from a file. Changing the filename to an appropriate file will
load the scene and propagate the resulting state. Exporting and
importing the scene graph is only partially supported at the moment
through "ospsg" files. Currently, any nodes with Data members will break
this functionality, however right clicking and selecting export on the
camera or lights nodes for instance will save out their respective state
which can be imported on the command line. The Example Viewer also
functions as an OSPRay state debugger – invalid values will be shown in
red up the hierarchy and will not change the viewer until corrected.

### Volume Rendering

Volumes are loaded into the viewer just as a mesh is. Volume appearance
is modified according to the transfer function, which will show up in a
popup window on the GUI after pressing 'g'. Click and drag across the
transfer function to set opacity values, and selecting near the bottom
of the editable transfer function widget sets the opacity to zero. The
colors themselves can only be modified by selecting from the dropdown
menu 'ColorMap' or importing and exporting json colors. The range that
the transfer function operates on can be modified on the scene graph
viewer.

### Controls
* 'g' - toggle scene graph display
* 'q' - quit
* Left click and drag to rotate
* Right click and drag or mouse wheel to zoom in and out.
* Mouse-Wheel click will pan the camera.
* Control-Left clicking on an object will select a model and all of its
  children which will be displayed in the
* Shift-Left click on an object will zoom into that part of the scene
  and set the focal distance.

### Command Line Options

* Running `./ospExampleViewer -help` will bring up a list of
  command line options. These options allow you to load files, run
  animations, modify any scene graph state, and many other functions. See
  the [demos] page for examples.
* Supported file importers currently include: `obj`, `ply`, `x3d`,
  `vtu`, `osp`, `ospsg`, `xml` (rivl), `points`, `xyz`.

### Denoiser

When the example viewer is built with OpenImageDenoise, the denoiser is
automatically enabled when running the application. It can be toggled
on/off at runtime via the `useDenoiser` GUI parameter found under the
framebuffer in the scene graph.


Distributed Viewer
------------------

The application `ospDistribViewerDemo` demonstrates how to write a
distributed SciVis style interactive renderer using the distributed MPI
device. Note that because OSPRay uses sort-last compositing it is up to
the user to ensure that the data distribution across the nodes is
suitable. Specifically, each nodes' data must be convex and disjoint.
This renderer supports multiple volumes and geometries per node. To
ensure they are composited correctly you specify a list of bounding
regions to the model, within these regions can be arbitrary
volumes/geometries and each rank can have as many regions as needed. As
long as the regions are disjoint/convex the data will be rendered
correctly. In this demo we either generate a volume, or load a RAW
volume file if one is passed on the command line.

### Loading a RAW Volume

To load a RAW volume you must specify the filename (`-f <file>`), the
data type (`-dtype <dtype>`), the dimensions (`-dims <x> <y> <z>`) and the
value range for the transfer function (`-range <min> <max>`). For example,
to run on the [CSAFE data set from the demos
page](http://www.ospray.org/demos.html#csafe-heptane-gas-dataset) you
would pass the following arguments:

    mpirun -np <n> ./ospDistribViewerDemo \
        -f <path to csafe>/csafe-heptane-302-volume.raw \
        -dtype uchar -dims 302 302 302 -range 0 255

The volume file will then be chunked up into an `x×y×z` grid such that
$n = xyz$. See `loadVolume` in [gensv/generateSciVis.cpp] for an
example of how to properly load a volume distributed across ranks with
correct specification of brick positions and ghost voxels for
interpolation at boundaries. If no volume file data is passed a volume
will be generated instead, in that case see `makeVolume`.

### Geometry

The viewer can also display some randomly generated sphere geometry if
you pass `-spheres <n>` where `n` is the number of spheres to generate
per-node. These spheres will be generated inside the bounding box of the
region's volume data.

In the case that you have geometry crossing the boundary of nodes and
are replicating it on both nodes to render (ghost zones, etc.) the
region will be used by the renderer to clip rays against allowing to
split the object between the two nodes, with each rendering half. This
will keep the regions rendered by each rank disjoint and thus avoid any
artifacts. For example, if a sphere center is on the border between two
nodes, each would render half the sphere and the halves would be
composited to produce the final complete sphere in the image.

### App-initialized MPI

Passing the `-appMPI` flag will have the application initialize MPI
instead of letting OSPRay do it internally when creating the MPI
distributed device. In this case OSPRay will not finalize MPI when
cleaning up the device, allowing the application to use OSPRay for some
work, shut it down and recreate everything later if needed for
additional computation, without accidentally shutting down its MPI
communication.

### Interactive Viewer

Rank 0 will open an interactive window with GLFW and display the
rendered image. When the application state needs to update (e.g., camera
or transfer function changes), this information is broadcasted out to
the other nodes to update their scene data.

Demos
-----

Several ready-to-run demos, models and data sets for OSPRay can be found
at the [OSPRay Demos and Examples] page.
