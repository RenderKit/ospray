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
#include "modules/loaders/TriangleMeshFile.h"
#include "VolumeViewer.h"
#include "TransferFunctionEditor.h"
#include "IsosurfaceEditor.h"
#include "LightEditor.h"
#include "SliceEditor.h"
#include "PreferencesDialog.h"

VolumeViewer::VolumeViewer(const std::vector<std::string> &objectFileFilenames,
                           bool showFrameRate,
                           bool fullScreen,
                           std::string writeFramesFilename)
  : objectFileFilenames(objectFileFilenames),
    modelIndex(0),
    boundingBox(osp::vec3f(0.f), osp::vec3f(1.f)),
    renderer(NULL),
    rendererInitialized(false),
    transferFunction(NULL),
    light(NULL),
    osprayWindow(NULL),
    transferFunctionEditor(NULL),
    isosurfaceEditor(NULL),
    autoRotateAction(NULL),
    autoRotationRate(0.025f)
{
  //! Default window size.
  resize(1024, 768);

  //! Create and configure the OSPRay state.
  initObjects();

  //! Configure the user interface widgets and callbacks.
  initUserInterfaceWidgets();

  //! Create an OSPRay window and set it as the central widget, but don't let it start rendering until we're done with setup.
  osprayWindow = new QOSPRayWindow(this, renderer, showFrameRate, writeFramesFilename);
  setCentralWidget(osprayWindow);

  //! Set the window bounds based on the OSPRay world bounds.
  osprayWindow->setWorldBounds(boundingBox);

  if (fullScreen)
    setWindowState(windowState() | Qt::WindowFullScreen);

  //! Show the window.
  show();
}

void VolumeViewer::setModel(size_t index)
{
  modelIndex = index;

  //! Set current model on the OSPRay renderer.
  ospSetObject(renderer, "model", models[index]);
  ospCommit(renderer);
  rendererInitialized = true;

  //! Update transfer function and isosurface editor data value range with the voxel range of the current model's first volume.
  osp::vec2f voxelRange(0.f);  ospGetVec2f(volumes[index][0], "voxelRange", &voxelRange);

  if(voxelRange != osp::vec2f(0.f)) {
    transferFunctionEditor->setDataValueRange(voxelRange);
    isosurfaceEditor->setDataValueRange(voxelRange);
  }

  //! Update current filename information label.
  currentFilenameInfoLabel.setText("<b>Timestep " + QString::number(index) + QString("</b>: ") + QString(objectFileFilenames[index].c_str()).split('/').back() + ". Data value range: [" + QString::number(voxelRange.x) + ", " + QString::number(voxelRange.y) + "]");

  //! Enable rendering on the OSPRay window.
  osprayWindow->setRenderingEnabled(true);
}

