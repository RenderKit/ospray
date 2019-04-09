Projects that make use of OSPRay
================================

This page gives a brief (and incomplete) list of other projects that
make use of OSPRay, as well as a set of related links to other projects
and related information.

If you have a project that makes use of OSPRay and would like this to be
listed here, please let us know.

VTK+ParaView
------------------------------------------

![](images/teaser_clouds.jpg)
VTK and Paraview 5.x+ have direct integrations for OSPRay rendering.  Code and documentation
are available on the kitware [gitlab site](https://gitlab.kitware.com/paraview/paraview).
Kitware hosts a [tutorial on using OSPRay](https://www.paraview.org/Wiki/Intel_HPC_Dev_Con_ParaView_and_OSPRay_Tutorial).
For information on using the path tracer inside ParaView, see [this blog post](https://blog.kitware.com/virtual-tour-and-high-quality-visualization-with-paraview-5-6-ospray/).

HdOSPRay
------------------------------------------
![](images/teaser_hdospray.jpg)
HdOSPRay is an OSPRay-based rendering plugin for the Hydra rendering layer in USD.
For more information see the [github page](https://github.com/ospray/hdospray).

Megamol
------------------------------------------
[Megamol](https://megamol.org/2018/07/02/megamol-at-isc-2018/) is a molecular dynamics framework.  With
OSPRay, it can render billions of particles interactively.  

VMD
------------------------------------------
VMD is a popular molecular dynamics visualization package.  OSPRay is integrated as one of the
renderers. For more information see the [release notes](https://www.ks.uiuc.edu/Research/vmd/current/).

StingRay
--------

[![](related_projects/stingray/stingray.jpg)](related_projects/stingray/stingray.jpg)

SURVICE Engineering's ["StingRay"
toolkit](http://www.rtvtk.org/~cgribble/research/papers/gribble14high.pdf)
makes use of Embree for simulating radio frequency, and uses OSPRay for
visualizing the results.


Projects that are closely related to OSPRay
===========================================

-   The [Embree](http://embree.github.io) Ray Tracing Kernel Framework
-   The [Intel® SPMD Program Compiler](http://ispc.github.io) (ISPC)
-   OSPRay's "sister project" is [OpenSWR](http://OpenSWR.github.io)
    (**o**pen **s**oft**w**are **r**asterizer), a new high-performance,
    highly-scalable, software OpenGL implementation that runs on host
    CPUs.

