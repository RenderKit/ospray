Version History
---------------

### Changes in v1.5.0:

-   TetrahedralVolume now generalized to take both tet and hex data, now called
    `UnstructuredVolume`
-   New function for creating materials (`ospNewMaterial2`) which takes the
    renderer type string, not a renderer instance (the old version is now
    deprecated)
-   New `tonemapper` PixelOp for tone mapping final frames
-   Streamlines now support per-vertex radii and smooth interpolation
-   `ospray_sg` headers are now installed alongside the SDK
-   Core OSPRay ispc device now implemented as a module
    -   Devices which implement the public API are no longer required to link
        the dependencies to core OSPRay (e.g. Embree v2.x)
    -   By default, `ospInit` will load the ispc module if a device was not
        created via `--osp:mpi` or `--osp:device:[name]`
-   MPI devices can now accept an existing world communicator instead of always
    creating their own
-   Added ability to control ISPC specific optimization flags via CMake options
    -   See the various `ISPC_FLAGS_*` variables to control which flags get used
-   Enhancements to sample applications
    -   `ospray_sg` (and thus `ospExampleViewer`/`ospBenchmark`) can now be
        extended with new scene data importers via modules or the SDK
    -   Updated ospTutorial examples to properly call ospRelease()
    -   New options in the ospExampleViewer GUI to showcase new features
        -   SRGB frame buffers, tone mapping, etc.
-   General bug fixes
    -   Fixes to geometries with multiple emissive materials
    -   Improvements to precision of ray-sphere intersections

### Changes in v1.4.3:

-   Several bug fixes
    -   Fixed potential issue with static initialization order
    -   Correct compiler flags for Debug config
    -   Spheres `postIntersect` shading is now 64-bit safer

### Changes in v1.4.2:

-   Several cleanups and bug fixes
    -   Fixed memory leak where the Embree BVH was never released when an
        OSPModel was released
    -   Fixed a crash when API logging was enabled in certain situations
    -   Fixed a crash in MPI mode when creating lights without a renderer
    -   Fixed an issue with camera lens samples not initilized when spp <= 0
    -   Fixed an issue in ospExampleViewer when specifying multiple data files
-   The C99 tutorial is now indicated as the default; the C++ wrappers do not
    change the semantics of the API (memory management) so the C99 version
    should be considered first when learning the API

### Changes in v1.4.1:

-   Several cleanups and bug fixes
    -   Improved precision of ray intersection with streamlines,
        spheres, and cylinder geometries
    -   Fixed address overflow in framebuffer, in practice image size is
        now limited only by available memory
    -   Fixed several deadlocks and race conditions
    -   Fix shadow computation in SciVis renderer, objects behind light
        sources do not occlude anymore
    -   No more image jittering with MPI rendering when no accumulation
        buffer is used
-   Improved path tracer materials
    -   Also support RGB `eta`/`k` for Metal
    -   Added Alloy material, a "metal" with textured color
-   Minimum required Embree version is now v2.15

### Changes in v1.4.0:

-   New adaptive mesh refinement (AMR) and unstructured tetrahedral
    volume types
-   Dynamic load balancing is now implemented for the `mpi_offload`
    device
-   Many improvements and fixes to the available path tracer materials
    -   Specular lobe of OBJMaterial uses Blinn-Phong for more realistic
        highlights
    -   Metal accepts spectral samples of complex refraction index
    -   ThinGlass behaves consistent to Glass and can texture
        attenuation color
-   Added Russian roulette termination to path tracer
-   SciVis OBJMaterial accepts texture coordinate transformations
-   Applications can now access depth information in MPI distributed
    uses of OSPRay (both `mpi_offload` and `mpi_distributed` devices)
-   Many robustness fixes for both the `mpi_offload` and
    `mpi_distributed` devices through improvements to the `mpi_common`
    and `mpi_maml` infrastructure libraries
-   Major sample app cleanups
    -   `ospray_sg` library is the new basis for building apps, which is
        a scenegraph implementation
    -   Old (unused) libraries have been removed: miniSG, commandline,
        importer, loaders, and scripting
    -   Some removed functionality (such as scripting) may be
        reintroduced in the new infrastructure later, though most
        features have remained and have been improved
    -   Optional improved texture loading has been transitioned from
        ImageMagick to OpenImageIO
-   Many cleanups, bug fixes, and improvements to `ospray_common` and
    other support libraries
-   This will be the last release in which we support MSVC12 (Visual
    Studio 2013). Future releases will require VS2015 or newer

### Changes in v1.3.1:

-   Improved robustness of OSPRay CMake `find_package` config
    -   Fixed bugs related to CMake configuration when using the OSPRay
        SDK from an install
-   Fixed issue with Embree library when installing with
    `OSPRAY_INSTALL_DEPENDENCIES` enabled

### Changes in v1.3.0:

