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

#include <algorithm>
#include "modules/loaders/ObjectFile.h"
#include "VolumeViewer.h"
#include "TransferFunctionEditor.h"
#include "IsosurfaceEditor.h"
#include "LightEditor.h"
#include "SliceWidget.h"
#include "PLYGeometryFile.h"
#include "PreferencesDialog.h"

VolumeViewer::VolumeViewer(const std::vector<std::string> &objectFileFilenames,
                           bool showFrameRate,
                           std::string writeFramesFilename)
  : objectFileFilenames(objectFileFilenames),
    renderer(NULL),
    rendererInitialized(false),
    transferFunction(NULL), 
    osprayWindow(NULL), 
    autoRotationRate(0.025f) 
{
  //! Default window size.
  resize(1024, 768);

  //! Create an OSPRay renderer.
  renderer = ospNewRenderer("raycast_volume_renderer");
  exitOnCondition(renderer == NULL, "could not create OSPRay renderer object");

  //! Create an OSPRay window and set it as the central widget, but don't let it start rendering until we're done with setup.
  osprayWindow = new QOSPRayWindow(this, renderer, showFrameRate, writeFramesFilename);
  setCentralWidget(osprayWindow);

  //! Set the window bounds based on the OSPRay world bounds (always [(0,0,0), (1,1,1)) for volumes).
  osprayWindow->setWorldBounds(osp::box3f(osp::vec3f(0.0f), osp::vec3f(1.0f)));

  //! Create an OSPRay light source.
  light = ospNewLight(NULL, "DirectionalLight");
  ospSet3f(light, "direction", 1.0f, -2.0f, -1.0f);
  ospSet3f(light, "color", 1.0f, 1.0f, 1.0f);
  ospCommit(light);

  //! Set the light source on the renderer.
  ospSetData(renderer, "lights", ospNewData(1, OSP_OBJECT, &light));

  //! Create an OSPRay transfer function.
  transferFunction = ospNewTransferFunction("piecewise_linear");
  exitOnCondition(transferFunction == NULL, "could not create OSPRay transfer function object");

  //! Create and configure the OSPRay state.
  initObjects();

  //! Configure the user interface widgets and callbacks.
  initUserInterfaceWidgets();

  //! Set the window bounds based on the OSPRay world bounds.
  osprayWindow->setWorldBounds(boundingBox);

  //! Show the window.
  show();
}

void VolumeViewer::setModel(size_t index) {

  //! Set current model on the OSPRay renderer.
  ospSetObject(renderer, "model", models[index]);
  ospCommit(renderer);
  rendererInitialized = true;

  //! Update transfer function and isosurface editor data value range with the voxel range of the current volume.
  osp::vec2f voxelRange(0.f);  ospGetVec2f(volumes[index], "voxelRange", &voxelRange);

  if(voxelRange != osp::vec2f(0.f)) {
    transferFunctionEditor->setDataValueRange(voxelRange);
    isosurfaceEditor->setDataValueRange(voxelRange);
  }

  //! Update current filename information label.
  currentFilenameInfoLabel.setText("<b>Timestep " + QString::number(index) + QString("</b>: ") + QString(objectFileFilenames[index].c_str()).split('/').back() + ". Data value range: [" + QString::number(voxelRange.x) + ", " + QString::number(voxelRange.y) + "]");

  //! Enable rendering on the OSPRay window.
  osprayWindow->setRenderingEnabled(true);
}

void VolumeViewer::autoRotate(bool set) {

  if(osprayWindow == NULL)
    return;

  if(autoRotateAction != NULL)
    autoRotateAction->setChecked(set);

  if(set) {
    osprayWindow->setRotationRate(autoRotationRate);
    osprayWindow->updateGL();
  }
  else
    osprayWindow->setRotationRate(0.);
}

void VolumeViewer::addSlice(std::string filename) {

  //! Use dynamic geometry model for slices.
  std::vector<OSPModel> dynamicModels;
  dynamicModels.push_back(dynamicModel);

  //! Create a slice widget and add it to the dock. This widget modifies the slice directly.
  SliceWidget * sliceWidget = new SliceWidget(dynamicModels, boundingBox);
  connect(sliceWidget, SIGNAL(sliceChanged()), this, SLOT(render()));
  sliceWidgetsLayout.addWidget(sliceWidget);

  //! Load state from file if specified.
  if(!filename.empty())
    sliceWidget->load(filename);

  //! Apply the slice (if auto apply enabled), triggering a commit and render.
  sliceWidget->autoApply();
}

