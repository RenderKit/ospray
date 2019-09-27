# OSPRay 2.0.0 Porting Guide

OSPRay 2.0.0 introduces a number of new features and updates, as well as some
API changes.

This guide is intended as an introduction to the new features and the concepts
behind them, and as a guide to porting applications using OSPRay 1.8.x to
2.0.0.

## New Object Hierarchy

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
    ospSetData(group, "geometry", geometricModels);
    ospCommit(group);

    // put the group in an instance
    OSPInstance = ospNewInstance(group);
    ospCommit(instance);

    // put the instance in the world
    OSPWorld world = ospNewWorld();
    OSPData instances = ospNewSharedData1D(&instance, OSP_INSTANCE, 1);
    ospSetData(world, "instance", instances);
    ospCommit(world);

While this looks more complex at first, the new hierarchy structure
provides more fine control over transformations and instances.

### `OSPGeometry` and `OSPVolume`

`OSPGeometry` and `OSPVolume` contain actual data in an OSPRay container. For
example, a triangle mesh geometry contains vertex positions and normals, among
other parameters.

### `OSPGeometricModel` and `OSPVolumetricModel`

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

### `OSPGroup`

`OSPGroup` objects contain zero or more `OSPGeometricModel` and
`OSPVolumetricModel` objects.  They can hold geometries and volumes
simultaneously.

This is useful to collect together any objects that are logically grouped
together in the scene.

### `OSPInstance`

`OSPInstance` contains transformation information on an `OSPGroup` object. It
has a one-to-one relationship with `OSPGroup`.

This means that each `OSPInstance` contains exactly one `OSPGroup`. However, a
single `OSPGroup` may be placed into multiple `OSPInstace` objects. This allows
for _instancing_ of multiple objects throughout the scene, each with different
transformations.

`OSPInstance` objects can hold an affine transformation matrix that is applied
to all geometries and/or volumes in its group.

### `OSPWorld`

`OSPWorld` is the final container for all `OSPInstance` objects. It can contain
one or more instances.  The world is passed along with a renderer, camera, and
framebuffer to `ospRenderFrame` to generate an image.

## Updated Public Parameter Names

OSPRay 2.0.0 has updated public parameter names (the strings used in
`ospSetData`, for example) to a more consistent naming convention.  OSPRay now
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

OSPRay will print warnings for parameters that were not used by the object.
This should aid in the transition to the new parameter names.
