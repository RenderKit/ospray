// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
// own
#include "VolumeViewer.h"
// #include "loaders/ObjectFile.h"
// #include "loaders/TriangleMeshFile.h"
#include "TransferFunctionEditor.h"
#include "IsosurfaceEditor.h"
#include "LightEditor.h"
#include "SliceEditor.h"
#include "PreferencesDialog.h"
#include "ProbeWidget.h"
#include "OpenGLAnnotationRenderer.h"

// #ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
// #include "../../modules/loaders/VolumeFile.h"
// #endif

#include "importer/Importer.h"

VolumeViewer::VolumeViewer(const std::vector<std::string> &objectFileFilenames,
                           std::string renderer_type,
                           bool ownModelPerObject,
                           bool showFrameRate,
                           bool fullScreen,
                           std::string writeFramesFilename)
  : objectFileFilenames(objectFileFilenames),
    modelIndex(0),
    ownModelPerObject(ownModelPerObject),
    boundingBox(ospcommon::vec3f(0.f), ospcommon::vec3f(1.f)),
    renderer(NULL),
    rendererInitialized(false),
    transferFunction(NULL),
    ambientLight(NULL),
    directionalLight(NULL),
    osprayWindow(NULL),
    annotationRenderer(NULL),
    transferFunctionEditor(NULL),
    isosurfaceEditor(NULL),
    autoRotateAction(NULL),
    autoRotationRate(0.025f)
{
  // Default window size.
  resize(1024, 768);

  // Create and configure the OSPRay state.
  initObjects(renderer_type);

  // Create an OSPRay window and set it as the central widget, but don't let it start rendering until we're done with setup.
  osprayWindow = new QOSPRayWindow(this, this->renderer,
                                   showFrameRate, writeFramesFilename);
  setCentralWidget(osprayWindow);

  // Set the window bounds based on the OSPRay world bounds.
  osprayWindow->setWorldBounds(boundingBox);

  // Configure the user interface widgets and callbacks.
  initUserInterfaceWidgets();

  if (fullScreen)
    setWindowState(windowState() | Qt::WindowFullScreen);

  // Show the window.
  show();
}

ospcommon::box3f VolumeViewer::getBoundingBox()
{
  return boundingBox;
}

QOSPRayWindow *VolumeViewer::getWindow()
{
  return osprayWindow;
}

TransferFunctionEditor *VolumeViewer::getTransferFunctionEditor()
{
  return transferFunctionEditor;
}

void VolumeViewer::setModel(size_t index)
{
  modelIndex = index;

  // Set current model on the OSPRay renderer.
  ospSetObject(renderer, "model", modelStates[index].model);
  ospCommit(renderer);
  rendererInitialized = true;

  PRINT(modelStates[index].volumes.size());
  if (!modelStates[index].volumes.empty()) {
    // Update transfer function and isosurface editor data value range with the voxel range of the current model's first volume.
    OSPVolume volume = modelStates[index].volumes[0]->handle;
    ospcommon::vec2f voxelRange = modelStates[index].volumes[0]->voxelRange;
    // #ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
    //   voxelRange = VolumeFile::voxelRangeOf[volume];
    // #else
    //   ospGetVec2f(modelStates[index].volumes[0], "voxelRange", (osp::vec2f*)&voxelRange);
    // #endif
    
    if(voxelRange != ospcommon::vec2f(0.f)) {
      transferFunctionEditor->setDataValueRange(voxelRange);
      isosurfaceEditor->setDataValueRange(voxelRange);
    }
    
    // Update active volume on probe widget.
    probeWidget->setVolume(modelStates[index].volumes[0]->handle);
    
    // Update current filename information label.
    if (ownModelPerObject)
      currentFilenameInfoLabel.setText("<b>Timestep " + QString::number(index) + QString("</b>: Data value range: [") + QString::number(voxelRange.x) + ", " + QString::number(voxelRange.y) + "]");
    else
      currentFilenameInfoLabel.setText("<b>Timestep " + QString::number(index) + QString("</b>: ") + QString(objectFileFilenames[index].c_str()).split('/').back() + ". Data value range: [" + QString::number(voxelRange.x) + ", " + QString::number(voxelRange.y) + "]");
  }
  // Enable rendering on the OSPRay window.
  osprayWindow->setRenderingEnabled(true);
}

