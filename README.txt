/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

Dependencies
  cmake 2.4+
  freeglut

Generating make files:
  Create a new directory for the build files and cd into it
  Run 'ccmake <path to ospray>' to generate the necessary files
  Run make to build the software

Using the model viewer:
  The generated model viewer is called 'ospModelViewer'

  usage: ospModelViewer <arguments> <model path>

  The following arguments may be useful:
    --osp:debug : Runs in debug mode, useful for tracking text printed to std out
    --osp:coi   : Runs on Intel MIC coprocessors, MIC and COI must be enabled via cmake
    --module <module name> : Load a given Ospray module
    --renderer <renderer name> : Use a given renderer instead of the default
    --always-redraw : Force the model viewer to continually repaint the screen
    --spp : Set the number of samples per-pixel per-frame
    --sun-dir x y z: Set the sunlight to be coming from the direction described by vector {x,y,z}

  Manipulator modes:
    The model viewer has two modes that can be used to view a given model. An
    orbiting camera, and a free flying camera. By default we start with the
    orbiting camera.

  Keyboard and mouse commands:
    Orbit :
    W : Orbit camera up
    A : Orbit camera left
    S : Orbit camera down
    D : Orbit camera right
    F : Switch to free flight mode (move mode)
    Left click & drag : orbit camera
    Shift + Left click : Set a new inspection point

    Free Flight:
    W : Move camera forward
    A : Pan camera left
    S : Move camera backwards
    D : Pan camera right
    I : Switch to Orbit mode

    Common:
    Q : Quit the program
    f : Switch to full screen
    r : Reset the view to the startup position
    R : Enter 'always-redraw' mode
    Right click & drag : zoom
    S : Toggle shadows
    D : Toggle between depth buffer and color buffer

Using the streamline viewer:
  The generated streamline viewer is called 'ospStreamlineViewer'

  usage: ospStreamlineViewer <arguments> <model path>

  The following arguments are valid for the streamline viewer:
  --module : Load a given Ospray module
  --renderer : Use a given renderer
  --radius : Set the radius of the rendered streamlines


  Keyboard and mouse commands:
    Orbit :
    W : Orbit camera up
    A : Orbit camera left
    S : Orbit camera down
    D : Orbit camera right
    F : Switch to free flight mode (move mode)
    Left click & drag : orbit camera

    Free Flight:
    W : Move camera forward
    A : Pan camera left
    S : Move camera backwards
    D : Pan camera right
    I : Switch to Orbit mode

    Common:
    Q : Quit the program
    Right click & drag : zoom
    S : Toggle shadows
