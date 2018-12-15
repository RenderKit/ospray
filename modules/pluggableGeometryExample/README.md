PluggableGeometryExample: Module that demonstrates how to use the Module Concept to extend OSPRay with a new geometry type
==========================================================================================================================


This module serves as a simple example of

1) how to define a new ospray geometry type and 'plug' this into ospray. In particular, how this geometry can use its "commit()" method to query data the user has added to it.

2) how to use embree and ispc to implement this geometry type's intersect() and postIntersect()
   functoins

3) how to do this through a 'module' that is dynamically loadable; and

4) how to use this module - and geometry - from within your own applications

Using this example you should be able to write your own modules, with
your own geometry types. For the purpose of this geometry we have
chosen a intentionally very simple type of geometry - a bilinear
patch.

Notes:

- Names matter. E.g., for ospray to find the module "myModuleName" the
  module _has_ to build a library of name
  libospray_module_myModuleName.so. Similarly, a module that wants to
  define a new geometry type "MyGeometry" _has_ to have a
  OSPRAY_REGISTER_GEOMETRY(WhatEverYouCallItInYourImplementation,MyGeometry)
  macro invocation in one of its cpp files.

- This example only demonstrates how to write a _geometry_ type. It is
  similarly possible to write new renderers, new camera types, new
  volume types, etc.

Running the Example
===================

Build OSPRay (make sure that OSPRAY_MODULE_BILINEAR_PATCH is on!), then in your build directory, do a 

      ./ospExampleViewer --sg:module bilinear_patches ../modules/pluggableGeometryExample/examples/shadows.patches --renderer pt 