-   New MPI distributed device to support MPI distributed applications
    using OSPRay collectively for "in-situ" rendering (currently in
    "alpha")
    -   Enabled via new `mpi_distributed` device type
    -   Currently only supports `raycast` renderer, other renderers will
        be supported in the future
    -   All API calls are expected to be exactly replicated (object
        instances and parameters) except scene data (geometries and
        volumes)
    -   The original MPI device is now called the `mpi_offload` device
        to differentiate between the two implementations
-   Support of Intel® AVX-512 for next generation Intel® Xeon® processor
    (codename Skylake), thus new minimum ISPC version is 1.9.1
-   Thread affinity of OSPRay's tasking system can now be controlled via
    either device parameter `setAffinity`, or commandline parameter
    `osp:setaffinity`, or environment variable `OSPRAY_SET_AFFINITY`
-   Changed behavior of the background color in the SciVis renderer:
    `bgColor` now includes alpha and is always blended (no
    `backgroundEnabled` anymore). To disable the background don't set
    bgColor, or set it to transparent black (0, 0, 0, 0)
-   Geometries "`spheres`" and "`cylinders`" now support texture
    coordinates
-   The GLUT- and Qt-based demo viewer applications have been replaced
    by an example viewer with minimal dependencies
    -   Building the sample applications now requires GCC 4.9
        (previously 4.8) for features used in the C++ standard library;
        OSPRay itself can still be built with GCC 4.8
    -   The new example viewer based on `ospray::sg` (called
        `ospExampleViewerSg`) is the single application we are
        consolidating to, `ospExampleViewer` will remain only as a
        deprecated viewer for compatibility with the old `ospGlutViewer`
        application
-   Deprecated `ospCreateDevice()`; use `ospNewDevice()` instead
-   Improved error handling
    -   Various API functions now return an `OSPError` value
    -   `ospDeviceSetStatusFunc` replaces the deprecated
        `ospDeviceSetErrorMsgFunc`
    -   New API functions to query the last error
        (`ospDeviceGetLastErrorCode()` and `ospDeviceGetLastErrorMsg()`)
        or to register an error callback with `ospDeviceSetErrorFunc()`
    -   Fixed bug where exceptions could leak to C applications

### Changes in v1.2.1:

-   Various bug fixes related to MPI distributed rendering, ISPC issues
    on Windows, and other build related issues

### Changes in v1.2.0:

-   Added support for volumes with voxelType `short` (16-bit signed
    integers). Applications need to recompile, because `OSPDataType` has
    been re-enumerated
-   Removed SciVis renderer parameter `aoWeight`, the intensity (and now
    color as well) of AO is controlled via "`ambient`" lights. If
    `aoSamples` is zero (the default) then ambient lights cause ambient
    illumination (without occlusion)
-   New SciVis renderer parameter `aoTransparencyEnabled`, controlling
    whether object transparency is respected when computing ambient
    occlusion (disabled by default, as it is considerable slower)
-   Implement normal mapping for SciVis renderer
-   Support of emissive (and illuminating) geometries in the path tracer
    via new material "`Luminous`"
-   Lights can optionally made invisible by using the new parameter
    `isVisible` (only relevant for path tracer)
-   OSPRay Devices are now extendable through modules and the SDK
    -   Devices can be created and set current, creating an alternative
        method for initializing the API
    -   New API functions for committing parameters on Devices
-   Removed support for the first generation Intel® Xeon Phi™ coprocessor
    (codename Knights Corner)
-   Other minor improvements, updates, and bug fixes
    -   Updated Embree required version to v2.13.0 for added features
        and performance
    -   New API function `ospDeviceSetErrorMsgFunc()` to specify a
        callback for handling message outputs from OSPRay
    -   Added ability to remove user set parameter values with new
        `ospRemoveParam()` API function
    -   The MPI device is now provided via a module, removing the need
        for having separately compiled versions of OSPRay with and
        without MPI
    -   OSPRay build dependencies now only get installed if
        `OSPRAY_INSTALL_DEPENDENCIES` CMake variable is enabled

### Changes in v1.1.2:

-   Various bug fixes related to normalization, epsilons and debug
    messages

### Changes in v1.1.1:

-   Fixed support of first generation Intel Xeon Phi coprocessor
    (codename Knights Corner) and the COI device
-   Fix normalization bug that caused rendering artifacts

### Changes in v1.1.0:

-   New "scivis" renderer features
    -   Single sided lighting (enabled by default)
    -   Many new volume rendering specific features
        -   Adaptive sampling to help improve the correctness of
            rendering high frequency volume data
        -   Pre-integration of transfer function for higher fidelity
            images
        -   Ambient occlusion
        -   Volumes can cast shadows
        -   Smooth shading in volumes
        -   Single shading point option for accelerated shading
-   Added preliminary support for adaptive accumulation in the MPI
    device
-   Camera specific features
    -   Initial support for stereo rendering with the perspective camera
    -   Option `architectural` in perspective camera, rectifying
        vertical edges to appear parallel
    -   Rendering a subsection of the full view with
        `imageStart`/`imageEnd` supported by all cameras
