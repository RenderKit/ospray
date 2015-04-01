OSPRay
======

This is release v0.8.0 of OSPRay. Changes since the 0.7.x branch:

- Incorporated early version of a new, QT-based viewer to eventually 
  unify (and replace) the exising simpler glut-based viewers.
- Added new path trace renderer (ospray/render/pathtrace),
  roughly based on the embree sample path tracer.
- Various cleanups and documentation updates throughout the codebase.
- Added a subsampling-mode, enabled with a negative "--spp" parameter;
  the first frame after scene changes is rendered with reduced
  resolution, increasing interactivity.

For more information, visit http://www.ospray.org.


Release History
---------------

Changes in v0.7.2

- Build fixes for older versions of GCC and Clang
- Fixed time series support in ospVolumeViewer
- Corrected memory management for shared data buffers
- Updated to ISPC 1.8.1
- Resolved issue in XML parser
