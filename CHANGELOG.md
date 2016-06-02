Version History
---------------

### Changes in v0.10.1:

-   Fixed support of first generation Intel® Xeon Phi™ coprocessor
    (codename Knights Corner)
-   Restored missing implementation of `ospRemoveVolume()`

### Changes in v0.10.0:

-   Added new tasking options: `Cilk`, `Internal`, and `Debug`
    -   Provides more ways for OSPRay to interact with calling
        application tasking systems
        - `Cilk`: Use Intel® Cilk™ Plus language extensions (ICC only)
        - `Internal`: Use hand written OSPRay tasking system
        - `Debug`: All tasks are run in serial (useful for debugging)
    -   In most cases, Intel Threading Building Blocks (Intel `TBB`)
        remains the fastest option
-   Added support for adaptive accumulation and stopping
    -   `ospRenderFrame` now returns an estimation of the variance in
        the rendered image if the framebuffer was created with the
        `OSP_FB_VARIANCE` channel
    -   If the renderer parameter `varianceThreshold` is set,
        progressive refinement concentrates on regions of the image with
        a variance higher than this threshold
-   Added support for volumes with voxelType `ushort` (16-bit unsigned
    integers)
-   `OSPTexture2D` now supports sRGB formats -- actually most images are
    stored in sRGB. As a consequence the API call `ospNewTexture2D()`
    needed to change to accept the new `OSPTextureFormat` parameter.
-   Similarly, OSPRay's framebuffer types now also distinguishes between
    linear and sRGB 8-bit formats. The new types are `OSP_FB_NONE`,
    `OSP_FB_RGBA8`, `OSP_FB_SRGBA`, and `OSP_FB_RGBA32F`
-   Changed "scivis" renderer parameter defaults
    -   All shading (AO + shadows) must be explicitly enabled
-   OSPRay can now use a newer, pre-installed Embree enabled by the new
    `OSPRAY_USE_EXTERNAL_EMBREE` CMake option
-   New `ospcommon` library used to separately provide math types and OS
    abstractions for both OSPRay and sample apps
    -   Removes extra dependencies on internal Embree math types and
        utility functions
    -   `ospray.h` header is now C99 compatible
-   Removed loaders module, functionality remains inside of
    `ospVolumeViewer`
-   Many miscellaneous cleanups, bugfixes, and improvements:
    -   Fixed data distributed volume rendering bugs when using less
        blocks than workers
    -   Fixes to CMake `find_package()` config
    -   Fix bug in `GhostBlockBrickVolume` when using `double`s
    -   Various robustness changes made in CMake to make it easier to
        compile OSPRay

### Changes in v0.9.1:

-   Volume rendering now integrated into the "scivis" renderer
    -   Volumes are rendered in the same way the "dvr" volume renderer
        renders them
    -   Ambient occlusion works with implicit isosurfaces, with a known
        visual quality/performance trade-off
-   Intel Xeon Phi coprocessor (codename Knights Corner) COI device and
    build infrastructure restored (volume rendering is known to still be
    broken)
-   New support for CPack built OSPRay binary redistributable packages
-   Added support for HDRI lighting in path tracer
-   Added `ospRemoveVolume()` API call
-   Added ability to render a subsection of the full view into the
    entire framebuffer in the perspective camera
-   Many miscellaneous cleanups, bugfixes, and improvements:
    -   The depthbuffer is now correctly populated by in the "scivis"
        renderer
    -   Updated default renderer to be "ao1" in ospModelViewer
    -   Trianglemesh postIntersect shading is now 64-bit safe
    -   Texture2D has been reworked, with many improvements and bug fixes
    -   Fixed bug where MPI device would freeze while rendering frames
        with Intel TBB
    -   Updates to CMake with better error messages when Intel TBB is
        missing

### Changes in v0.9.0:

The OSPRay v0.9.0 release adds significant new features as well as API
changes.

-   Experimental support for data-distributed MPI-parallel volume
    rendering
-   New SciVis-focused renderer ("raytracer" or "scivis") combining
    functionality of "obj" and "ao" renderers
    -   Ambient occlusion is quite flexible: dynamic number of samples,
        maximum ray distance, and weight
-   Updated Embree version to v2.7.1 with native support for Intel
    AVX-512 for triangle mesh surface rendering on the Intel Xeon Phi
    processor (codename Knights Landing)
