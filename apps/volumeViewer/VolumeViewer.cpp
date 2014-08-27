/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "VolumeViewer.h"
#include "TransferFunctionEditor.h"
#include <algorithm>

VolumeViewer::VolumeViewer(const osp::vec3i &dimensions, const float dt) : renderer_(NULL), transferFunction_(NULL), osprayWindow_(NULL)
{
    // default window size
    resize(1024, 768);

    QToolBar * toolbar = addToolBar("toolbar");

    // add timestep controls
    QAction * nextTimestepAction = new QAction("Next timestep", this);
    connect(nextTimestepAction, SIGNAL(triggered()), this, SLOT(nextTimestep()));
    toolbar->addAction(nextTimestepAction);

    QAction * playTimestepsAction = new QAction("Play timesteps", this);
    playTimestepsAction->setCheckable(true);
    connect(playTimestepsAction, SIGNAL(toggled(bool)), this, SLOT(playTimesteps(bool)));
    toolbar->addAction(playTimestepsAction);

    // connect play timesteps timer slot
    connect(&playTimestepsTimer_, SIGNAL(timeout()), this, SLOT(nextTimestep()));

    // create renderer for volume viewer
    ospLoadModule("dvr");
    renderer_ = ospNewRenderer("dvr_ispc");

    // set the dt value; auto-set based on volume dimensions if dt == 0
    if (dt != 0.f) ospSet1f(renderer_, "dt", dt);
    else ospSet1f(renderer_, "dt", 1.f / float(std::max(std::max(dimensions.x, dimensions.y), dimensions.z)));

    if(!renderer_)
        throw std::runtime_error("could not create renderer type 'dvr_ispc'");

    // create transfer function
    createTransferFunction();

    // create the OSPRay window and set it as the central widget, but don't let it start rendering until we're done with setup...
    osprayWindow_ = new QOSPRayWindow(renderer_);
    setCentralWidget(osprayWindow_);

    // set world bounds on OSPRay window. for volumes this is always (0,0,0) to (1,1,1).
    osprayWindow_->setWorldBounds(osp::box3f(osp::vec3f(0.f), osp::vec3f(1.f)));

    // create transfer function editor dock widget
    // the transfer function editor will modify the transfer function directly
    QDockWidget * transferFunctionEditorDockWidget = new QDockWidget("Transfer Function Editor", this);
    TransferFunctionEditor * transferFunctionEditor = new TransferFunctionEditor(transferFunction_);
    transferFunctionEditorDockWidget->setWidget(transferFunctionEditor);
    addDockWidget(Qt::LeftDockWidgetArea, transferFunctionEditorDockWidget);

    // connect signals and slots
    connect(transferFunctionEditor, SIGNAL(transferFunctionChanged()), this, SLOT(render()));

    show();
}

void VolumeViewer::loadVolume(const std::string &filename, const osp::vec3i &dimensions, const std::string &format, const std::string &layout)
{
    std::string volumeType = layout + "_" + format;
    OSPVolume volume = ospNewVolume(volumeType.c_str());

    if (!volume) throw std::runtime_error("could not create volume type '" + volumeType + "'");

    ospSetString(volume, "filename", filename.c_str());
    ospSet3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);

    // assign the transfer function
    ospSetParam(volume, "transferFunction", transferFunction_);

    // finally, commit the volume.
    ospCommit(volume);

    // add the volume to the vector of volumes
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
