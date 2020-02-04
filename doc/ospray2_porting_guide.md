OSPRay v2.0 Porting Guide
=========================

OSPRay v2.0.0 introduces a number of new features and updates, as well
as some API changes. This guide is intended as an introduction to the
new features and the concepts behind them, and as a guide to porting
applications using OSPRay v1.8.x to v2.0.0.

Parameters
----------

### Setting parameters

OSPRay traded having a limited number of parameter types (with a unique
function signature for each type) by instead having a single generic
function that takes any type found in the `OSPDataType` enum. The new
function signature is as follows:

    void ospSetParam(OSPObject, const char *name, OSPDataType type, const void *mem)

This function takes the address of the value being set, where care
should be taken when setting pointer-based types. For example, consider
the following C-array:

    float myValues[] = {0.f, 1.f, 2.f}

If the application wants to set these values as an OSP_VEC3F, note that
the `const void *mem` parameter to `ospSetParam` should point to what is
the address of what would be a `vec3f`. Thus either of the following are
valid:

    void ospSetParam(object, "some_parameter", OSP_VEC3F, &myValues[0]);

or

    void ospSetParam(object, "some_parameter", OSP_VEC3F, myValues);

The second variant relies on implicit casting from `float[]` to
`float *`, which will point to the address of the first value. This then
gets casted to a `vec3f` to be set on the target `object`.

Some of the `ospSet*` functions were preserved as utility wrapper
functions outlined later in this document (and can be found in
`ospray_util.h`).

### OSPDataType type checking

Where it was previously accepted to create data of generic type
`OSP_OBJECT` to represent a list of any object type, the `OSPDataType`
must now match the object(s) it contains.

Objects
-------

### New Object Hierarchy

OSPRay objects, such as `OSPGeometry` and `OSPVolume`, have a new
hierarchy that affords more control over geometry and volume
transformation and instancing in the scene.

Previously, the workflow was to create an object, fill it with the
necessary parameters, and then place the object into an `OSPModel` via,
for example, `ospAddGeometry`. An example is shown below using the C
API.

    OSPGeometry mesh = ospNewGeometry("triangles");
    // set parameters on mesh
    ospCommit(mesh);

    OSPModel world = ospNewModel();
    ospAddGeometry(world, mesh);
    ospRelease(mesh);
    ospCommit(world);

In OSPRay v2.0.0, there is now an `OSPWorld`, which effectively replaces
the old `OSPModel`. In addition, there are now 3 new objects that exist
"in between" the geometry and the world: `OSPGeometricModel`,
`OSPGroup`, and `OSPInstance`. There is also an `OSPVolumetricModel`
equivalent for working with `OSPVolume` objects.

The new workflow is shown below. Note that calls to `ospRelease()` have
been removed for brevity.

    // create a geometry
    OSPGeometry mesh = ospNewGeometry("triangles");
    // set parameters on mesh
    ospCommit(mesh);

    // put the geometry in a model (a geometry can exist in more than one model)
    OSPGeometricModel model = ospNewGeometricModel(mesh);
    ospCommit(model);

    // put the geometric model(s) in a group
    OSPGroup group = ospNewGroup();
    OSPData geometricModels = ospNewSharedData1D(&model, OSP_GEOMETRIC_MODEL, 1);
    ospSetObject(group, "geometry", geometricModels);
    ospCommit(group);

    // put the group in an instance (a group can be instanced more than once)
    OSPInstance = ospNewInstance(group);
    ospCommit(instance);

    // put the instance in the world
    OSPWorld world = ospNewWorld();
    OSPData instances = ospNewSharedData1D(&instance, OSP_INSTANCE, 1);
    ospSetObject(world, "instance", instances);
    ospCommit(world);

While this looks more complex at first, the new hierarchy structure
provides more fine control over appearance information and instance
transformations.

In OSPRay v1.x, geometries and volumes contained both structural and
appearance information which limited their reuse in other objets. For
example, the volume's transfer function can now be different between an
isosurface, slice, and rendered volume all in the same scene without
duplicating the actual volume itself.

#### OSPGeometry and OSPVolume

`OSPGeometry` and `OSPVolume` contain the physical data represented by
the object. For geometries, this is the intersectable surface. For
volumes, it is the scalar field to be sampled.

#### OSPGeometricModel and OSPVolumetricModel

`OSPGeometricModel` and `OSPVolumetricModel` contain appearance
information about the geometry or volume that they hold. They have a
one-to-N relationship with `OSPGeometry` and `OSPVolume` objects (i.e. a
geometry or volume can exist in more than one model), but commonly exist
as one-to-one. This could be used to create multiple copies of a
geometry with different materials, for example.

