# OSPRay 2.0.0 Porting Guide

OSPRay 2.0.0 introduces a number of new features and updates, as well as some
API changes.

This guide is intended as an introduction to the new features and the concepts
behind them, and as a guide to porting applications using OSPRay 1.8.x to
2.0.0.

# <span style="color:red">**TODO**</span>
- c++ wrappers not have to worry about release/retain
- snippet from ospTutorial.c vs .cpp

## Objects

### New Object Hierarchy

OSPRay objects, such as `OSPGeometry` and `OSPVolume`, have a new hierarchy
that affords more control over geometry and volume transformation and
instancing in the scene.

Previously, the workflow was to create an object, fill it with the necessary
parameters, and then place the object into an `OSPModel` via, for example,
`ospAddGeometry`. An example is shown below using the C API.

    OSPGeometry mesh = ospNewGeometry("triangles");
    // set parameters on mesh
    ospCommit(mesh);

    OSPModel world = ospNewModel();
    ospAddGeometry(world, mesh);
    ospRelease(mesh);
    ospCommit(world);

In OSPRay 2.0.0, there is now an `OSPWorld`, which effectively replaces the old
`OSPModel`.  In addition, there are now 3 new objects that exist "in between"
the geometry and the world: `OSPGeometricModel`, `OSPGroup`, and `OSPInstance`.
There is also an `OSPVolumetricModel` equivalent for working with `OSPVolume`
objects.

The new workflow is shown below. Note that calls to `ospRelease()` have been
removed for brevity.

    // create a geometry
    OSPGeometry mesh = ospNewGeometry("triangles");
    // set parameters on mesh
    ospCommit(mesh);

    // put the geometry in a geometric model
    OSPGeometricModel model = ospNewGeometricModel(mesh);
    ospCommit(model);

    // put the geometric model(s) in a group
    OSPGroup group = ospNewGroup();
    OSPData geometricModels = ospNewSharedData1D(&model, OSP_GEOMETRIC_MODEL, 1);
    ospSetObject(group, "geometry", geometricModels);
    ospCommit(group);

    // put the group in an instance
    OSPInstance = ospNewInstance(group);
    ospCommit(instance);

    // put the instance in the world
    OSPWorld world = ospNewWorld();
    OSPData instances = ospNewSharedData1D(&instance, OSP_INSTANCE, 1);
    ospSetObject(world, "instance", instances);
    ospCommit(world);


While this looks more complex at first, the new hierarchy structure
provides more fine control over transformations and instances.

#### OSPGeometry and OSPVolume

`OSPGeometry` and `OSPVolume` contain actual data in an OSPRay container. For
example, a triangle mesh geometry contains vertex positions and normals, among
other parameters.

#### OSPGeometricModel and OSPVolumetricModel

`OSPGeometricModel` and `OSPVolumetricModel` contain appearance information
about the geometry or volume that they hold. They have a one-to-one
relationship with `OSPGeometry` and `OSPVolume` objects.

This means that each `OSPGeometricModel` contains exactly one `OSPGeometry`.
However, a single `OSPGeometry` may be put into multiple `OSPGeometricModel`
objects. This could be used to create multiple copies of a geometry with
different materials, for example.

`OSPGeometricModel`s can hold primitive color information (e.g. the color used
for each sphere in a `Spheres` geometry), a material or list of materials, and
primitive material IDs (i.e. indexes into the list of materials if it is used).

`OSPVolumetricModel`s can hold transfer functions.

#### OSPGroup

`OSPGroup` objects contain zero or more `OSPGeometricModel` and
`OSPVolumetricModel` objects.  They can hold geometries and volumes
simultaneously.

This is useful to collect together any objects that are logically grouped
together in the scene.

#### OSPInstance

`OSPInstance` contains transformation information on an `OSPGroup` object. It
has a one-to-one relationship with `OSPGroup`.

This means that each `OSPInstance` contains exactly one `OSPGroup`. However, a
single `OSPGroup` may be placed into multiple `OSPInstace` objects. This allows
for _instancing_ of multiple objects throughout the scene, each with different
transformations.

`OSPInstance` objects can hold an affine transformation matrix that is applied
to all geometries and/or volumes in its group.

#### OSPWorld

`OSPWorld` is the final container for all `OSPInstance` and `OSPLight` objects.
It can contain one or more instances and lights.  The world is passed along with
a renderer, camera, and framebuffer to `ospRenderFrame` to generate an image.

### Object Usage Simplification
<!-- XXX BMC: Only true if issue !418 is merged into the release -->

To simplify setting data onto an object; wherever an array of `type[]` is
expected, it is now permissible to pass a single data item of `type`, as well.
This permits the user to assign a single item without first creating a data
object containing that item.

