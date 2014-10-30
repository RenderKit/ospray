//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#pragma once

#include "QOSPRayWindow.h"
#include <QtGui>
#include <string>
#include <vector>

class TransferFunctionEditor;

class VolumeViewer : public QMainWindow {

Q_OBJECT

public:

    //! Constructor.
    VolumeViewer(const std::vector<std::string> &filenames);

    //! Destructor.
   ~VolumeViewer() {};

    //! Get the OSPRay output window.
    QOSPRayWindow *getWindow() { return(osprayWindow); }

    //! Get the transfer function editor.
    TransferFunctionEditor *getTransferFunctionEditor() { return(transferFunctionEditor); }

    //! Select the model to be displayed.
    void setModel(size_t index) { ospSetObject(renderer, "model", models[index]);  ospCommit(renderer);  osprayWindow->setRenderingEnabled(true); }

    //! A string description of this class.
    std::string toString() const { return("VolumeViewer"); }

public slots:

    //! Toggle auto-rotation of the view.
    void autoRotate(bool set);

    //! Set auto-rotation rate
    void setAutoRotationRate(float rate) { autoRotationRate = rate; }

    //! Draw the model associated with the next time step.
    void nextTimeStep() { static size_t index = 0;  index = (index + 1) % models.size();  setModel(index); }

    //! Toggle animation over the time steps.
    void playTimeSteps(bool animate) { if (animate == true) playTimeStepsTimer.start(2000);  else playTimeStepsTimer.stop(); }

    //! Add a slice to the volume, optionally from file.
    void addSlice(std::string filename = std::string());


    //! Re-commit all OSPRay volumes.
    void commitVolumes() { for(size_t i=0; i<volumes.size(); i++) ospCommit(volumes[i]); }

    //! Force the OSPRay window to be redrawn.
    void render() { if (osprayWindow != NULL) osprayWindow->updateGL(); }

protected:

    //! OSPRay models.
    std::vector<OSPModel> models;

    //! OSPRay volumes.
    std::vector<OSPVolume> volumes;

    //! OSPRay renderer.
    OSPRenderer renderer;

    //! OSPRay transfer function.
    OSPTransferFunction transferFunction;

    //! OSPRay light.
    OSPLight light;

    //! The OSPRay output window.
    QOSPRayWindow *osprayWindow;

    //! The transfer function editor.
    TransferFunctionEditor *transferFunctionEditor;

    //! Layout for slice widgets.
    QVBoxLayout sliceWidgetsLayout;

    //! Auto-rotate button.
    QAction * autoRotateAction;

    //! Auto-rotation rate
    float autoRotationRate;

    //! Timer for use when stepping through multiple models.
    QTimer playTimeStepsTimer;

    //! Print an error message.
    void emitMessage(const std::string &kind, const std::string &message) const
        { std::cerr << "  " + toString() + "  " + kind + ": " + message + "." << std::endl; }

    //! Error checking.
    void exitOnCondition(bool condition, const std::string &message) const
        { if (!condition) return;  emitMessage("ERROR", message);  exit(1); }

    //! Load an OSPRay model from a file.
    void importObjectsFromFile(const std::string &filename);

    //! Create and configure the OSPRay state.
    void initObjects(const std::vector<std::string> &filenames);

    //! Create and configure the user interface widgets and callbacks.
    void initUserInterfaceWidgets();

};