`OSPGeometricModel`s can hold primitive color information (e.g. the
color used for each sphere in a `Spheres` geometry), a material or list
of materials, and primitive material IDs (i.e. indexes into the list of
materials if it is used).

`OSPVolumetricModel`s can hold transfer functions.

#### OSPGroup

`OSPGroup` objects contain zero or more `OSPGeometricModel` and
`OSPVolumetricModel` objects. They can hold geometries and volumes
simultaneously.

This is useful to collect together any objects that are logically
grouped together in the scene.

#### OSPInstance

`OSPInstance` contains transformation information on an `OSPGroup`
object. It has a one-to-one relationship with `OSPGroup`.

This means that each `OSPInstance` contains exactly one `OSPGroup`.
Similar to models and groups, a single `OSPGroup` may be placed into
multiple `OSPInstace` objects. This allows for *instancing* of multiple
objects throughout the scene, each with different transformations.

`OSPInstance` objects holds an affine transformation matrix that is
applied to all objects in its group.

#### OSPWorld

`OSPWorld` is the final container for all `OSPInstance` and `OSPLight`
objects. It can contain one or more instances and lights. The world is
passed along with a renderer, camera, and framebuffer to
`ospRenderFrame` to generate an image.

### void ospRetain(OSPObject)

To allow the user greater control over the lifetime of objects, a new
API `ospRetain` has been introduced. This call increments an object's
reference count, and can delay automatic deletion.

Updated Public Parameter Names
------------------------------

OSPRay v2.0.0 has updated public parameter names (the strings used in
`ospSetParam`) to a more consistent naming convention. OSPRay now will
print a warning (visible if debug logs are enabled) if a parameter
provided by the user is not used by an object. This can help catch cases
where applications are using parameter names from OSPRay v1.8.5 or
mistyped names. Some objects have required parameters. In these cases,
OSPRay will invoke the error callback indicating which object and
which parameter.

OSPRay now universally uses the camelCase naming convention for all object
types and parameter names. Furthermore, parameter names which are data
arrays now use singular names to indicate what the elements are, and parameters
which form a logical "struture-of-arrays" are communicated via the
`"[structure_name].[member_name]"` naming convention.

For example, the `"opacities"` array for `linear` transfer functions is now
named `"opacity"`.

Finally, some parameters have changed from being string-based to enum-based.
All enums can be found in `OSPEnums.h`, where their usage can be found in
the main API documentation.

ospRenderFrame
--------------

`ospRenderFrame` has changed in two ways. The signature has changed
from:

    float ospRenderFrame(OSPFrameBuffer, OSPRenderer,
                         const uint32_t frameBufferChannels = OSP_FB_COLOR);

to

    OSPFuture ospRenderFrame(OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld);

And, notably, it is no longer blocking. Two new calls `ospIsReady`, and
`ospWait` are available to manage synchronization.

Utility Library
---------------

As a convenience, a lightweight utility library has been provided to
help users port from the previous versions and reduce boilerplate code.
This set of additional API calls are all implemented in terms of the
core API found in `ospray.h`, where they can be found in
`ospray_util.h`. Their definitions are compiled into `libospray` (the
`ospray::ospray` CMake target) and are compatible with any valid device
implementation.

### OSPData and Parameter helpers

The core OSPRay API has been simplified by removing many of the type
specializations from the data and parameter set calls. The utility library
provides wrappers to the familiar calls listed below:

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

OSPRay v1.x calls to `ospSetData` have been replaced with `ospSetObject`.
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

### Object Usage Simplification

To simplify setting data onto an object, if there is only a single data
item, the wrapper `ospSetObjectAsData` permits the user to assign this
item without first creating a data object containing that item.

For example:

    OSPData geometricModels = ospNewSharedData1D(&model, OSP_GEOMETRIC_MODEL, 1);
    ospSetObject(group, "geometry", geometricModels);
    ospRelease(geometricModels);

simply becomes:

    ospSetObjectAsData(group, "geometry", OSP_GEOMETRIC_MODEL, model);

### Rendering helpers

While `ospRenderFrame` is now asynchronous, some users will prefer the
original blocking behavior that returns the frame variance. The utility
library provides a wrapper to this functionality:

    float ospRenderFrameBlocking(OSPFrameBuffer fb,
                                 OSPRenderer renderer,
                                 OSPCamera camera,
                                 OSPWorld world)