std::string VolumeViewer::toString() const
{
  return("VolumeViewer");
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

void VolumeViewer::setAutoRotationRate(float rate)
{
  autoRotationRate = rate;
}

void VolumeViewer::nextTimeStep()
{
  modelIndex = (modelIndex + 1) % modelStates.size();
  setModel(modelIndex);
  render();
}

void VolumeViewer::playTimeSteps(bool animate)
{
  if (animate == true)
    playTimeStepsTimer.start(500);
  else
    playTimeStepsTimer.stop();
}

void VolumeViewer::addSlice(std::string filename)
{
  sliceEditor->addSlice(filename);
}

void VolumeViewer::addGeometry(std::string filename)
{
  // For now we assume PLY geometry files. Later we can support other geometry formats.

  // Get filename if not specified.
  if(filename.empty())
    filename = QFileDialog::getOpenFileName(this, tr("Load geometry"), ".", "Geometry files (*.ply *.dds)").toStdString();

  if(filename.empty())
    return;

  // Attempt to load the geometry through the TriangleMeshFile loader.
  // OSPGeometry triangleMesh = ospNewGeometry("trianglemesh");

  // If successful, commit the triangle mesh and add it to all models.
  // if(TriangleMeshFile::importTriangleMesh(filename, triangleMesh) != NULL) {
  ospray::vv_importer::Group *newStuff = ospray::vv_importer::import(filename);
  if (!newStuff) return;
  if (newStuff->geometry.size() != 1) return;
  
  OSPGeometry triangleMesh = newStuff->geometry[0]->handle;
  // For now: if this is a DDS geometry, assume it is a horizon and its color should be mapped through the first volume's transfer function.
  if(QString(filename.c_str()).endsWith(".dds") && modelStates.size() > 0 && modelStates[0].volumes.size() > 0) {
    
    OSPMaterial material = ospNewMaterial(renderer, "default");
    ospSet3f(material, "Kd", 1,1,1);
    ospSetObject(material, "volume", modelStates[0].volumes[0]->handle);
    ospCommit(material);
    
    ospSetMaterial(triangleMesh, material);
  }
  
  ospCommit(triangleMesh);
  
  // Create an instance of the geometry and add the instance to the main model(s)--this prevents the geometry
  // from being rebuilt every time the main model is committed (e.g. when slices / isosurfaces are manipulated)
  OSPModel modelInstance = ospNewModel();
  ospAddGeometry(modelInstance, triangleMesh);
  ospCommit(modelInstance);
  
  ospcommon::affine3f xfm = ospcommon::one;
  OSPGeometry triangleMeshInstance = ospNewInstance(modelInstance, (osp::affine3f&)xfm);
  ospCommit(triangleMeshInstance);
  
  for(size_t i=0; i<modelStates.size(); i++) {
    ospAddGeometry(modelStates[i].model, triangleMeshInstance);
    ospCommit(modelStates[i].model);
  }
  
  // Force render.
  render();
}

void VolumeViewer::screenshot(std::string filename)
{
  // Print current camera view parameters (can be used on command line to recreate view)
  std::cout << "screenshot view parameters (use on command line to reproduce view): " << std::endl
            << "  " << *(osprayWindow->getViewport()) << std::endl;

  // Get filename if not specified.
  if(filename.empty())
    filename = QFileDialog::getSaveFileName(this, tr("Save screenshot"), ".", "PNG files (*.png)").toStdString();

  if(filename.empty())
    return;

  // Make sure the filename has the proper extension.
  if(QString(filename.c_str()).endsWith(".png") != true)
    filename += ".png";

  // Grab the image.
  QImage image = osprayWindow->grabFrameBuffer();

  // Save the screenshot.
  bool success = image.save(filename.c_str());

  std::cout << (success ? "saved screenshot to " : "failed saving screenshot ") << filename << std::endl;
}

void VolumeViewer::keyPressEvent(QKeyEvent * event)
{
  if (event->key() == Qt::Key_Escape){
    close();
  }
}

void VolumeViewer::commitVolumes()
{
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++)
      ospCommit(modelStates[i].volumes[j]->handle);
}

