//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include <algorithm>
#include "VolumeViewer.h"
#include "TransferFunctionEditor.h"
#include "LightEditor.h"

VolumeViewer::VolumeViewer(const std::vector<std::string> &filenames) : renderer(NULL), transferFunction(NULL), osprayWindow(NULL) {

    //! Default window size.
    resize(1024, 768);

    //! Create an OSPRay renderer.
    renderer = ospNewRenderer("raycast_volume_renderer");  exitOnCondition(renderer == NULL, "could not create OSPRay renderer object");

    //! Create an OSPRay window and set it as the central widget, but don't let it start rendering until we're done with setup.
    osprayWindow = new QOSPRayWindow(renderer);  setCentralWidget(osprayWindow);

    //! Set the window bounds based on the OSPRay world bounds (always [(0,0,0), (1,1,1)) for volumes).
    osprayWindow->setWorldBounds(osp::box3f(osp::vec3f(0.0f), osp::vec3f(1.0f)));

    //! Create an OSPRay light source.
    light = ospNewLight(NULL, "DirectionalLight");  ospSet3f(light, "direction", 1.0f, -2.0f, -1.0f);  ospSet3f(light, "color", 1.0f, 1.0f, 1.0f);

    //! Set the light source on the renderer.
    ospCommit(light);  ospSetData(renderer, "lights", ospNewData(1, OSP_OBJECT, &light));

    //! Create an OSPRay transfer function.
    transferFunction = ospNewTransferFunction("piecewise_linear");  exitOnCondition(transferFunction == NULL, "could not create OSPRay transfer function object");

    //! Configure the user interface widgets and callbacks.
    initUserInterfaceWidgets();

    //! Commit the transfer function only after the initial colors and alphas have been set (workaround for Qt signalling issue).
    ospCommit(transferFunction);

    //! Create and configure the OSPRay state.
    initObjects(filenames);

    //! Show the window.
    show();

}

void VolumeViewer::importObjectsFromFile(const std::string &filename) {

    //! Create an OSPRay model.
    OSPModel model = ospNewModel();

    //! Load OSPRay objects from a file.
    OSPObjectCatalog catalog = ospImportObjects(filename.c_str());

    //! For now we set the same transfer function on all volumes.
    for (size_t i=0 ; catalog->entries[i] ; i++) if (catalog->entries[i]->type == OSP_VOLUME) ospSetObject(catalog->entries[i]->object, "transferFunction", transferFunction);

    //! Add the loaded volume(s) to the model.
    for (size_t i=0 ; catalog->entries[i] ; i++) if (catalog->entries[i]->type == OSP_VOLUME) ospAddVolume(model, (OSPVolume) catalog->entries[i]->object);

    //! Keep vector of all loaded volume(s).
    for (size_t i=0 ; catalog->entries[i] ; i++) if (catalog->entries[i]->type == OSP_VOLUME) volumes.push_back((OSPVolume) catalog->entries[i]->object);

    //! Commit the OSPRay object state.
    ospCommitCatalog(catalog);  ospCommit(model);  models.push_back(model);

}

void VolumeViewer::initObjects(const std::vector<std::string> &filenames) {

    //! Load OSPRay objects from files.
    for (size_t i=0 ; i < filenames.size() ; i++) importObjectsFromFile(filenames[i]);

}

void VolumeViewer::initUserInterfaceWidgets() {

    //! Add the "next timestep" widget and callback.
    QToolBar *toolbar = addToolBar("toolbar");
    QAction *nextTimeStepAction = new QAction("Next timestep", this);
    connect(nextTimeStepAction, SIGNAL(triggered()), this, SLOT(nextTimeStep()));
    toolbar->addAction(nextTimeStepAction);

    //! Add the "play timesteps" widget and callback.
    QAction *playTimeStepsAction = new QAction("Play timesteps", this);
    playTimeStepsAction->setCheckable(true);
    connect(playTimeStepsAction, SIGNAL(toggled(bool)), this, SLOT(playTimeSteps(bool)));
    toolbar->addAction(playTimeStepsAction);

    //! Create the transfer function editor dock widget, this widget modifies the transfer function directly.
    QDockWidget *transferFunctionEditorDockWidget = new QDockWidget("Transfer Function Editor", this);
    TransferFunctionEditor *transferFunctionEditor = new TransferFunctionEditor(transferFunction);
    transferFunctionEditorDockWidget->setWidget(transferFunctionEditor);
    addDockWidget(Qt::LeftDockWidgetArea, transferFunctionEditorDockWidget);

    //! Create the light editor dock widget, this widget modifies the light directly.
    QDockWidget *lightEditorDockWidget = new QDockWidget("Light Editor", this);
    LightEditor *lightEditor = new LightEditor(light);
    lightEditorDockWidget->setWidget(lightEditor);
    addDockWidget(Qt::LeftDockWidgetArea, lightEditorDockWidget);

    //! Connect the "play timesteps" timer.
    connect(&playTimeStepsTimer, SIGNAL(timeout()), this, SLOT(nextTimeStep()));

    //! Connect the Qt event signals and callbacks.
    connect(transferFunctionEditor, SIGNAL(transferFunctionChanged()), this, SLOT(commitVolumes()));
    connect(transferFunctionEditor, SIGNAL(transferFunctionChanged()), this, SLOT(render()));
    connect(lightEditor, SIGNAL(lightChanged()), this, SLOT(render()));

}

