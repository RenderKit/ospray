Version History
---------------

### Changes in v2.1.0:

-   New clipping geometries feature that allows clipping any scene
    (geometry and volumes); all OSPRay geometry types can by used as
    clipping geometry
    -   Inverted clipping is supported via new `invertNormals` parameter
        of `GeometricModel`
    -   Currently there is a fixed upper limit (64) of how many clipping
        geometries can be nested
    -   When clipping with curves geometry (any basis except linear)
        some rendering artifacts may appear
-   New plane geometry defined via plane equation and optional bounding
    box
-   Sun-sky light based on physical model of Hošek-Wilkie
-   Support for photometric lights (e.g. IES or EULUMDAT)
-   Add new `ospGetTaskDuration` API call to query execution time of
    asynchronous tasks
-   Support for 16bit (unsigned short) textures
-   Add static `cpp::Device::current` method as a C++ wrapper equivalent
    to `ospGetCurrentDevice`
-   Generalized `cpp::Device` parameter setting to match other handle
    types
-   Passing `NULL` to `ospRelease` is not reported as error anymore
-   Fix computation of strides for `OSPData`
-   Fix transparency in `scivis` renderer
-   Add missing C++ wrapper for `ospGetVariance`
-   Proper demonstration of `ospGetVariance` in `ospTutorialAsync`
-   Fix handling of `--osp:device-params` to process and set all passed
    arguments first before committing the device, to ensure it is
    committed in a valid state.
-   Object factory functions are now registered during module
    initialization via the appropriate `registerType` function
-   Fix issue with OSPRay ignoring tasking system thread count settings
-   Fix issue where OSPRay always loaded the ISPC module, even if not
    required
-   OSPRay now requires minimum Open VKL v0.9.0

### Changes in v2.0.1:

-   Fix bug where Embree user-defined geometries were not indexed
    correctly in the scene, which now requires Embree v3.8.0+
-   Fix crash when the path tracer encounters geometric models that do
    not have a material
-   Fix crash when some path tracer materials generated `NULL` bsdfs
-   Fix bug where `ospGetBounds` returned incorrect values
-   Fix missing symbol in denoiser module
-   Fix missing symbol exports on Windows for all OSPRay built modules
-   Add the option to specify a single color for geometric models
-   The `scivis` renderer now respects the opacity component of `color`
    on geometric models
-   Fix various inconsistent handling of frame buffer alpha between
    renderers
-   `ospGetCurrentDevice` now increments the ref count of the returned
    `OSPDevice` handle, so applications will need to release the handle
    when finished by using `ospDeviceRelease` accordingly
-   Added denoiser to `ospExamples` app
-   Added `module_mpi` to superbuild (disabled by default)
-   The superbuild now will emit a CMake error when using any 32-bit
    CMake generator, as 32-bit builds are not supported

### Changes in v2.0.0:

-   New major revision of OSPRay brings API breaking improvements over
    v1.x. See [doc/ospray2_porting_guide.md] for a deeper description of
    migrating from v1.x to v2.0 and the latest
    [API documentation](README.md#ospray-api)
    -   `ospRenderFrame` now takes all participating objects as
        function parameters instead of setting some as renderer params
    -   `ospRenderFrame` is now asynchronous, where the task is managed
        through a returned `OSPFuture` handle
    -   The heirarchy of objets in a scene are now more granular to
        aid in scene construction flexibility and reduce potential
        object duplication
    -   Type-specific parameter setting functions have been consolidated
        into a single `ospSetParam` API call
    -   C++ wrappers found in `ospray_cpp.h` now automatically track
        handle lifetimes, therefore applications using them do not need
        to use `ospRelease` (or the new `ospRetain`) with them: see
        usage example in `apps/tutorials/ospTutorial.cpp`
    -   Unused parameters are reported as status messages when
        `logLevel` is at least `warning` (most easily set by enabling
        OSPRay debug on initialization)
-   New utility library which adds functions to help with new API
    migration and reduction of boilerplate code
    -   Use `ospray_util.h` to access these additional functions
    -   All utility functions are implemented in terms of the core API
        found in `ospray.h`, therefore they are compatible with any
        device backend
-   Introduction of new Intel® Open Volume Kernel Library (Open VKL)
    for greatly enhanced volume sampling and rendering features and
    performance
-   Added direct support for Intel® Open Image Denoise as an optional
    module, which adds a `denoiser` type to `ospNewImageOperation`
-   OSPRay now requires minimum Embree v3.7.0
-   New CMake superbuild available to build both OSPRay's dependencies
    and OSPRay itself
    -   Found in `scripts/superbuild`
    -   See documentation for more details and example usage
-   The `ospcommon` library now lives as a stand alone repository and
    is required to build OSPRay
-   The MPI module is now a separate repository, which also contains all
    MPI distributed rendering documentation
-   Log levels are now controled with enums and named strings (where applicable)
    -   A new flag was also introduced which turns all OSP_LOG_WARNING messages
        into errors, which are submitted to the error callback instead of the
        message callback
    -   Any unused parameters an object ignores now emit a warning message
-   New support for volumes in the `pathtracer`
    -   Several parameters are available for performance/quality
        trade-offs for both photo-realistic and scientific visualization
        use cases
-   Simplification of the SciVis renderer
    -   Fixed AO lighting and simple ray marched volume rendering for
        ease of use and performance
-   Overlapping volumes are now supported in both the `pathtracer` and `scivis`
    renderers
-   New API call for querying the bounds of objects (`OSPWorld`,
    `OSPInstance`, and `OSPGroup`)
-   Lights now exist as a parameter to the world instead of the renderer
-   Removal of `slices` geometry. Instead, any geometry with volume
    texture can be used for slicing
-   Introduction of new `boxes` geometry type
-   Expansion of information returned by `ospPick`
-   Addition of API to query version information at runtime
-   Curves now supports both, per vertex varying radii as in `vec4f[]
    vertex.position_radius` and constant radius for the geometry with
    `float radius`. It uses `OSP_ROUND` type and `OSP_LINEAR` basis by
    default to create the connected segments of constant radius. For per
    vertex varying radii curves it uses Embree curves.
-   Add new Embree curve type `OSP_CATMULL_ROM` for curves
-   Minimum required Embree version is now 3.7.0
-   Removal of `cylinders` and `streamlines` geometry, use `curves`
    instead
-   Triangle mesh and Quad mesh are superseded by the `mesh` geometry
-   Applications need to use the various error reporting methods to
    check wether the creation (via `ospNew...`) of objects failed; a
    returned `NULL` is not a special handle anymore to signify an error
-   Changed module init methods to facilitate version checking:
    `extern "C" OSPError ospray_module_init_<name>(int16_t versionMajor, int16_t versionMinor, int16_t versionPatch)`
-   The `map_backplate` texture is supported in all renderers and does
    not hide lights in infinity (like the HDRI light) anymore;
    explicitely make lights in`visible` if this is needed
-   Changed the computation of variance for adaptive accumulation to be
    independent of `TILE_SIZE`, thus `varianceThreshold` needs to be
    adapted if using a different TILE_SIZE than default 64
-   `OSPGeometricModel` now has the option to index a renderer-global material
    list that lives on the renderer, allowing scenes to avoid renderer-specific
    materials
-   Object type names and parameters all now follow the camel-case convention
-   New `ospExamples` app which consolidates previous interactive apps into one
-   New `ospBenchmark` app which implements a runnable benchmark suite
-   Known issues:
    -   ISPC v1.11.0 and Embree v3.6.0 are both incompatible with OSPRay
        and should be avoided (OSPRay should catch this during CMake
        configure)

### Changes in v1.8.5:

-   Fix float precision cornercase (`NaN`s) in sphere light sampling
-   Fix CMake bug that assumed `.git` was a directory, which is not true
    when using OSPRay as a git submodule
-   Fix CMake warning
-   Fix `DLL_EXPORT` issue with `ospray_testing` helper library on
    Windows

### Changes in v1.8.4:

-   Add location of `libospray` to paths searched to find modules

### Changes in v1.8.3:

-   Fix bug where parameters set by `ospSet1b()` were being ignored
-   Fix bug in box intersection tests possibly creating `NaN`s
-   Fix issue with client applications calling `find_package(ospray)`
    more than once
-   Fix bug in cylinder intersection when ray and cylinder are
    perpendicular
-   Fix rare crash in path tracer / MultiBSDF

### Changes in v1.8.2:

-   CMake bug fix where external users of OSPRay needed CMake newer than
    version 3.1
-   Fix incorrect propagation of tasking system flags from an OSPRay
    install
-   Fix inconsistency between supported environment variables and
    command line parameters passed to `ospInit()`
    -   Missing variables were `OSPRAY_LOAD_MODULES` and
        `OSPRAY_DEFAULT_DEVICE`

### Changes in v1.8.1:

-   CMake bug fix to remove full paths to dependencies in packages

### Changes in v1.8.0:

-   This will be the last minor revision of OSPRay. Future development
    effort in the `devel` branch will be dedicated to v2.0 API changes
    and may break existing v1.x applications.
    -   This will also be the last version of OSPRay to include
        `ospray_sg` and the Example Viewer. Users which depend on these
        should instead use the separate OSPRay Studio project, where
        `ospray_sg` will be migrated.
    -   We will continue to support patch releases of v1.8.x in case of
        any reported bugs
-   Refactored CMake to use newer CMake concepts
    -   All targets are now exported in OSPRay installs and can be
        consumed by client projects with associated includes, libraries,
        and definitions
    -   OSPRay now requires CMake v3.1 to build
    -   See documentation for more details
-   Added new minimal tutorial apps to replace the more complex Example
    Viewer
-   Added new "`subdivision`" geometry type to support subdivision
    surfaces
-   Added support for texture formats `L8`, `LA8` (gamma-encoded
    luminance), and `RA8` (linear two component). Note that the enum
    `OSP_TEXTURE_FORMAT_INVALID` changed its value, thus recompilation
    may be necessary.
-   Automatic epsilon handling, which removes the "`epsilon`" parameter
    on all renderers
-   Normals in framebuffer channel `OSP_FB_NORMAL` are now in
    world-space
-   Added support for Intel® Open Image Denoise to the Example Viewer
    -   This same integration will soon be ported to OSPRay Studio
-   Fixed artifacts for scaled instances of spheres, cylinders and
    streamlines
-   Improvements to precision of intersections with cylinders and
    streamlines
-   Fixed Quadlight: the emitting side is now indeed in direction
    `edge1`×`edge2`

### Changes in v1.7.3:

-   Make sure a "`default`" device can always be created
-   Fixed `ospNewTexture2D` (completely implementing old behavior)
-   Cleanup any shared object handles from the OS created from
    `ospLoadModule()`

### Changes in v1.7.2:

-   Fixed issue in `mpi_offload` device where `ospRelease` would
    sometimes not correctly free objects
-   Fixed issue in `ospray_sg` where structured volumes would not
    properly release the underlying OSPRay object handles

### Changes in v1.7.1:

-   Fixed issue where the `Principled` material would sometimes show up
    incorrectly as black
-   Fixed issue where some headers were missing from install packages

### Changes in v1.7.0:

-   Generalized texture interface to support more than classic 2D image
    textures, thus `OSPTexture2D` and `ospNewTexture2D` are now
    deprecated, use the new API call `ospNewTexture("texture2d")`
    instead
    -   Added new `volume` texture type to visualize volume data on
        arbitrary geometry placed inside the volume
-   Added new framebuffer channels `OSP_FB_NORMAL` and `OSP_FB_ALBEDO`
-   Applications can get information about the progress of rendering the
    current frame, and optionally cancel it, by registering a callback
    function via `ospSetProgressFunc()`
-   Lights are not tied to the renderer type, so a new function
    `ospNewLight3()` was introduced to implement this. Please convert
    all uses of `ospNewLight()` and/or `ospNewLight2()` to
    `ospNewLight3()`
-   Added sheenTint parameter to Principled material
-   Added baseNormal parameter to Principled material
-   Added low-discrepancy sampling to path tracer
-   The `spp` parameter on the renderer no longer supports values less
    than 1, instead applications should render to a separate, lower
    resolution framebuffer during interaction to achieve the same
    behavior

### Changes in v1.6.1:

-   Many bug fixes
    -   Quad mesh interpolation and sampling
    -   Normal mapping in path tracer materials
    -   Memory corruption with partly emitting mesh lights
    -   Logic for setting thread affinity

### Changes in v1.6.0:

-   Updated ispc device to use Embree3 (minimum required Embree
    version is 3.1)
-   Added new `ospShutdown()` API function to aid in correctness and
    determinism of OSPRay API cleanup
-   Added "`Principled`" and "`CarPaint"` materials to the path tracer
-   Improved flexibility of the tone mapper
-   Improvements to unstructured volume
    -   Support for wedges (in addition to tets and hexes)
    -   Support for implicit isosurface geometry
    -   Support for cell-centered data (as an alternative to per-vertex
        data)
    -   Added an option to precompute normals, providing a
        memory/performance trade-off for applications
-   Implemented "`quads`" geometry type to handle quads directly
-   Implemented the ability to set "void" cell values in all volume
    types: when `NaN` is present as a volume's cell value the volume
    sample will be ignored by the SciVis renderer
-   Fixed support for color strides which were not multiples of
    `sizeof(float)`
-   Added support for RGBA8 color format to spheres, which can be set by
    specifying the "`color_format`" parameter as `OSP_UCHAR4`, or
    passing the "`color`" array through an `OSPData` of type
    `OSP_UCHAR4`.
-   Added ability to configure Embree scene flags via `OSPModel`
    parameters
-   `ospFreeFrameBuffer()` has been deprecated in favor of using
    `ospRelease()` to free framebuffer handles
-   Fixed memory leak caused by incorrect parameter reference counts in
    ispc device
-   Fixed occasional crashes in the `mpi_offload` device on shutdown
-   Various improvements to sample apps and `ospray_sg`
    -   Added new `generator` nodes, allowing the ability to inject
        programmatically generated scene data (only C++ for now)
    -   Bug fixes and improvements to enhance stability and usability

### Changes in v1.5.0:

-   Unstructured tetrahedral volume now generalized to also support
    hexahedral data, now called `unstructured_volume`
-   New function for creating materials (`ospNewMaterial2()`) which
    takes the renderer type string, not a renderer instance (the old
    version is now deprecated)
-   New `tonemapper` PixelOp for tone mapping final frames
-   Streamlines now support per-vertex radii and smooth interpolation
-   `ospray_sg` headers are now installed alongside the SDK
-   Core OSPRay ispc device now implemented as a module
    -   Devices which implement the public API are no longer required to
        link the dependencies to core OSPRay (e.g., Embree v2.x)
    -   By default, `ospInit()` will load the ispc module if a device
        was not created via `--osp:mpi` or `--osp:device:[name]`
-   MPI devices can now accept an existing world communicator instead of
    always creating their own
-   Added ability to control ISPC specific optimization flags via CMake
    options. See the various `ISPC_FLAGS_*` variables to control which
    flags get used
-   Enhancements to sample applications
    -   `ospray_sg` (and thus `ospExampleViewer`/`ospBenchmark`) can now
        be extended with new scene data importers via modules or the SDK
    -   Updated `ospTutorial` examples to properly call `ospRelease()`
    -   New options in the `ospExampleViewer` GUI to showcase new
        features (sRGB framebuffers, tone mapping, etc.)
-   General bug fixes
    -   Fixes to geometries with multiple emissive materials
    -   Improvements to precision of ray-sphere intersections

### Changes in v1.4.3:

-   Several bug fixes
    -   Fixed potential issue with static initialization order
    -   Correct compiler flags for `Debug` config
    -   Spheres `postIntersect` shading is now 64-bit safer

### Changes in v1.4.2:

-   Several cleanups and bug fixes
    -   Fixed memory leak where the Embree BVH was never released when
        an `OSPModel` was released
    -   Fixed a crash when API logging was enabled in certain situations
    -   Fixed a crash in MPI mode when creating lights without a
        renderer
    -   Fixed an issue with camera lens samples not initialized when
        `spp` <= 0
    -   Fixed an issue in `ospExampleViewer` when specifying multiple data
        files
-   The C99 tutorial is now indicated as the default; the C++ wrappers
    do not change the semantics of the API (memory management) so the
    C99 version should be considered first when learning the API

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
    -   Additionally support RGB `eta`/`k` for Metal
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
        a scene graph implementation
    -   Old (unused) libraries have been removed: miniSG, command line,
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
    either device parameter `setAffinity`, or command line parameter
    `osp:setaffinity`, or environment variable `OSPRAY_SET_AFFINITY`
-   Changed behavior of the background color in the SciVis renderer:
    `bgColor` now includes alpha and is always blended (no
    `backgroundEnabled` anymore). To disable the background do not set
    `bgColor`, or set it to transparent black (0, 0, 0, 0)
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
    -   `ospDeviceSetStatusFunc()` replaces the deprecated
        `ospDeviceSetErrorMsgFunc()`
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
            rendering high-frequency volume data
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
    speedups of up to 1.5×
-   Updates to path tracer
    -   Reworked material system
    -   Added texture transformations and colored transparency in OBJ
        material
    -   Support for alpha and depth components of framebuffer
-   Added thinlens camera, i.e., support for depth of field
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
    -   New API call to set (sub-)regions of volume data (`ospSetRegion()`)
-   Added a subsampling-mode, enabled with a negative `spp` parameter;
    the first frame after scene changes is rendered with reduced
    resolution, increasing interactivity
-   Added multi-target ISA support: OSPRay will now select the
    appropriate ISA at runtime
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