void VolumeViewer::addGeometry(std::string filename) {

  //! For now we assume PLY geometry files. Later we can support other geometry formats.

  //! Get filename if not specified.
  if(filename.empty())
    filename = QFileDialog::getOpenFileName(this, tr("Load geometry"), ".", "PLY files (*.ply)").toStdString();

  if(filename.empty())
    return;

  //! Load the geometry.
  PLYGeometryFile geometryFile(filename);

  //! Add the OSPRay triangle mesh to all models.
  OSPTriangleMesh triangleMesh = geometryFile.getOSPTriangleMesh();

  for(unsigned int i=0; i<models.size(); i++) {
    ospAddGeometry(models[i], triangleMesh);
    ospCommit(models[i]);
  }

  //! Force render.
  render();
}

void VolumeViewer::screenshot(std::string filename) {

  //! Get filename if not specified.
  if(filename.empty())
    filename = QFileDialog::getSaveFileName(this, tr("Save screenshot"), ".", "PNG files (*.png)").toStdString();

  if(filename.empty())
    return;

  //! Make sure the filename has the proper extension.
  if(QString(filename.c_str()).endsWith(".png") != true)
    filename += ".png";

  //! Grab the image.
  QImage image = osprayWindow->grabFrameBuffer();

  //! Save the screenshot.
  bool success = image.save(filename.c_str());

  std::cout << (success ? "saved screenshot to " : "failed saving screenshot ") << filename << std::endl;
}

void VolumeViewer::importObjectsFromFile(const std::string &filename) {

  //! Create an OSPRay model.
  OSPModel model = ospNewModel();

  //! Load OSPRay objects from a file.
  OSPObject *objects = ObjectFile::importObjects(filename.c_str());

  //! Iterate over the volumes contained in the object list.
  for (size_t i=0 ; objects[i] ; i++) {
    OSPDataType type;
    ospGetType(objects[i], NULL, &type);

    if (type == OSP_VOLUME) {

      //! For now we set the same transfer function on all volumes.
      ospSetObject(objects[i], "transferFunction", transferFunction);
      ospCommit(objects[i]);

      //! Add the loaded volume(s) to the model.
      ospAddVolume(model, (OSPVolume) objects[i]);

      //! Keep a vector of all loaded volume(s).
      volumes.push_back((OSPVolume) objects[i]);
    }
  }

  //! Commit the model.
  ospCommit(model);
  models.push_back(model);
}

void VolumeViewer::initObjects() {

  //! Create model for dynamic geometry.
  dynamicModel = ospNewModel();
  ospCommit(dynamicModel);

  //! Set the dynamic model on the renderer.
  ospSetObject(renderer, "dynamic_model", dynamicModel);

  //! Load OSPRay objects from files.
  for (size_t i=0 ; i < objectFileFilenames.size() ; i++)
    importObjectsFromFile(objectFileFilenames[i]);

  //! Get the bounding box of the first volume.
  if(volumes.size() > 0) {
    ospGetVec3f(volumes[0], "boundingBoxMin", &boundingBox.lower);
    ospGetVec3f(volumes[0], "boundingBoxMax", &boundingBox.upper);
  }
}