void VolumeViewer::render()
{
  if (osprayWindow != NULL) {
    osprayWindow->resetAccumulationBuffer();
    osprayWindow->updateGL();
  }
}

void VolumeViewer::setRenderAnnotationsEnabled(bool value)
{
  if (value) {
    if (!annotationRenderer)
      annotationRenderer = new OpenGLAnnotationRenderer(this);

    connect(osprayWindow, SIGNAL(renderGLComponents()), annotationRenderer, SLOT(render()), Qt::UniqueConnection);
  }
  else {
    delete annotationRenderer;
    annotationRenderer = NULL;
  }

  render();
}

void VolumeViewer::setSubsamplingInteractionEnabled(bool value)
{
  ospSet1i(renderer, "spp", value ? -1 : 1);
  if(rendererInitialized)
    ospCommit(renderer);
}

void VolumeViewer::setGradientShadingEnabled(bool value)
{
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet1i(modelStates[i].volumes[j]->handle, "gradientShadingEnabled", value);
      ospCommit(modelStates[i].volumes[j]->handle);
    }

  render();
}

void VolumeViewer::setSamplingRate(double value)
{
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet1f(modelStates[i].volumes[j]->handle, "samplingRate", value);
      ospCommit(modelStates[i].volumes[j]->handle);
    }

  render();
}

void VolumeViewer::setVolumeClippingBox(ospcommon::box3f value)
{
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet3fv(modelStates[i].volumes[j]->handle, "volumeClippingBoxLower", &value.lower.x);
      ospSet3fv(modelStates[i].volumes[j]->handle, "volumeClippingBoxUpper", &value.upper.x);
      ospCommit(modelStates[i].volumes[j]->handle);
    }

  render();
}

void VolumeViewer::setSlices(std::vector<SliceParameters> sliceParameters)
{
  // Provide the slices to OSPRay as the coefficients (a,b,c,d) of the plane equation ax + by + cz + d = 0.
  std::vector<ospcommon::vec4f> planes;

  for(size_t i=0; i<sliceParameters.size(); i++)
    planes.push_back(ospcommon::vec4f(sliceParameters[i].normal.x,
                                sliceParameters[i].normal.y,
                                sliceParameters[i].normal.z,
                                -dot(sliceParameters[i].origin, sliceParameters[i].normal)));

  OSPData planesData = ospNewData(planes.size(), OSP_FLOAT4, &planes[0].x);

  // Remove existing slice geometries from models.
  for(size_t i=0; i<modelStates.size(); i++) {
    for(size_t j=0; j<modelStates[i].slices.size(); j++)
      ospRemoveGeometry(modelStates[i].model, modelStates[i].slices[j]->handle);

    modelStates[i].slices.clear();
  }

  // Add new slices for each volume of each model. Later we can do this only for the active model on time step change...
  for(size_t i=0; i<modelStates.size(); i++) {

    if(planes.size() > 0) {
      for(size_t j=0; j<modelStates[i].volumes.size(); j++) {

        OSPGeometry slicesGeometry = ospNewGeometry("slices");
        ospSetData(slicesGeometry, "planes", planesData);
        ospSetObject(slicesGeometry, "volume", modelStates[i].volumes[j]->handle);
        ospCommit(slicesGeometry);

        ospAddGeometry(modelStates[i].model, slicesGeometry);

        modelStates[i].slices.push_back(new ModelState::Geometry(slicesGeometry));
      }
    }

    ospCommit(modelStates[i].model);
  }

  render();
}