For example:

    OSPGroup group = ospNewGroup();
    OSPData geometricModels = ospNewSharedData1D(&model, OSP_GEOMETRIC_MODEL, 1);
    ospSetObject(group, "geometry", geometricModels);

simply becomes:

    OSPGroup group = ospNewGroup();
    ospSetObject(group, "geometry", model);

### OSPDataType type checking

Where it was previously accepted to create data of generic type `OSP_OBJECT` to
represent a list of any object type, the `OSPDataType` must now match the
object(s) it contains.

### void ospRetain(OSPObject)

To allow the user greater control over the lifetime of objects, a new API
`ospRetain` has been introduced.  This call increments an object's reference
count, and can delay automatic deletion.

## Updated Public Parameter Names

OSPRay 2.0.0 has updated public parameter names (the strings used in
`ospSetParam`) to a more consistent naming convention.  OSPRay now
will print a warning (visible if debug logs are enabled) if a parameter
provided by the user is not used by an object. This can help catch cases where
applications are using parameter names from OSPRay 1.8.5 or mistyped names.
Some objects have required parameters. In these cases, OSPRay will terminate
with an error message indicating which object and which parameter.

Below is a table of updated objects, their old parameter names, and the updated
equivalent names. In some cases, parameters have changed from taking a string
identifying an option to an enumerated value. The available options for these
values are listed.

<table>
<!-- ALOK: need to identify parameters that are enums now -->
  <tr>
    <th>Object</th> <th>Old parameter name</th> <th>New parameter name</th> <th>Enum values (if applicable)</th>
  </tr>

  <tr>
    <td rowspan="2">Camera</td> <td>pos</td> <td>position</td> <td></td>
  </tr>
  <tr>
    <td>dir</td> <td>direction</td> <td></td>
  </tr>

  <tr>
    <td rowspan="4">PerspectiveCamera</td> <td rowspan="4">stereoMode</td> <td rowspan="4">stereoMode</td> <td>OSP_STEREO_NONE (default)</td>
  </tr>
  <tr>
    <td>OSP_STEREO_LEFT</td>
  </tr>
  <tr>
    <td>OSP_STEREO_RIGHT</td>
  </tr>
  <tr>
    <td>OSP_STEREO_SIDE_BY_SIDE</td>
  </tr>

  <tr>
    <td>Boxes</td> <td>boxes</td> <td>box</td> <td></td>
  </tr>

  <tr>
    <td rowspan="8">Curves</td> <td>vertex</td> <td>vertex.position</td> <td></td>
  </tr>
  <tr>
    <td rowspan="4">curveBasis</td> <td rowspan="4">basis</td> <td>OSP_LINEAR</td>
  </tr>
  <tr>
    <td>OSP_BEZIER</td>
  </tr>
  <tr>
    <td>OSP_BSPLINE</td>
  </tr>
  <tr>
    <td>OSP_HERMITE</td>
  </tr>
  <tr>
    <td rowspan="3">curveType</td> <td rowspan="3">type</td> <td>OSP_ROUND</td>
  </tr>
  <tr>
    <td>OSP_FLAT</td>
  </tr>
  <tr>
    <td>OSP_RIBBON</td>
  </tr>

  <tr>
    <td>Isosurfaces</td> <td>isovalues</td> <td>isovalue</td> <td></td>
  </tr>

  <tr>
    <td>QuadMesh</td> <td>vertex</td> <td>vertex.position</td> <td></td>
  </tr>

  <tr>
    <td>Slices</td> <td>planes</td> <td>plane</td> <td></td>
  </tr>

  <tr>
    <td>StreamLines</td> <td>vertex</td> <td>vertex.position</td> <td></td>
  </tr>

  <tr>
    <td>Subdivision</td> <td>vertex</td> <td>vertex.position</td> <td></td>
  </tr>

  <tr>
    <td>TriangleMesh</td> <td>vertex</td> <td>vertex.position</td> <td></td>
  </tr>

  <tr>
    <td>Light</td> <td>isVisible</td> <td>visible</td> <td></td>
  </tr>

  <tr>
    <td rowspan="2">PathTracer</td> <td>useGeometryLights</td> <td>geometryLights</td> <td></td>
  </tr>
  <tr>
    <td>lights</td> <td>light</td> <td></td>
  </tr>

  <tr>
    <td rowspan="2">LinearTransferFunction</td> <td>colors</td> <td>color</td> <td></td>
  </tr>
  <tr>
    <td>opacities</td> <td>opacity</td> <td></td>
  </tr>

  <tr>
    <td rowspan="7">AMRVolume</td> <td rowspan="3">amrMethod</td> <td rowspan="3">method</td> <td>OSP_AMR_CURRENT (default)</td>
  </tr>
  <tr>
    <td>OSP_AMR_FINEST</td>
  </tr>
  <tr>
    <td>OSP_AMR_OCTANT</td>
  </tr>
  <tr>
    <td>blockBounds</td> <td>block.bounds</td> <td></td>
  </tr>
  <tr>
    <td>refinementLevels</td> <td>block.level</td> <td></td>
  </tr>
  <tr>
    <td>cellWidths</td> <td>block.cellWidth</td> <td></td>
  </tr>
  <tr>
    <td>blockData</td> <td>block.data</td> <td></td>
  </tr>

  <tr>
    <td rowspan="5">StructuredVolume</td> <td rowspan="5">voxelType</td> <td rowspan="5">voxelType</td> <td>OSP_UCHAR</td>
  </tr>
  <tr>
    <td>OSP_SHORT</td>
  </tr>
  <tr>
    <td>OSP_USHORT</td>
  </tr>
  <tr>
    <td>OSP_FLOAT</td>
  </tr>
  <tr>
    <td>OSP_DOUBLE</td>
  </tr>

  <tr>
    <td rowspan="3">UnstructuredVolume</td> <td>vertex</td> <td>vertex.position</td> <td></td>
  </tr>
  <tr>
    <td rowspan="2">hexMethod</td> <td rowspan="2">hexMethod</td> <td>OSP_FAST</td>
  </tr>
  <tr>
    <td>OSP_ITERATIVE</td>
  </tr>
