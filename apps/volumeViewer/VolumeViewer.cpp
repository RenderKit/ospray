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

VolumeViewer::VolumeViewer(const float dt) : renderer_(NULL), transferFunction_(NULL), osprayWindow_(NULL) {

    //! Default window size.
    resize(1024, 768);

    QToolBar *toolbar = addToolBar("toolbar");

    //! Add the "next timestep" widget and callback.
    QAction *nextTimestepAction = new QAction("Next timestep", this);
    connect(nextTimestepAction, SIGNAL(triggered()), this, SLOT(nextTimestep()));
    toolbar->addAction(nextTimestepAction);

    //! Add the "play timesteps" widget and callback.
    QAction *playTimestepsAction = new QAction("Play timesteps", this);
    playTimestepsAction->setCheckable(true);
    connect(playTimestepsAction, SIGNAL(toggled(bool)), this, SLOT(playTimesteps(bool)));
    toolbar->addAction(playTimestepsAction);

    //! Connect the "play timesteps" timer.
    connect(&playTimestepsTimer_, SIGNAL(timeout()), this, SLOT(nextTimestep()));

    //! Create an OSPRay renderer for the volume viewer.
    renderer_ = ospNewRenderer("dvr_ispc");  if (renderer_ == NULL) throw std::runtime_error("could not create renderer type 'dvr_ispc'");

    //! If no "dt" value is given, the renderer sets a default based on the volume bounds.
    if (dt != 0.0f) ospSet1f(renderer_, "dt", dt);

    //! Create the transfer function.
    createTransferFunction();

    //! Create the OSPRay window and set it as the central widget, but don't let it start rendering until we're done with setup.
    osprayWindow_ = new QOSPRayWindow(renderer_);
    setCentralWidget(osprayWindow_);

    //! Set the window bounds based on the OSPRay world bounds (this is always [(0,0,0), (1,1,1)] for volumes).
    osprayWindow_->setWorldBounds(osp::box3f(osp::vec3f(0.0f), osp::vec3f(1.0f)));

    //! Create the transfer function editor dock widget, this widget modifies the transfer function directly.
    QDockWidget *transferFunctionEditorDockWidget = new QDockWidget("Transfer Function Editor", this);
    TransferFunctionEditor *transferFunctionEditor = new TransferFunctionEditor(transferFunction_);
    transferFunctionEditorDockWidget->setWidget(transferFunctionEditor);
    addDockWidget(Qt::LeftDockWidgetArea, transferFunctionEditorDockWidget);

    //! Connect the Qt event signals and callbacks.
    connect(transferFunctionEditor, SIGNAL(transferFunctionChanged()), this, SLOT(render()));

    //! Show the window.
    show();

}

void VolumeViewer::initVolumeFromFile(const std::string &filename) {

    //! Currently only bricked volume types are supported.
    std::string volumeType = "bricked";  OSPVolume volume = ospNewVolume(volumeType.c_str());

    //! The requested volume type may not be known to OSPRay.
    if (!volume) throw std::runtime_error("could not create volume type '" + volumeType + "'");

    //! All volumes are assumed to be self-describing (e.g. via a header file).
    ospSetString(volume, "Filename", filename.c_str());

    //! Associate the transfer function with the volume.
    ospSetParam(volume, "TransferFunction", transferFunction_);

    //! Commit that bad boy.
    ospCommit(volume);

    //! Add the volume to the vector of volumes.
    volumes_.push_back(volume);

}

void VolumeViewer::setVolume(size_t index) {

    // and set the volume on the renderer
    ospSetParam(renderer_, "volume", volumes_[index]);

    // at this point all required parameters of the renderer are set, so commit it
    ospCommit(renderer_);

    // enable rendering in the OSPRay window
    osprayWindow_->setRenderingEnabled(true);

}

void VolumeViewer::render()
{
    if(osprayWindow_ != NULL)
    {
        // force the OSPRay window to render
        osprayWindow_->updateGL();
    }
}

void VolumeViewer::nextTimestep()
{

    static size_t index = 0;  index = (index + 1) % volumes_.size();
    setVolume(index);
}

void VolumeViewer::playTimesteps(bool set)
{
    if(set == true)
    {
        // default 2 second delay
        playTimestepsTimer_.start(2000);
    }
    else
    {
        playTimestepsTimer_.stop();
    }
}

void VolumeViewer::createTransferFunction()
{
    // create transfer function
    transferFunction_ = ospNewTransferFunction("TransferFunctionPiecewiseLinear");

    if(!transferFunction_)
        throw std::runtime_error("could not create transfer function type 'TransferFunctionPiecewiseLinear'");

    ospCommit(transferFunction_);
}