void VolumeViewer::setIsovalues(std::vector<float> isovalues)
{
  OSPData isovaluesData = ospNewData(isovalues.size(), OSP_FLOAT, &isovalues[0]);

  // Remove existing isosurface geometries from models.
  for(size_t i=0; i<modelStates.size(); i++) {
    for(size_t j=0; j<modelStates[i].isosurfaces.size(); j++)
      ospRemoveGeometry(modelStates[i].model, modelStates[i].isosurfaces[j]->handle);

    modelStates[i].isosurfaces.clear();
  }

  // Add new isosurfaces for each volume of each model. Later we can do this only for the active model on time step change...
  for(size_t i=0; i<modelStates.size(); i++) {

    if(isovalues.size() > 0) {
      for(size_t j=0; j<modelStates[i].volumes.size(); j++) {

        OSPGeometry isosurfacesGeometry = ospNewGeometry("isosurfaces");
        ospSetData(isosurfacesGeometry, "isovalues", isovaluesData);
        ospSetObject(isosurfacesGeometry, "volume", modelStates[i].volumes[j]->handle);
        ospCommit(isosurfacesGeometry);

        ospAddGeometry(modelStates[i].model, isosurfacesGeometry);

        modelStates[i].isosurfaces.push_back(new ModelState::Geometry(isosurfacesGeometry));
      }
    }

    ospCommit(modelStates[i].model);
  }

  render();
}

void VolumeViewer::importObjectsFromFile(const std::string &filename)
{
  if (!ownModelPerObject)
    // Create an OSPRay model and its associated model state.
    modelStates.push_back(ModelState(ospNewModel()));
  
  // Load OSPRay objects from a file.
  //  OSPObject *objects = ObjectFile::importObjects(filename.c_str());
  ospray::vv_importer::Group *imported = ospray::vv_importer::import(filename);
  assert(imported);

#if 1
  PING;
  // Iterate over the GEOMETREIS contained in the object list.
  PRINT(imported->geometry.size());

  for (size_t i=0 ; i < imported->geometry.size() ; i++) {
    if (ownModelPerObject)
      modelStates.push_back(ModelState(ospNewModel()));
    
    // Commit the geometry.
    ospCommit(imported->geometry[i]->handle);
    
    // Add the loaded geometry to the model.
    ospAddGeometry(modelStates.back().model, imported->geometry[i]->handle);

    if (ownModelPerObject)
      ospCommit(modelStates.back().model);
  }
  // Iterate over the objects contained in the object list.
  for (size_t i=0 ; i < imported->volume.size() ; i++) {
    if (ownModelPerObject)
      modelStates.push_back(ModelState(ospNewModel()));
    
    ospray::vv_importer::Volume *vol = imported->volume[i];
    assert(vol);
    // For now we set the same transfer function on all volumes.
    ospSetObject(vol->handle, "transferFunction", transferFunction);
    ospCommit(vol->handle);
    
    // Add the loaded volume(s) to the model.
    ospAddVolume(modelStates.back().model, vol->handle);
    
    assert(!vol->bounds.empty());
    // Add to volumes vector for the current model.
    modelStates.back().volumes.push_back(new ModelState::Volume(vol->handle,
                                                                vol->bounds,
                                                                vol->voxelRange
                                                                ));

    if (ownModelPerObject)
      ospCommit(modelStates.back().model);
  }
#else
  // Iterate over the objects contained in the object list.
  for (size_t i=0 ; objects[i] ; i++) {
    if (ownModelPerObject)
      modelStates.push_back(ModelState(ospNewModel()));
    
    OSPDataType type;
    ospGetType(objects[i], NULL, &type);
    
    if (type == OSP_GEOMETRY) {
      
      // Commit the geometry.
      ospCommit(objects[i]);

      // Add the loaded geometry to the model.
      ospAddGeometry(modelStates.back().model, (OSPGeometry) objects[i]);

    } else if (type == OSP_VOLUME) {

      // For now we set the same transfer function on all volumes.
      ospSetObject(objects[i], "transferFunction", transferFunction);
      ospCommit(objects[i]);

      // Add the loaded volume(s) to the model.
      ospAddVolume(modelStates.back().model, (OSPVolume) objects[i]);

      // Add to volumes vector for the current model.
      modelStates.back().volumes.push_back((OSPVolume) objects[i]);
    }

    if (ownModelPerObject)
      ospCommit(modelStates.back().model);
  }
#endif

  if (!ownModelPerObject)
    // Commit the model.
    ospCommit(modelStates.back().model);
}