void VolumeViewer::autoRotate(bool set)
{
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

void VolumeViewer::addSlice(std::string filename)
{
  sliceEditor->addSlice(filename);
}

void VolumeViewer::addGeometry(std::string filename)
{
  //! For now we assume PLY geometry files. Later we can support other geometry formats.

  //! Get filename if not specified.
  if(filename.empty())
    filename = QFileDialog::getOpenFileName(this, tr("Load geometry"), ".", "Geometry files (*.ply *.dds)").toStdString();

  if(filename.empty())
    return;

  //! Attempt to load the geometry through the TriangleMeshFile loader.
  OSPTriangleMesh triangleMesh = ospNewTriangleMesh();

  //! If successful, commit the triangle mesh and add it to all models.
  if(TriangleMeshFile::importTriangleMesh(filename, triangleMesh) != NULL) {

    ospCommit(triangleMesh);

    for(unsigned int i=0; i<models.size(); i++) {
      ospAddGeometry(models[i], triangleMesh);
      ospCommit(models[i]);
    }

    //! Force render.
    render();
  }
}

void VolumeViewer::screenshot(std::string filename)
{
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

void VolumeViewer::setGradientShadingEnabled(bool value)
{
  for(size_t i=0; i<volumes.size(); i++)
    for(size_t j=0; j<volumes[i].size(); j++) {
      ospSet1i(volumes[i][j], "gradientShadingEnabled", value);
      ospCommit(volumes[i][j]);
    }

  render();
}

void VolumeViewer::setSamplingRate(double value)
{
  for(size_t i=0; i<volumes.size(); i++)
    for(size_t j=0; j<volumes[i].size(); j++) {
      ospSet1f(volumes[i][j], "samplingRate", value);
      ospCommit(volumes[i][j]);
    }

  render();
}

void VolumeViewer::setVolumeClippingBox(osp::box3f value)
{
  for(size_t i=0; i<volumes.size(); i++)
    for(size_t j=0; j<volumes[i].size(); j++) {
      ospSet3fv(volumes[i][j], "volumeClippingBoxLower", &value.lower.x);
      ospSet3fv(volumes[i][j], "volumeClippingBoxUpper", &value.upper.x);
      ospCommit(volumes[i][j]);
    }

  render();
}

void VolumeViewer::setSlices(std::vector<SliceParameters> sliceParameters)
{
  //! Provide the slices to OSPRay as the coefficients (a,b,c,d) of the plane equation ax + by + cz + d = 0.
  std::vector<osp::vec4f> planes;

  for(size_t i=0; i<sliceParameters.size(); i++)
    planes.push_back(osp::vec4f(sliceParameters[i].normal.x,
                                sliceParameters[i].normal.y,
                                sliceParameters[i].normal.z,
                                -dot(sliceParameters[i].origin, sliceParameters[i].normal)));

  OSPData planesData = ospNewData(planes.size(), OSP_FLOAT4, &planes[0].x);

  //! Remove existing slice geometries from models.
  for(size_t i=0; i<slices.size(); i++)
    for(size_t j=0; j<slices[i].size(); j++) {
      ospRemoveGeometry(models[i], slices[i][j]);
    }

  slices.clear();

  //! Add new slices for each volume of each model. Later we can do this only for the active model on time step change...
  for(size_t i=0; i<volumes.size(); i++) {
    slices.push_back(std::vector<OSPGeometry>());

    for(size_t j=0; j<volumes[i].size(); j++)
      for(size_t k=0; k<sliceParameters.size(); k++) {

        OSPGeometry slicesGeometry = ospNewGeometry("slices");
        ospSetData(slicesGeometry, "planes", planesData);
        ospSetObject(slicesGeometry, "volume", volumes[i][j]);
        ospCommit(slicesGeometry);

        ospAddGeometry(models[i], slicesGeometry);

        slices[i].push_back(slicesGeometry);
      }

    ospCommit(models[i]);
  }

  render();
}

void VolumeViewer::setIsovalues(std::vector<float> isovalues)
{
  OSPData isovaluesData = ospNewData(isovalues.size(), OSP_FLOAT, &isovalues[0]);

  for(size_t i=0; i<volumes.size(); i++)
    for(size_t j=0; j<volumes[i].size(); j++) {
      ospSetData(volumes[i][j], "isovalues", isovaluesData);
      ospCommit(volumes[i][j]);
    }

  render();
}

void VolumeViewer::importObjectsFromFile(const std::string &filename)
{
  //! Create an OSPRay model.
  OSPModel model = ospNewModel();
  models.push_back(model);

  //! Create vector of volumes for this model.
  volumes.push_back(std::vector<OSPVolume>());

  //! Load OSPRay objects from a file.
  OSPObject *objects = ObjectFile::importObjects(filename.c_str());

  //! Iterate over the volumes contained in the object list.
  for (size_t i=0 ; objects[i] ; i++) {
    OSPDataType type;
    ospGetType(objects[i], NULL, &type);

    if (type == OSP_GEOMETRY) {

      //! Commit the geometry.
      ospCommit(objects[i]);

      //! Add the loaded geometry to the model.
      ospAddGeometry(model, (OSPGeometry) objects[i]);

    } else if (type == OSP_VOLUME) {

      //! For now we set the same transfer function on all volumes.
      ospSetObject(objects[i], "transferFunction", transferFunction);
      ospCommit(objects[i]);

      //! Add the loaded volume(s) to the model.
      ospAddVolume(model, (OSPVolume) objects[i]);

      //! Add to volumes vector for the current model.
      volumes.back().push_back((OSPVolume) objects[i]);
    }
  }

  //! Commit the model.
  ospCommit(model);
}

void VolumeViewer::initObjects()
{
  //! Create an OSPRay renderer.
  renderer = ospNewRenderer("raycast_volume_renderer");
  exitOnCondition(renderer == NULL, "could not create OSPRay renderer object");

  //! Create an OSPRay light source.
  light = ospNewLight(NULL, "DirectionalLight");
  exitOnCondition(light == NULL, "could not create OSPRay light object");
  ospSet3f(light, "direction", 1.0f, -2.0f, -1.0f);
  ospSet3f(light, "color", 1.0f, 1.0f, 1.0f);
  ospCommit(light);

  //! Set the light source on the renderer.
  ospSetData(renderer, "lights", ospNewData(1, OSP_OBJECT, &light));

  //! Create an OSPRay transfer function.
  transferFunction = ospNewTransferFunction("piecewise_linear");
  exitOnCondition(transferFunction == NULL, "could not create OSPRay transfer function object");
  ospCommit(transferFunction);

  //! Load OSPRay objects from files.
  for (size_t i=0 ; i < objectFileFilenames.size() ; i++)
    importObjectsFromFile(objectFileFilenames[i]);

  //! Get the bounding box of the first volume.
  if(volumes.size() > 0 && volumes[0].size() > 0) {
    ospGetVec3f(volumes[0][0], "boundingBoxMin", &boundingBox.lower);
    ospGetVec3f(volumes[0][0], "boundingBoxMax", &boundingBox.upper);
  }
}

void VolumeViewer::initUserInterfaceWidgets()
{
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

  //! Add the "add geometry" widget and callback.
  QAction *addGeometryAction = new QAction("Add geometry", this);
  connect(addGeometryAction, SIGNAL(triggered()), this, SLOT(addGeometry()));
  toolbar->addAction(addGeometryAction);

  //! Add the "screenshot" widget and callback.
  QAction *screenshotAction = new QAction("Screenshot", this);
  connect(screenshotAction, SIGNAL(triggered()), this, SLOT(screenshot()));
  toolbar->addAction(screenshotAction);

  //! Create the transfer function editor dock widget, this widget modifies the transfer function directly.
  QDockWidget *transferFunctionEditorDockWidget = new QDockWidget("Transfer Function", this);
  transferFunctionEditor = new TransferFunctionEditor(transferFunction);
  transferFunctionEditorDockWidget->setWidget(transferFunctionEditor);
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(commitVolumes()));
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(render()));
  addDockWidget(Qt::LeftDockWidgetArea, transferFunctionEditorDockWidget);

  //! Set the transfer function editor widget to its minimum allowed height, to leave room for other dock widgets.
  transferFunctionEditor->setMaximumHeight(transferFunctionEditor->minimumSize().height());

  //! Create slice editor dock widget.
  QDockWidget *sliceEditorDockWidget = new QDockWidget("Slices", this);
  sliceEditor = new SliceEditor(boundingBox);
  sliceEditorDockWidget->setWidget(sliceEditor);
  connect(sliceEditor, SIGNAL(slicesChanged(std::vector<SliceParameters>)), this, SLOT(setSlices(std::vector<SliceParameters>)));
  addDockWidget(Qt::LeftDockWidgetArea, sliceEditorDockWidget);

  //! Create isosurface editor dock widget.
  QDockWidget *isosurfaceEditorDockWidget = new QDockWidget("Isosurfaces", this);
  isosurfaceEditor = new IsosurfaceEditor();
  isosurfaceEditorDockWidget->setWidget(isosurfaceEditor);
  connect(isosurfaceEditor, SIGNAL(isovaluesChanged(std::vector<float>)), this, SLOT(setIsovalues(std::vector<float>)));
  addDockWidget(Qt::LeftDockWidgetArea, isosurfaceEditorDockWidget);

  //! Create the light editor dock widget, this widget modifies the light directly.
  //! Disable for now pending UI improvements...
  /* QDockWidget *lightEditorDockWidget = new QDockWidget("Light Editor", this);
     LightEditor *lightEditor = new LightEditor(light);
     lightEditorDockWidget->setWidget(lightEditor);
     connect(lightEditor, SIGNAL(lightChanged()), this, SLOT(render()));
     addDockWidget(Qt::LeftDockWidgetArea, lightEditorDockWidget); */

  //! Tabify dock widgets.
  tabifyDockWidget(transferFunctionEditorDockWidget, sliceEditorDockWidget);
  tabifyDockWidget(transferFunctionEditorDockWidget, isosurfaceEditorDockWidget);

  //! Tabs on top.
  setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

  //! Default to showing transfer function tab widget.
  transferFunctionEditorDockWidget->raise();

  //! Add the current OSPRay object file label to the bottom status bar.
  statusBar()->addWidget(&currentFilenameInfoLabel);
}
