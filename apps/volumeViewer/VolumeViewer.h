// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "ospray/common/OSPCommon.h"
#include "QOSPRayWindow.h"
#include "SliceWidget.h"
#include <QtGui>
#include <string>
#include <vector>

class TransferFunctionEditor;
class IsosurfaceEditor;
class ProbeWidget;
class OpenGLAnnotationRenderer;

//! OSPRay model and its volumes / geometries
struct ModelState {

  ModelState(OSPModel model) : model(model) { }

  OSPModel model; //!< the OSPRay model

  std::vector<OSPVolume> volumes; //!< OSPRay volumes for the model
  std::vector<OSPGeometry> slices; //! OSPRay slice geometries for the model
  std::vector<OSPGeometry> isosurfaces; //! OSPRay isosurface geometries for the model
};

class VolumeViewer : public QMainWindow {

Q_OBJECT

public:

  //! Constructor.
  VolumeViewer(const std::vector<std::string> &objectFileFilenames, bool ownModelPerObject, bool showFrameRate, bool fullScreen, std::string writeFramesFilename);

  //! Destructor.
  ~VolumeViewer() {};

  //! Get the volume bounding box.
  ospray::box3f getBoundingBox() { return boundingBox; }

  //! Get the OSPRay output window.
  QOSPRayWindow *getWindow() { return osprayWindow; }

  //! Get the transfer function editor.
  TransferFunctionEditor *getTransferFunctionEditor() { return transferFunctionEditor; }

  //! Select the model (time step) to be displayed.
  void setModel(size_t index);

  //! A string description of this class.
  std::string toString() const { return("VolumeViewer"); }

public slots:

  //! Toggle auto-rotation of the view.
  void autoRotate(bool set);

  //! Set auto-rotation rate
  void setAutoRotationRate(float rate) { autoRotationRate = rate; }

  //! Draw the model associated with the next time step.
  void nextTimeStep() { modelIndex = (modelIndex + 1) % modelStates.size();  setModel(modelIndex);  render(); }

  //! Toggle animation over the time steps.
  void playTimeSteps(bool animate) { if (animate == true) playTimeStepsTimer.start(500);  else playTimeStepsTimer.stop(); }

  //! Add a slice to the volume from file.
  void addSlice(std::string filename);

  //! Add geometry from file.
  void addGeometry(std::string filename = std::string());

  //! Save screenshot.
  void screenshot(std::string filename = std::string());

  //! Re-commit all OSPRay volumes.
  void commitVolumes() { for(size_t i=0; i<modelStates.size(); i++) for(size_t j=0; j<modelStates[i].volumes.size(); j++) ospCommit(modelStates[i].volumes[j]); }

  //! Force the OSPRay window to be redrawn.
  void render() { if (osprayWindow != NULL) { osprayWindow->resetAccumulationBuffer(); osprayWindow->updateGL(); } }

  //! Enable / disable rendering of annotations.
  void setRenderAnnotationsEnabled(bool value);

  //! Set subsampling during interaction mode on renderer.
  void setSubsamplingInteractionEnabled(bool value) {
    ospSet1i(renderer, "spp", value ? -1 : 1);
    if(rendererInitialized) ospCommit(renderer);
  }

  //! Set gradient shading flag on all volumes.
  void setGradientShadingEnabled(bool value);

  //! Set sampling rate on all volumes.
  void setSamplingRate(double value);

  //! Set volume clipping box on all volumes.
  void setVolumeClippingBox(ospray::box3f value);

  //! Set slices on all volumes.
  void setSlices(std::vector<SliceParameters> sliceParameters);

  //! Set isosurfaces on all volumes.
  void setIsovalues(std::vector<float> isovalues);

protected:

  //! OSPRay object file filenames, one for each model / time step.
  std::vector<std::string> objectFileFilenames;
  bool ownModelPerObject; // create a separate model for each object (not not only for each file)

  //! OSPRay models and their volumes / geometries.
  std::vector<ModelState> modelStates;

  //! Active OSPRay model index (time step).
  size_t modelIndex;

  //! Bounding box of the (first) volume.
  ospray::box3f boundingBox;

  //! OSPRay renderer.
  OSPRenderer renderer;

  //! OSPRay renderer initialization state: set to true after renderer is committed.
  bool rendererInitialized;

  //! OSPRay transfer function.
  OSPTransferFunction transferFunction;

  //! OSPRay ambient light.
  OSPLight ambientLight;

  //! OSPRay directional light.
  OSPLight directionalLight;

  //! The OSPRay output window.
  QOSPRayWindow *osprayWindow;

  //! The OpenGL annotation renderer.
  OpenGLAnnotationRenderer *annotationRenderer;

  //! The transfer function editor.
  TransferFunctionEditor *transferFunctionEditor;

  //! The slice editor.
  SliceEditor *sliceEditor;

  //! The isosurface editor.
  IsosurfaceEditor *isosurfaceEditor;

  //! The probe widget.
  ProbeWidget *probeWidget;

  //! Auto-rotate button.
  QAction *autoRotateAction;

  //! Auto-rotation rate
  float autoRotationRate;

  //! Timer for use when stepping through multiple models.
  QTimer playTimeStepsTimer;

  //! Label for current OSPRay object file information.
  QLabel currentFilenameInfoLabel;

  //! Print an error message.
  void emitMessage(const std::string &kind, const std::string &message) const
  { std::cerr << "  " + toString() + "  " + kind + ": " + message + "." << std::endl; }

  //! Error checking.
  void exitOnCondition(bool condition, const std::string &message) const
  { if (!condition) return;  emitMessage("ERROR", message);  exit(1); }

  //! Load an OSPRay model from a file.
  void importObjectsFromFile(const std::string &filename);

  //! Create and configure the OSPRay state.
  void initObjects();

  //! Create and configure the user interface widgets and callbacks.
  void initUserInterfaceWidgets();

};
