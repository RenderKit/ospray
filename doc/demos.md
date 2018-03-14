OSPRay Demos
============

This page serves as a repository for "ready-to-run" demos for OSPRay
(currently, most demos are intended for OSPRay v<OSPRAY_VERSION>). If you do not yet
have a version of OSPRay installed on your system, you can follow these
instructions for [getting OSPRay].

Sponza
----------------------------------

[![](demos/sponza_small.jpg)](demos/sponza.jpg)

*Model courtesy Carson Brownlee, Morgan McGuire, Crytek*

### Demo Highlights

-   Crytek Sponza scene -- a common scene for showcasing global
    illumination (model from [McGuire Graphics Data]).

### Instructions

-   *IMPORTANT:* This demo currently requires a build from GitHub with `OSPRAY_SG_OPENIMAGEIO=ON`
-   Download
    [crytek-sponza.tgz](http://www.sdvis.org/ospray/download/demos/crytek-sponza.tgz)
    and [light probe] in pfm-format.
-   Run via\
    `./ospExampleViewer [path/to/sponza.obj] -vp 667.492554 186.974228 76.008301 -vu 0.000000 1.000000 0.000000 -vi 84.557503 188.199417 -38.148270 -r pt -sg:sun:direction=-.3,-1,.1 -sg:sun:intensity=12 --hdri-light [path/to/rnl_probe.pfm]`


San Miguel
----------------------------------

[![](demos/san-miguel_small.jpg)](demos/san-miguel.jpg)

*Model courtesy Carson Brownlee, Morgan McGuire, Guillermo M. Leal
Llaguno*

### Demo Highlights

-   A quaint cafe in San Miguel's historic district. This is a common
    scene for showcasing global illumination. (Model from [McGuire
    Graphics Data]).

### Instructions

-   *IMPORTANT:* This demo currently requires a build from GitHub with `OSPRAY_SG_OPENIMAGEIO=ON`
-   Download
    [sanm.zip](http://www.sdvis.org/ospray/download/demos/sanm.zip)
    and [light probe] in pfm-format.
-   Run via\
    `./ospExampleViewer [path/to/sanm.obj] -vp 22.958788 3.204613 2.712676 -vu 0.000000 1.000000 0.000000 -vi 12.364944 0.176316 4.009342 -sg:sun:intensity=4.0 -sg:sun:direction=0,-1,0 -sg:bounce:intensity=0.0 --hdri-light [path/to/rnl_probe.pfm] -sg:hdri:intensity=1.25 -r pt`

Sibenik Cathedral
----------------------------------

[![](demos/sibenik_small.jpg)](demos/sibenik.jpg)

*Model courtesy Morgan McGuire, Kenzie Lamar,  Aleksander Stompel*

### Demo Highlights

-   A large cathedral model commonly used for showcasing global
    illumination. (Original model from [McGuire Graphics Data]).

### Instructions

-   *IMPORTANT:* This demo currently requires a build from GitHub with `OSPRAY_SG_OPENIMAGEIO=ON`
-   Download
    [sibenik.tgz](http://www.sdvis.org/ospray/download/demos/sibenik.tgz)
    and [light probe] in pfm-format.
-   Run via\
    `./ospExampleViewer sibenik.obj -vp -17.734447 -13.788272 3.443677 -vu 0.000000 1.000000 0.000000 -vi -2.789550 -10.993323 0.331822`

Animated Woody
----------------------------------

[![](demos/sponza_animated_small.jpg)](demos/sponza_animated.jpg)

*Model courtesy Carson Brownlee, Ingo Wald, Morgan McGuire, Crytek*

### Demo Highlights

-   Animated Wooden Doll walking in the Sponza model (from [McGuire
    Graphics Data]).

### Instructions

-   *IMPORTANT:* This demo currently requires a build from GitHub with `OSPRAY_SG_OPENIMAGEIO=ON`
-   Download
    [crytek-sponza.tgz](http://www.sdvis.org/ospray/download/demos/crytek-sponza.tgz)
    and
    [wooddoll.tgz](http://www.sdvis.org/ospray/download/demos/wooddoll.tgz).
-   Run via\
    `./ospExampleViewer [path/to/crytek-sponza/sponza.obj] --renderer scivis -sun:direction=-.3,-1,-.04 -sg:sun:intensity=3 -vp 667.492554 186.974228 76.008301 -vu 0.000000 1.000000 0.000000 -vi 84.557503 188.199417 -38.148270 --translate 250 -11 0 --scale 300 300 300 --animation [path/to/wooddoll/wooddoll*.obj] -sg:aoSamples=0`
-   Controls: +/- increase and decrease animation speed. Space bar
    pauses.

"Magnetic Reconnection" Volume Rendered
---------------------------------------

[![](demos/MagneticReconnection/magnetic-512-volume-thumbnail.jpg)](demos/MagneticReconnection/magnetic-512-volume.png)

*Model courtesy Bill Daughton (LANL) and Berk Geveci (KitWare). Please
acknowledge via [this paper](http://arxiv.org/abs/1405.4040).*

### Demo Highlights

-   Volume rendering of the 512^3^ Magnetic Reconnection volume data.
-   Rendered using OSPRay's ray cast volume renderer
    (`raycast_volume_renderer`).

### Instructions

-   Download
    [magnetic-512-volume.tar.bz2](http://www.sdvis.org/ospray/download/demos/MagneticReconnection/magnetic-512-volume.tar.bz2).
-   Run via\
    `./ospExampleViewerSg magnetic-512-volume.osp`


290M triangle "Richtmyer-Meshkov" Iso-Surface
---------------------------------------------

[![](demos/llnl-250/llnl-250-small.png)](demos/llnl-250/llnl-250.png)

*Model courtesy Lawrence-Livermore National Labs (LLNL).*

### Demo Highlights

-   Iso-surface of the 2048^3^ Richtmyer-Meshkov Instability simulation
    performed by LLNL, iso-surface extracted through ParaView.
-   Detailed iso-surface has 290 million triangles.
-   Rendered using OSPRay's Ambient Occlusion renderer (`ao4`).

### Instructions

-   Download
    [llnl-2048-iso.tar.bz2](http://www.sdvis.org/ospray/download/demos/LLNLRichtmyerMeshkov/llnl-2048-iso.tar.bz2).
-   Run via\
    `./ospExampleViewer llnl-2048-iso.xml`


CSAFE Heptane Gas Dataset
-------------------------

[![](demos/CSAFEHeptane/csafe-heptane-302-volume-thumbnail.jpg)](demos/CSAFEHeptane/csafe-heptane-302-volume.png)

*Model courtesy of the Center for the Simulation of Accidental Fires and
Explosions (CSAFE) at the Scientific Computing and Imaging Institute
(SCI), University of Utah.*

### Demo Highlights

-   Volume rendering of a 302^3^ data set containing a single time step
    from a computational simulation of the combustion of heptane gas.
-   Rendered using OSPRay's ray cast volume renderer
    (`raycast_volume_renderer`).

### Instructions

-   Download
    [csafe-heptane-302-volume.tar.bz2](http://www.sdvis.org/ospray/download/demos/CSAFEHeptane/csafe-heptane-302-volume.tar.bz2).
-   Run via\
    `./ospExampleViewerSg csafe-heptane-302-volume.osp`


"TACC Isotropic Turbulence" Volume Rendered
-------------------------------------------

[![](demos/TACCIsotropicTurbulence/tacc-turbulence-256-volume-thumbnail.jpg)](demos/TACCIsotropicTurbulence/tacc-turbulence-256-volume.png)

*Model courtesy of the Texas Advanced Computing Center (TACC), Texas A&M
University, and Georgia Tech.*

### Demo Highlights

-   Volume rendering of a 256^3^ data set containing a single time step
    from a computational simulation of isotropic turbulence.
-   Rendered using OSPRay's ray cast volume renderer
    (`raycast_volume_renderer`).

### Instructions

-   Download
    [tacc-turbulence-256-volume.tar.bz2](http://www.sdvis.org/ospray/download/demos/TACCIsotropicTurbulence/tacc-turbulence-256-volume.tar.bz2).
-   Run via\
    `./ospExampleViewerSg tacc-turbulence-256-volume.osp`


"FIU" Ground Water Simulation
-----------------------------

[![](demos/fiu/fiu-ao-small.jpg)](demos/fiu/fiu-ao.png)

*Model courtesy Texas Advanced Computing Center (TACC) and Florida
International University. Please obtain TACC/FIU's permission before
using this model.*

### Demo Highlights

-   Dataset captured from VTK-based ParaView through TACC's GLURay tool.
-   Streamlines in this data set are actually tessellated. OSPRay could
    do them natively, but ParaView tessellates internally.
-   Model size roughly 8 million triangles
-   Rendered via OSPRay's Ambient Occlusion based renderer. To see a
    comparison with/without Ambient occlusion, [see this
    page](fiu_comparison.html).

### Instructions

-   Download
    [fiu-groundwater.tar.bz2](http://www.sdvis.org/ospray/download/demos/FIUGroundWater/fiu-groundwater.tar.bz2).
-   Run via\
    `./ospExampleViewer fiu-groundwater.xml --renderer scivis`
-   Run via OBJ renderer rather than Ambient Occlusion:\
    `./ospExampleViewer fiu-groundwater.xml --sg:sun:direction=-1.5,-1,-1`


"XFrog Forest" - A 1.7 billion triangle forest model
----------------------------------------------------

[![](demos/xfrog/xfrog-small.jpg)](demos/xfrog/xfrog.png)

*Model courtesy Oliver Deussen, University of Konstanz; modelled via the
[XFrog](http://www.xfrog.com) tool. Please acknowledge accordingly when
using this model.*

### Demo Highlights

-   1.7 billion triangles (377,266 instances of 69 different kinds of
    plants, for a total of 1,679,235,546 triangles after instantiation).
-   Additional detail through high-resolution alpha "stencil" textures
    to stencil detailed leaf shapes out of already highly detailed
    geometry.
-   Requires many rays per pixel to handle the (high!) number of
    transparency layers.

### Instructions

-   Download
    [xfrog-forest.tar.bz2](http://www.sdvis.org/ospray/download/demos/XFrogForest/xfrog-forest.tar.bz2).
-   Run via\
    `./ospExampleViewer xfrog-forest.xml`


"NASA Streamlines"
------------------

[![](demos/nasa-streamlines/nasa-streamlines-small.jpg)](demos/nasa-streamlines/nasa-streamlines.png)

*Model courtesy Timothy Sandstrom, NASA.*

### Demo Highlights

-   Model of a magnetic field on the sun, visualized through a
    attribute-mapped iso-surface of the B-field, plus a set of
    streamlines.
-   The streamlines are not tessellated, but OSPRay's actual
    "`streamlines`" primitive (essentially, a set of smoothly connected
    cylinders), which makes for a very memory-efficient representation.
-   Ambient occlusion greatly helps in understanding the shape of the
    streamlines (when rendered with local-only shading, it looks like a
    "lint ball").

### Instructions

-   Download
    [NASA-B-field-sun.osx](http://www.sdvis.org/ospray/download/demos/NASA-B-field-sun/NASA-B-field-sun.osx).
-   Run via\
    `./ospExampleViewer NASA-B-field-sun.osx`



[McGuire Graphics Data]: http://graphics.cs.williams.edu/data/meshes.xml "McGuire, Computer Graphics Archive, Aug 2011"
[light probe]: http://www.pauldebevec.com/Probes/rnl_probe.pfm "RNL Light Probe Image"