-   OSPRay now uses C++11 features, requiring up to date compiler and
    standard library versions (GCC v4.8.0)
-   Optimization of volume sampling resulting in volume rendering
    speedups of up to 1.5x
-   Updates to pathtracer
    -   Reworked material system
    -   Added texture transformations and colored transparency in OBJ
        material
    -   Support for alpha and depth components of framebuffer 
-   Added thinlens camera, i.e. support for depth of field
-   Tasking system has been updated to use Intel Threading Building
    Blocks (Intel TBB)
-   The `ospGet*()` API calls have been deprecated and will be removed
    in a subsequent release

### Changes in v0.8.3:

-   Enhancements and optimizations to path tracer
    -   Soft shadows (light sources: sphere, cone, extended spot, quad)
    -   Transparent shadows
    -   Normal mapping (OBJ material)
-   Volume rendering enhancements:
    -   Expanded material support
    -   Support for multiple lights
    -   Support for double precision volumes
    -   Added `ospSampleVolume()` API call to support limited probing of
        volume values
-   New features to support compositing externally rendered content with
    OSPRay-rendered content
    -   Renderers support early ray termination through a maximum depth
        parameter
    -   New OpenGL utility module to convert between OSPRay and OpenGL
        depth values
-   Added panoramic and orthographic camera types
-   Proper CMake-based installation of OSPRay and CMake `find_package()`
    support for use in external projects
-   Experimental Windows support
-   Deprecated `ospNewTriangleMesh()`; use `ospNewGeometry("triangles")`
    instead
-   Bug fixes and cleanups throughout the codebase

### Changes in v0.8.2:

-   Initial support for Intel AVX-512 and the Intel Xeon Phi processor
    (codename Knights Landing)
-   Performance improvements to the volume renderer
-   Incorporated implicit slices and isosurfaces of volumes as core
    geometry types
-   Added support for multiple disjoint volumes to the volume renderer
-   Improved performance of `ospSetRegion()`, reducing volume load times
-   Improved large data handling for the `shared_structured_volume` and
    `block_bricked_volume` volume types
-   Added support for DDS horizon data to the seismic module
-   Initial support in the Qt viewer for volume rendering
-   Updated to ISPC 1.8.2
-   Various bug fixes, cleanups and documentation updates throughout the
    codebase

### Changes in v0.8.1:

-   The volume renderer and volume viewer can now be run MPI parallel
    (data replicated) using the `--osp:mpi` command line option
-   Improved performance of volume grid accelerator generation, reducing
    load times for large volumes
-   The volume renderer and volume viewer now properly handle multiple
    isosurfaces
-   Added small example tutorial demonstrating how to use OSPRay
-   Several fixes to support older versions of GCC
-   Bug fixes to `ospSetRegion()` implementation for arbitrarily shaped
    regions and setting large volumes in a single call
-   Bug fix for geometries with invalid bounds; fixes streamline and
    sphere rendering in some scenes
-   Fixed bug in depth buffer generation

### Changes in v0.8.0:

-   Incorporated early version of a new Qt-based viewer to eventually
    unify (and replace) the existing simpler GLUT-based viewers
-   Added new path tracing renderer (`ospray/render/pathtracer`),
-   roughly based on the Embree sample path tracer
-   Added new features to the volume renderer:
    -   Gradient shading (lighting)
    -   Implicit isosurfacing
    -   Progressive refinement
    -   Support for regular grids, specified with the `gridOrigin` and
        `gridSpacing` parameters
    -   New `shared_structured_volume` volume type that allows voxel
        data to be provided by applications through a shared data buffer
    -   New API call to set subregions of volume data (`ospSetRegion()`)
-   Added a subsampling-mode, enabled with a negative `spp` parameter;
    the first frame after scene changes is rendered with reduced
    resolution, increasing interactivity
-   Added multi-target ISA support: OSPRay will now select the
    appropriate ISA at run time
-   Added support for the Stanford SEP file format to the seismic
    module
-   Added `--osp:numthreads <n>` command line option to restrict the
    number of threads OSPRay creates
-   Various bug fixes, cleanups and documentation updates throughout the
    codebase

### Changes in v0.7.2:

-   Build fixes for older versions of GCC and Clang
-   Fixed time series support in ospVolumeViewer
-   Corrected memory management for shared data buffers
-   Updated to ISPC 1.8.1
-   Resolved issue in XML parser

