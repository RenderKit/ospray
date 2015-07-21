OSPRay
======

This is release v0.8.2 of OSPRay.

For more information, visit http://www.ospray.org.

Release History
---------------

#### Changes in v0.8.1:

- The volume renderer and volume viewer can now be run MPI parallel
  (data replicated) using the "--osp:mpi" command line option.
- Improved performance of volume grid accelerator generation, reducing
  load times for large volumes.
- The volume renderer and volume viewer now properly handle multiple
  isosurfaces.
- Added small example tutorial demonstrating how to use OSPRay.
- Several fixes to support older versions of GCC.
- Bug fixes to ospSetRegion() implementation for arbitrarily shaped
  regions and setting large volumes in a single call.
- Bug fix for geometries with invalid bounds; fixes streamline and
  sphere rendering in some scenes.
- Fixed bug in depth buffer generation.

#### Changes in v0.8.0:

- Incorporated early version of a new Qt-based viewer to eventually
  unify (and replace) the existing simpler GLUT-based viewers.
- Added new path tracing renderer (ospray/render/pathtracer),
  roughly based on the Embree sample path tracer.
- Added new features to the volume renderer:
    - Gradient shading (lighting)
    - Implicit isosurfacing
    - Progressive refinement
    - Support for regular grids, specified with the "gridOrigin" and
      "gridSpacing" parameters.
    - New "shared_structured_volume" volume type that allows voxel
      data to be provided by applications through a shared data buffer.
    - New API call to set subregions of volume data (ospSetRegion()).
- Added a subsampling-mode, enabled with a negative "spp" parameter;
  the first frame after scene changes is rendered with reduced
  resolution, increasing interactivity.
- Added multi-target ISA support; OSPRay will now select the appropriate
  ISA at run time.
- Added support for the Stanford SEP file format to the seismic module.
- Added "--osp:numthreads <n>" command line option to restrict  the
  number of threads OSPRay creates.
- Various bug fixes, cleanups and documentation updates throughout the
  codebase.

#### Changes in v0.7.2:

- Build fixes for older versions of GCC and Clang
- Fixed time series support in ospVolumeViewer
- Corrected memory management for shared data buffers
- Updated to ISPC 1.8.1
- Resolved issue in XML parser
