Documentation
=============

The following [API documentation][OSPRayReadme] of OSPRay can also be
found as a [pdf document][OSPRayReadme].

For a deeper explanation of the concepts, design, features and
performance of OSPRay also have a look at the IEEE Vis 2016 paper
"[OSPRay – A CPU Ray Tracing Framework for Scientific
Visualization](http://www.sdvis.org/ospray/download/talks/IEEEVis2016_OSPRay_paper.pdf)"
(49MB, or get the [smaller
version](http://www.sdvis.org/ospray/download/talks/IEEEVis2016_OSPRay_paper_small.pdf)
1.8MB). The [slides of the
talk](http://www.sdvis.org/ospray/download/talks/IEEEVis2016_OSPRay_talk.pdf)
(5.2MB) are also available.

Finding OSPRay with CMake
-------------------------

Client applications using OSPRay can find it with CMake's ```find_package()```
command. For example:

    find_package(ospray 1.8.0 REQUIRED)

finds OSPRay according to where ```ospray_DIR``` points to OSPRay's config file:
```osprayConfig.cmake```. This file can be found in\
`${install_location}/[lib|lib64]/cmake`, where ```ospray_DIR``` can be specified
as either an environment variable or CMake variable in the client CMake
build. Once OSPRay is found, the following is all that is required to use
OSPRay:

    target_link_libraries(${client_target} ospray::ospray)

This will automatically propogate all required include paths, linked libraries,
and compiler definitions to the client CMake target (either an executable or
library).

Advanced users may want to link to additional targets which are exported in
OSPRay's CMake config, which includes all installed modules. All targets
built with OSPRay are exported in the ```ospray::``` namespace, therefore all
targets locally used in the OSPRay source tree can be accessed from an
install. For example, ```ospray_common``` can be consumed directly via the
```ospray::ospray_common``` target. All targets have their libraries,
includes, and definitions attached to them for public consumption (please
report bugs if one is found!).