void VolumeViewer::initObjects(const std::string &renderer_type)
{
  // Create an OSPRay renderer.
  renderer = ospNewRenderer(renderer_type.c_str());
  exitOnCondition(renderer == NULL, "could not create OSPRay renderer object");

  // Set renderer defaults (if not using 'aoX' renderers)
  if (renderer_type[0] != 'a' && renderer_type[1] != 'o')
  {
    ospSet1i(renderer, "aoSamples", 1);
    ospSet1i(renderer, "shadowsEnabled", 1);
  }

  // Create OSPRay ambient and directional lights. GUI elements will modify their parameters.
  ambientLight = ospNewLight(renderer, "AmbientLight");
  exitOnCondition(ambientLight == NULL, "could not create ambient light");
  ospCommit(ambientLight);

  directionalLight = ospNewLight(renderer, "DirectionalLight");
  exitOnCondition(directionalLight == NULL, "could not create directional light");
  ospCommit(directionalLight);

  // Set the light sources on the renderer.
  std::vector<OSPLight> lights;
  lights.push_back(ambientLight);
  lights.push_back(directionalLight);

  ospSetData(renderer, "lights", ospNewData(lights.size(), OSP_OBJECT, &lights[0]));

  // Create an OSPRay transfer function.
  transferFunction = ospNewTransferFunction("piecewise_linear");
  exitOnCondition(transferFunction == NULL, "could not create OSPRay transfer function object");
  ospCommit(transferFunction);

  // Load OSPRay objects from files.
  for (size_t i=0 ; i < objectFileFilenames.size() ; i++)
    importObjectsFromFile(objectFileFilenames[i]);

  boundingBox = ospcommon::empty;
  if (!modelStates.empty()) {
    for (size_t i=0; i<modelStates[0].volumes.size(); i++) 
      boundingBox.extend(modelStates[0].volumes[i]->boundingBox);
  }
  PRINT(boundingBox);
  // // Get the bounding box of all volumes of the first model.
  // if(modelStates.size() > 0 && modelStates[0].volumes.size() > 0) {
  //   ospGetVec3f(modelStates[0].volumes[0], "boundingBoxMin", (osp::vec3f*)&boundingBox.lower);
  //   ospGetVec3f(modelStates[0].volumes[0], "boundingBoxMax", (osp::vec3f*)&boundingBox.upper);

  //   for (size_t i=1; i<modelStates[0].volumes.size(); i++) {
  //     ospcommon::box3f volumeBoundingBox;
  //     ospGetVec3f(modelStates[0].volumes[i], "boundingBoxMin", (osp::vec3f*)&volumeBoundingBox.lower);
  //     ospGetVec3f(modelStates[0].volumes[i], "boundingBoxMax", (osp::vec3f*)&volumeBoundingBox.upper);

  //     boundingBox.extend(volumeBoundingBox);
  //   }
  // }
}