</table>

* **OSPRay will print warnings for parameters that were not used by the object, 
which should aid in the transition to the new parameter names.  To use this
feature, enable OSPRay debug (see api documentation) and search the output log
for "found unused parameter" messages.**

## ospRenderFrame

`ospRenderFrame` has changed in two ways.  The signature has changed from:

    float ospRenderFrame(OSPFrameBuffer, OSPRenderer,
                         const uint32_t frameBufferChannels = OSP_FB_COLOR);

to

    OSPFuture ospRenderFrame(OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld);

And, notably, it is no longer blocking.  Two new calls `ospIsReady`, and
`ospWait` are available to manage synchronization.

## Utility Library

The core OSPRay API has been simplified by removing many of the type
specializations from the data and parameter set calls.  Additionally, as
mentioned above, `ospRenderFrame` is now asynchronous.

As a convenience, a lightweight utility library has been provided to help users
port from the previous versions.

### OSPData and Parameter helpers

A single `ospSetParam` API replaces the entire `ospSet*` interface.  The utility
library provides wrappers to the familiar calls listed below:

    ospSetString(OSPObject, const char *n, const char *s);
    ospSetObject(OSPObject, const char *n, OSPObject obj);

    ospSetBool(OSPObject, const char *n, int x);
    ospSetFloat(OSPObject, const char *n, float x);
    ospSetInt(OSPObject, const char *n, int x);

    ospSetVec2f(OSPObject, const char *n, float x, float y);
    ospSetVec3f(OSPObject, const char *n, float x, float y, float z);
    ospSetVec4f(OSPObject, const char *n, float x, float y, float z, float w);

    ospSetVec2i(OSPObject, const char *n, int x, int y);
    ospSetVec3i(OSPObject, const char *n, int x, int y, int z);
    ospSetVec4i(OSPObject, const char *n, int x, int y, int z, int w);

    ospSetObjectAsData(OSPObject, const char *n, OSPDataType type, OSPObject obj);

Convenience wrappers have also been provided to specialize `ospNewData`, and the
new `ospNewSharedData` and `ospCopyData` APIs.

    ospNewSharedData1D(const void *sharedData, OSPDataType type, uint32_t numItems);
    ospNewSharedData1DStride(const void *sharedData, OSPDataType type, uint32_t numItems, int64_t byteStride);
    ospNewSharedData2D(const void *sharedData, OSPDataType type, uint32_t numItems1, uint32_t numItems2);
    ospNewSharedData2DStride(const void *sharedData, OSPDataType type, uint32_t numItems1, int64_t byteStride1, uint32_t numItems2, int64_t byteStride2);
    ospNewSharedData3D(const void *sharedData, OSPDataType type, uint32_t numItems1, uint32_t numItems2, uint32_t numItems3);

    ospNewData1D(OSPDataType type, uint32_t numItems);
    ospNewData2D(OSPDataType type, uint32_t numItems1, uint32_t numItems2);

    ospCopyData1D(const OSPData source, OSPData destination, uint32_t destinationIndex);
    ospCopyData2D(const OSPData source, OSPData destination, uint32_t destinationIndex1, uint32_t destinationIndex2);


### Rendering helpers

While `ospRenderFrame` is now asynchronous, some users will prefer the original
blocking behavior that returns a frame variance.  The utility library provides a
wrapper to this functionality:

    float ospRenderFrameBlocking(OSPFrameBuffer fb,
                                 OSPRenderer renderer,
                                 OSPCamera camera,
                                 OSPWorld world)


