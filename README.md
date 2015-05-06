OSPRay
======

This is release v0.8.0 of OSPRay. Changes since the 0.7.x branch:

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

For more information, visit http://www.ospray.org.

Release History
---------------

Changes in v0.7.2

- Build fixes for older versions of GCC and Clang
- Fixed time series support in ospVolumeViewer
- Corrected memory management for shared data buffers
- Updated to ISPC 1.8.1
- Resolved issue in XML parser
