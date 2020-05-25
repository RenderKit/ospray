Projects that make use of OSPRay
================================

This page gives a brief (and incomplete) list of other projects that
make use of OSPRay, as well as a set of related links to other projects
and related information.

If you have a project that makes use of OSPRay and would like this to be
listed here, please let us know.

VTK, ParaView and VisIt
-----------------------
VTK and Paraview 5.x+ have direct integrations for OSPRay rendering, as
has [VisIt 3.x+](https://wci.llnl.gov/simulation/computer-codes/visit).
Code and documentation are available on Kitware's instance of
[GitLab](https://gitlab.kitware.com/paraview/paraview). Furthermore,
interested users can also access Kitware's [tutorial on using
OSPRay](https://www.paraview.org/Wiki/Intel_HPC_Dev_Con_ParaView_and_OSPRay_Tutorial).
For information on using the path tracer inside ParaView, see [this
Kitware blog
post](https://blog.kitware.com/virtual-tour-and-high-quality-visualization-with-paraview-5-6-ospray/).

HdOSPRay
--------
HdOSPRay is a rendering plugin for the Hydra rendering layer in USD.
For more information see the project's [page on GitHub](https://github.com/ospray/hdospray).

OSPRay Studio
--------
[Studio](https://github.com/ospray/ospray_studio) is the
    OSPRay team's lightweight visualization application used to showcase
    the latest features of OSPRay. 

"osprey" Rhino Viewport Plugin
--------
[osprey](https://github.com/darbyjohnston/Osprey) is a rendering plugin written by Darby Johnston
for Rhino3D that greatly accelerates rendering over the built-in renderer.

BLOSPRAY
--------
[BLOSPRAY](https://github.com/surfsara-visualization/blospray) is a rendering plugin
for Blender that uses OSPRay for fast previews and final path traced rendering.

Tapestry
--------
Tapestry is a microservice for delivering scientific visualization on
the cloud. With OSPRay, Tapestry can provide interactive visualization
of large datasets accessible to many users throughout the web ecosystem.
See the [demo page](https://seelabutk.github.io/tapestry/) and the
project's [page on GitHub](https://github.com/seelabutk/tapestry) for
more information.

Megamol
-------
[Megamol](https://megamol.org/2018/07/02/megamol-at-isc-2018/) is a
molecular dynamics framework.  With OSPRay, it can render billions of
particles interactively.

VESTEC
------
[![VESTEC](images/VESTEC-Logo-web.png){style="display:inline"}](https://vestec-project.eu/)

[VESTEC](https://vestec-project.eu/) – Visual Exploration and Sampling
Toolkit for Extreme Computing – is an European funded project that
builds a flexible toolchain to combine multiple data sources,
efficiently extract essential features, enable flexible scheduling and
interactive supercomputing, and realize 3D visualization environments
(partly using OSPRay) for interactive explorations by stakeholders and
decision makers.

VMD
---
VMD is a popular molecular dynamics visualization package, where OSPRay
is integrated as one its renderers. For more information see VMD's
[release notes](https://www.ks.uiuc.edu/Research/vmd/current/).

pCon.planner
---
[pCon.planner](https://pcon-planner.com/en/) is an architectural design and rendering applicaiton that
utilizes OSPRay for path traced photorealistic renderings.



Projects that are closely related to OSPRay
===========================================

-   The Intel® [Embree] Ray Tracing Kernel Framework
-   Intel [Open Image Denoise], a high-performance denoising library
-   OSPRay's "sister project" [OpenSWR](http://openswr.org/)
    (**o**pen **s**oft**w**are **r**asterizer), a high-performance,
    highly-scalable, software OpenGL implementation that runs entirely
    on CPUs. OpenSWR is now included in [Mesa](http://mesa3d.org/).
-   The [Intel SPMD Program Compiler](http://ispc.github.io) (ISPC)