void VolumeViewer::initUserInterfaceWidgets()
{
  // Create a toolbar at the top of the window.
  QToolBar *toolbar = addToolBar("toolbar");

  // Add preferences widget and callback.
  PreferencesDialog *preferencesDialog = new PreferencesDialog(this, boundingBox);
  QAction *showPreferencesAction = new QAction("Preferences", this);
  connect(showPreferencesAction, SIGNAL(triggered()), preferencesDialog, SLOT(show()));
  toolbar->addAction(showPreferencesAction);

  // Add the "auto rotate" widget and callback.
  autoRotateAction = new QAction("Auto rotate", this);
  autoRotateAction->setCheckable(true);
  connect(autoRotateAction, SIGNAL(toggled(bool)), this, SLOT(autoRotate(bool)));
  toolbar->addAction(autoRotateAction);

  // Add the "next timestep" widget and callback.
  QAction *nextTimeStepAction = new QAction("Next timestep", this);
  connect(nextTimeStepAction, SIGNAL(triggered()), this, SLOT(nextTimeStep()));
  toolbar->addAction(nextTimeStepAction);

  // Add the "play timesteps" widget and callback.
  QAction *playTimeStepsAction = new QAction("Play timesteps", this);
  playTimeStepsAction->setCheckable(true);
  connect(playTimeStepsAction, SIGNAL(toggled(bool)), this, SLOT(playTimeSteps(bool)));
  toolbar->addAction(playTimeStepsAction);

  // Connect the "play timesteps" timer.
  connect(&playTimeStepsTimer, SIGNAL(timeout()), this, SLOT(nextTimeStep()));

  // Add the "add geometry" widget and callback.
  QAction *addGeometryAction = new QAction("Add geometry", this);
  connect(addGeometryAction, SIGNAL(triggered()), this, SLOT(addGeometry()));
  toolbar->addAction(addGeometryAction);

  // Add the "screenshot" widget and callback.
  QAction *screenshotAction = new QAction("Screenshot", this);
  connect(screenshotAction, SIGNAL(triggered()), this, SLOT(screenshot()));
  toolbar->addAction(screenshotAction);

  // Create the transfer function editor dock widget, this widget modifies the transfer function directly.
  QDockWidget *transferFunctionEditorDockWidget = new QDockWidget("Transfer Function", this);
  transferFunctionEditor = new TransferFunctionEditor(transferFunction);
  transferFunctionEditorDockWidget->setWidget(transferFunctionEditor);
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(commitVolumes()));
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(render()));
  addDockWidget(Qt::LeftDockWidgetArea, transferFunctionEditorDockWidget);

  // Set the transfer function editor widget to its minimum allowed height, to leave room for other dock widgets.
  transferFunctionEditor->setMaximumHeight(transferFunctionEditor->minimumSize().height());

  // Create slice editor dock widget.
  QDockWidget *sliceEditorDockWidget = new QDockWidget("Slices", this);
  sliceEditor = new SliceEditor(boundingBox);
  sliceEditorDockWidget->setWidget(sliceEditor);
  connect(sliceEditor, SIGNAL(slicesChanged(std::vector<SliceParameters>)), this, SLOT(setSlices(std::vector<SliceParameters>)));
  addDockWidget(Qt::LeftDockWidgetArea, sliceEditorDockWidget);

  // Create isosurface editor dock widget.
  QDockWidget *isosurfaceEditorDockWidget = new QDockWidget("Isosurfaces", this);
  isosurfaceEditor = new IsosurfaceEditor();
  isosurfaceEditorDockWidget->setWidget(isosurfaceEditor);
  connect(isosurfaceEditor, SIGNAL(isovaluesChanged(std::vector<float>)), this, SLOT(setIsovalues(std::vector<float>)));
  addDockWidget(Qt::LeftDockWidgetArea, isosurfaceEditorDockWidget);

  // Create the light editor dock widget, this widget modifies the light directly.
  QDockWidget *lightEditorDockWidget = new QDockWidget("Lights", this);
  LightEditor *lightEditor = new LightEditor(ambientLight, directionalLight);
  lightEditorDockWidget->setWidget(lightEditor);
  connect(lightEditor, SIGNAL(lightsChanged()), this, SLOT(render()));
  addDockWidget(Qt::LeftDockWidgetArea, lightEditorDockWidget);

  // Create the probe dock widget.
  QDockWidget *probeDockWidget = new QDockWidget("Probe", this);
  probeWidget = new ProbeWidget(this);
  probeDockWidget->setWidget(probeWidget);
  addDockWidget(Qt::LeftDockWidgetArea, probeDockWidget);

  // Tabify dock widgets.
  tabifyDockWidget(transferFunctionEditorDockWidget, sliceEditorDockWidget);
  tabifyDockWidget(transferFunctionEditorDockWidget, isosurfaceEditorDockWidget);
  tabifyDockWidget(transferFunctionEditorDockWidget, lightEditorDockWidget);
  tabifyDockWidget(transferFunctionEditorDockWidget, probeDockWidget);

  // Tabs on top.
  setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

  // Default to showing transfer function tab widget.
  transferFunctionEditorDockWidget->raise();

  // Add the current OSPRay object file label to the bottom status bar.
  statusBar()->addWidget(&currentFilenameInfoLabel);
}
