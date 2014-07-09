
#include "VolumeViewer.h"
#include "TransferFunctionEditor.h"

VolumeViewer::VolumeViewer() : renderer_(NULL), volume_(NULL), transferFunction_(NULL), osprayWindow_(NULL)
{
    // default window size
    resize(1024, 768);

    // create renderer for volume viewer
    ospLoadModule("dvr");
    renderer_ = ospNewRenderer("dvr_ispc");

    if(!renderer_)
        throw std::runtime_error("could not create renderer type 'dvr_ispc'");

    // create transfer function
    createTransferFunction();

    // create the OSPRay window and set it as the central widget, but don't let it start rendering until we're done with setup...
    osprayWindow_ = new QOSPRayWindow(renderer_);
    setCentralWidget(osprayWindow_);

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
    volume_ = ospNewVolume(volumeType.c_str());
    if (!volume_) 
      throw std::runtime_error("could not create volume type '"+volumeType+"'");

    ospSetString(volume_, "filename", filename.c_str());
    ospSet3i(volume_, "dimensions", dimensions.x, dimensions.y, dimensions.z);

    // assign the transfer function
    ospSetParam(volume_, "transferFunction", transferFunction_);

    // finally, commit the volume.
    ospCommit(volume_);

    // and set the volume on the renderer
    ospSetParam(renderer_, "volume", volume_);

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

void VolumeViewer::createTransferFunction()
{
    // create transfer function
    transferFunction_ = ospNewTransferFunction("TransferFunctionPiecewiseLinear");

    if(!transferFunction_)
        throw std::runtime_error("could not create transfer function type 'TransferFunctionPiecewiseLinear'");

    ospCommit(transferFunction_);
}