void VolumeViewer::initUserInterfaceWidgets() {

  //! Create a toolbar at the top of the window.
  QToolBar *toolbar = addToolBar("toolbar");

  //! Add preferences widget and callback.
  PreferencesDialog *preferencesDialog = new PreferencesDialog(this, boundingBox);
  QAction *showPreferencesAction = new QAction("Preferences", this);
  connect(showPreferencesAction, SIGNAL(triggered()), preferencesDialog, SLOT(show()));
  toolbar->addAction(showPreferencesAction);

  //! Add the "auto rotate" widget and callback.
  autoRotateAction = new QAction("Auto rotate", this);
  autoRotateAction->setCheckable(true);
  connect(autoRotateAction, SIGNAL(toggled(bool)), this, SLOT(autoRotate(bool)));
  toolbar->addAction(autoRotateAction);

  //! Add the "next timestep" widget and callback.
  QAction *nextTimeStepAction = new QAction("Next timestep", this);
  connect(nextTimeStepAction, SIGNAL(triggered()), this, SLOT(nextTimeStep()));
  toolbar->addAction(nextTimeStepAction);

  //! Add the "play timesteps" widget and callback.
  QAction *playTimeStepsAction = new QAction("Play timesteps", this);
  playTimeStepsAction->setCheckable(true);
  connect(playTimeStepsAction, SIGNAL(toggled(bool)), this, SLOT(playTimeSteps(bool)));
  toolbar->addAction(playTimeStepsAction);

  //! Connect the "play timesteps" timer.
  connect(&playTimeStepsTimer, SIGNAL(timeout()), this, SLOT(nextTimeStep()));

  //! Add the "add slice" widget and callback.
  QAction *addSliceAction = new QAction("Add slice", this);
  connect(addSliceAction, SIGNAL(triggered()), this, SLOT(addSlice()));
  toolbar->addAction(addSliceAction);

  //! Add the "add geometry" widget and callback.
  QAction *addGeometryAction = new QAction("Add geometry", this);
  connect(addGeometryAction, SIGNAL(triggered()), this, SLOT(addGeometry()));
  toolbar->addAction(addGeometryAction);

  //! Add the "screenshot" widget and callback.
  QAction *screenshotAction = new QAction("Screenshot", this);
  connect(screenshotAction, SIGNAL(triggered()), this, SLOT(screenshot()));
  toolbar->addAction(screenshotAction);

  //! Create the transfer function editor dock widget, this widget modifies the transfer function directly.
  QDockWidget *transferFunctionEditorDockWidget = new QDockWidget("Transfer Function Editor", this);
  transferFunctionEditor = new TransferFunctionEditor(transferFunction);
  transferFunctionEditorDockWidget->setWidget(transferFunctionEditor);
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(commitVolumes()));
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(render()));
  addDockWidget(Qt::LeftDockWidgetArea, transferFunctionEditorDockWidget);

  //! Set the transfer function editor widget to its minimum allowed height, to leave room for other dock widgets.
  transferFunctionEditor->setMaximumHeight(transferFunctionEditor->minimumSize().height());

  //! Create isosurface editor dock widget.
  QDockWidget *isosurfaceEditorDockWidget = new QDockWidget("Isosurface Editor", this);
  isosurfaceEditor = new IsosurfaceEditor();
  isosurfaceEditorDockWidget->setWidget(isosurfaceEditor);
  connect(isosurfaceEditor, SIGNAL(isovaluesChanged(std::vector<float>)), this, SLOT(setIsovalues(std::vector<float>)));
  addDockWidget(Qt::LeftDockWidgetArea, isosurfaceEditorDockWidget);

  //! Set the isosurface editor widget to its minimum allowed height, to leave room for other dock widgets.
  isosurfaceEditor->setMaximumHeight(isosurfaceEditor->minimumSize().height());

  //! Create the light editor dock widget, this widget modifies the light directly.
  //! Disable for now pending UI improvements...
  /* QDockWidget *lightEditorDockWidget = new QDockWidget("Light Editor", this);
     LightEditor *lightEditor = new LightEditor(light);
     lightEditorDockWidget->setWidget(lightEditor);
     connect(lightEditor, SIGNAL(lightChanged()), this, SLOT(render()));
     addDockWidget(Qt::LeftDockWidgetArea, lightEditorDockWidget); */

  //! Create a scrollable dock widget for any added slices.
  QDockWidget *slicesDockWidget = new QDockWidget("Slices", this);
  QScrollArea *slicesScrollArea = new QScrollArea();
  QWidget *slicesWidget = new QWidget();
  slicesWidget->setLayout(&sliceWidgetsLayout);
  slicesScrollArea->setWidget(slicesWidget);
  slicesScrollArea->setWidgetResizable(true);
  slicesDockWidget->setWidget(slicesScrollArea);
  addDockWidget(Qt::LeftDockWidgetArea, slicesDockWidget);

  //! Add the current OSPRay object file label to the bottom status bar.
  statusBar()->addWidget(&currentFilenameInfoLabel);
}