-   This will be our last release supporting the first generation Intel
    Xeon Phi coprocessor (codename Knights Corner)
    -   Future major and minor releases will be upgraded to the latest
        version of Embree, which no longer supports Knights Corner
    -   Depending on user feedback, patch releases are still made to
        fix bugs
-   Enhanced output statistics in `ospBenchmark` application
-   Many fixes to the OSPRay SDK
    -   Improved CMake detection of compile-time enabled features
    -   Now distribute OSPRay configuration and ISPC CMake macros
    -   Improved SDK support on Windows
-   OSPRay library can now be compiled with `-Wall` and `-Wextra` enabled
    -   Tested with GCC v5.3.1 and Clang v3.8
    -   Sample applications and modules have not been fixed yet, thus
        applications which build OSPRay as a CMake subproject should
        disable them with `-DOSPRAY_ENABLE_APPS=OFF` and
        `-DOSPRAY_ENABLE_MODULES=OFF`
-   Minor bug fixes, improvements, and cleanups
    -   Regard shading normal when bump mapping
    -   Fix internal CMake naming inconsistencies in macros
    -   Fix missing API calls in C++ wrapper classes
    -   Fix crashes on MIC
    -   Fix thread count initialization bug with TBB
    -   CMake optimizations for faster configuration times
    -   Enhanced support for scripting in both `ospGlutViewer` and
        `ospBenchmark` applications

### Changes in v1.0.0:

-   New OSPRay SDK
    -   OSPRay internal headers are now installed, enabling applications
        to extend OSPRay from a binary install
    -   CMake macros for OSPRay and ISPC configuration now a part of
        binary releases
        -   CMake clients use them by calling
            `include(${OSPRAY_USE_FILE})` in their CMake code after
            calling `find_package(ospray)`
    -   New OSPRay C++ wrapper classes
        -   These act as a thin layer on top of OSPRay object handles,
            where multiple wrappers will share the same underlying
            handle when assigned, copied, or moved
        -   New OSPRay objects are only created when a class instance is
            explicitly constructed
        -   C++ users are encouraged to use these over the `ospray.h`
            API
-   Complete rework of sample applications
    -   New shared code for parsing the `commandline`
    -   Save/load of transfer functions now handled through a separate
        library which does not depend on Qt
    -   Added `ospCvtParaViewTfcn` utility, which enables
        `ospVolumeViewer` to load color maps from ParaView
    -   GLUT based sample viewer updates
        -   Rename of `ospModelViewer` to `ospGlutViewer`
        -   GLUT viewer now supports volume rendering
        -   Command mode with preliminary scripting capabilities,
            enabled by pressing '`:`' key (not available when using
            Intel C++ Compiler (icc))
    -   Enhanced support of sample applications on Windows
-   New minimum ISPC version is 1.9.0
-   Support of Intel® AVX-512 for second generation Intel Xeon Phi
    processor (codename Knights Landing) is now a part of the
    `OSPRAY_BUILD_ISA` CMake build configuration
    -   Compiling AVX-512 requires icc to be enabled as a build option
-   Enhanced error messages when `ospLoadModule()` fails
-   Added `OSP_FB_RGBA32F` support in the `DistributedFrameBuffer`
-   Updated Glass shader in the path tracer
-   Many miscellaneous cleanups, bug fixes, and improvements

### Changes in v0.10.1:

-   Fixed support of first generation Intel Xeon Phi coprocessor
    (codename Knights Corner)
-   Restored missing implementation of `ospRemoveVolume()`

### Changes in v0.10.0:

-   Added new tasking options: `Cilk`, `Internal`, and `Debug`
    -   Provides more ways for OSPRay to interact with calling
        application tasking systems
        - `Cilk`: Use Intel® Cilk™ Plus language extensions (icc only)
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
    needed to change to accept the new `OSPTextureFormat` parameter
-   Similarly, OSPRay's framebuffer types now also distinguishes between
    linear and sRGB 8-bit formats. The new types are `OSP_FB_NONE`,
    `OSP_FB_RGBA8`, `OSP_FB_SRGBA`, and `OSP_FB_RGBA32F`
-   Changed "scivis" renderer parameter defaults
    -   All shading (AO + shadows) must be explicitly enabled
-   OSPRay can now use a newer, pre-installed Embree enabled by the new
    `OSPRAY_USE_EXTERNAL_EMBREE` CMake option
-   New `ospcommon` library used to separately provide math types and OS
    abstractions for both OSPRay and sample applications
    -   Removes extra dependencies on internal Embree math types and
        utility functions
    -   `ospray.h` header is now C99 compatible
-   Removed loaders module, functionality remains inside of
    `ospVolumeViewer`
-   Many miscellaneous cleanups, bug fixes, and improvements
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
-   Many miscellaneous cleanups, bug fixes, and improvements
    -   The depthbuffer is now correctly populated by in the "scivis"
        renderer
    -   Updated default renderer to be "ao1" in ospModelViewer
    -   Trianglemesh `postIntersect` shading is now 64-bit safe
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
-   Updates to path tracer
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
-   Volume rendering enhancements
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
-   Added new features to the volume renderer
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

