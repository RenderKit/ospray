// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "VolumeViewer.h"
#include "TransferFunctionEditor.h"
#include "IsosurfaceEditor.h"
#include "SliceEditor.h"
#include "PreferencesDialog.h"
#include "ProbeWidget.h"
#include "OpenGLAnnotationRenderer.h"
#include "commandline/SceneParser/trianglemesh/TriangleMeshSceneParser.h"
#include <common/miniSG/miniSG.h>
#include "ospcommon/FileName.h"

#include "importer/Importer.h"

using namespace ospray;
using namespace ospcommon;
using namespace commandline;

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
    renderer(nullptr),
    rendererInitialized(false),
    transferFunction(nullptr),
    ambientLight(nullptr),
    directionalLight(nullptr),
    osprayWindow(nullptr),
    annotationRenderer(nullptr),
    transferFunctionEditor(nullptr),
    isosurfaceEditor(nullptr),
    autoRotateAction(nullptr),
    autoRotationRate(0.025f),
    usePlane(-1),
    samplingRate(-1),
    adaptiveMaxSamplingRate(-1),
    preferencesDialog(nullptr),
    spp(-1),
    shadows(-1),
    preIntegration(-1),
    aoSamples(-1),
    adaptiveSampling(-1),
    gradientShadingEnabled(-1),
    renderInBackground(0)
{
  // Default window size.
  resize(1024, 768);

  // Create and configure the OSPRay state.
  initObjects(renderer_type);

  // Create an OSPRay window and set it as the central widget, but don't let
  // it start rendering until we're done with setup.
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

  setGradientShadingEnabled(true);
  setAOSamples(1);
  setAdaptiveSampling(true);
  setPreIntegration(true);
  setShadows(true);
  setSPP(1);
  setPlane(1);

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
    //! Update transfer function and isosurface editor data value range with the
    //  voxel range of the current model's first volume.
    ospcommon::vec2f voxelRange = modelStates[index].volumes[0]->voxelRange;
    
    if(voxelRange != ospcommon::vec2f(0.f)) {
      transferFunctionEditor->setDataValueRange(voxelRange);
      isosurfaceEditor->setDataValueRange(voxelRange);
    }
    
    // Update active volume on probe widget.
    probeWidget->setVolume(modelStates[index].volumes[0]->handle);
    
    // Update current filename information label.
    if (ownModelPerObject) {
      currentFilenameInfoLabel.setText(
        "<b>Timestep "
        + QString::number(index)
        + QString("</b>: Data value range: [")
        + QString::number(voxelRange.x)
        + ", "
        + QString::number(voxelRange.y)
        + "]"
      );
    } else {
      currentFilenameInfoLabel.setText(
        "<b>Timestep "
        + QString::number(index)
        + QString("</b>: ")
        + QString(objectFileFilenames[index].c_str()).split('/').back()
        + ". Data value range: ["
        + QString::number(voxelRange.x)
        + ", "
        + QString::number(voxelRange.y)
        + "]"
      );
    }
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
  if(osprayWindow == nullptr)
    return;

  if(autoRotateAction != nullptr)
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
  // Get filename if not specified.
  if(filename.empty()) {
    filename = QFileDialog::getOpenFileName(this,
                                            tr("Load geometry"),
                                            ".",
                                            "Geometry files (*)").toStdString();
  }

  if(filename.empty())
    return;

  ospcommon::FileName fn = filename;
  ospray::miniSG::Model* msgModel = new miniSG::Model;
  bool loadedSGScene = false;
  if (fn.ext() == "stl") {
    miniSG::importSTL(*msgModel,fn);
    loadedSGScene = true;
  } else if (fn.ext() == "msg") {
    miniSG::importMSG(*msgModel,fn);
    loadedSGScene = true;
  } else if (fn.ext() == "tri") {
    miniSG::importTRI_xyz(*msgModel,fn);
    loadedSGScene = true;
  } else if (fn.ext() == "xml") {
    miniSG::importRIVL(*msgModel,fn);
    loadedSGScene = true;
  } else if (fn.ext() == "obj") {
    miniSG::importOBJ(*msgModel,fn);
    loadedSGScene = true;
  } else if (fn.ext() == "hbp") {
    miniSG::importHBP(*msgModel,fn);
    loadedSGScene = true;
  } else if (fn.ext() == "x3d") {
    miniSG::importX3D(*msgModel,fn);
    loadedSGScene = true;
  } else if (fn.ext() == "astl") {
        // miniSG::importSTL(msgAnimation,fn);
        // loadedSGScene = true;
  } else {
    ospray::importer::Group *newStuff = ospray::importer::import(filename);
    if (!newStuff) return;
    if (newStuff->geometry.size() != 1) return;

    OSPGeometry triangleMesh = newStuff->geometry[0]->handle;
  // For now: if this is a DDS geometry, assume it is a horizon and its color should be mapped through the first volume's transfer function.
    if(QString(filename.c_str()).endsWith(".dds")
       && modelStates.size() > 0
       && modelStates[0].volumes.size() > 0) {

      OSPMaterial material = ospNewMaterial(renderer, "default");
      ospSet3f(material, "Kd", 1,1,1);
      ospSetObject(material, "volume", modelStates[0].volumes[0]->handle);
      ospCommit(material);

      ospSetMaterial(triangleMesh, material);
      ospCommit(triangleMesh);

      OSPModel modelInstance = ospNewModel();
      ospAddGeometry(modelInstance, triangleMesh);
      ospCommit(modelInstance);

      ospcommon::affine3f xfm = ospcommon::one;
      OSPGeometry triangleMeshInstance = ospNewInstance(modelInstance,
                                                        (osp::affine3f&)xfm);
      ospCommit(triangleMeshInstance);

      for(size_t i=0; i<modelStates.size(); i++) {
        ospAddGeometry(modelStates[i].model, triangleMeshInstance);
        ospCommit(modelStates[i].model);
      }
    }
  }

  if (loadedSGScene) {
    std::vector<OSPModel> instanceModels;

    for (size_t i = 0; i < msgModel->mesh.size(); i++) {
      Ref<miniSG::Mesh> msgMesh = msgModel->mesh[i];
      TriangleMeshSceneParser parser(ospray::cpp::Renderer(), "triangles");
      auto ospMesh = parser.createOSPRayGeometry(msgModel, msgMesh.ptr);

      OSPMaterial mat = ospNewMaterial(renderer, "OBJMaterial");
      ospSet3f(mat,"Kd",.8f,.8f,.8f);
      ospCommit(mat);
      ospSetMaterial(ospMesh.handle(), mat);
      ospCommit(ospMesh.handle());

      cpp::Model model_i;
      model_i.addGeometry(ospMesh);
      model_i.commit();
      instanceModels.push_back(model_i.handle());
    }

    for (size_t i = 0; i < msgModel->instance.size(); i++) {
      msgModel->instance[i].xfm = msgModel->instance[i].xfm*ospcommon::affine3f::translate(ospcommon::vec3f(16,16,.1));  // hack for landing gear
      OSPGeometry inst =
      ospNewInstance(instanceModels[msgModel->instance[i].meshID],
        reinterpret_cast<osp::affine3f&>(msgModel->instance[i].xfm));
      ospCommit(inst);
        // sceneModel->addGeometry(inst);
      for(size_t i=0; i<modelStates.size(); i++) {
        ospAddGeometry(modelStates[i].model, inst);
        ospCommit(modelStates[i].model);
      }
    }
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
  else if (event->key() == Qt::Key_P){
    std::cout << "View parameters (use on command line to reproduce view): " << std::endl
            << "  " << *(osprayWindow->getViewport()) << std::endl;
  }
  else if (event->key() == Qt::Key_L){
    std::cout << "Light parameters (use on command line to reproduce view): " << std::endl
            << "  " << *lightEditor << std::endl;
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
  if (osprayWindow != nullptr) {
    osprayWindow->resetAccumulationBuffer();
    osprayWindow->updateGL();
  }
}

void VolumeViewer::setRenderAnnotationsEnabled(bool value)
{
  if (value) {
    if (!annotationRenderer)
      annotationRenderer = new OpenGLAnnotationRenderer(this);

    connect(osprayWindow, SIGNAL(renderGLComponents()), annotationRenderer,
            SLOT(render()), Qt::UniqueConnection);
  }
  else {
    delete annotationRenderer;
    annotationRenderer = nullptr;
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
  if (gradientShadingEnabled != value)
  {
    for(size_t i=0; i<modelStates.size(); i++)
      for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
        ospSet1i(modelStates[i].volumes[j]->handle, "gradientShadingEnabled", value);
        ospCommit(modelStates[i].volumes[j]->handle);
      }

      render();
      gradientShadingEnabled = value;
      if (preferencesDialog)
        preferencesDialog->setGradientShadingEnabled(value);
    }
}

//! Set gradient shading flag on all volumes.
void VolumeViewer::setPreIntegration(bool value)
{
  if (preIntegration != value)
  {
    ospSet1i(transferFunction, "preIntegration", value);
    ospCommit(transferFunction);

    render();
    preIntegration = value;
    if (preferencesDialog)
      preferencesDialog->setPreIntegration(value);
  }
}

//! Set gradient shading flag on all volumes.
void VolumeViewer::setSingleShade(bool value)
{
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet1i(modelStates[i].volumes[j]->handle, "singleShade", value);
      ospCommit(modelStates[i].volumes[j]->handle);
  }

  render();
}

void VolumeViewer::setShadows(bool value)
{
  if (shadows != value)
  {
    ospSet1i(renderer, "shadowsEnabled", value);  
    if(rendererInitialized)
      ospCommit(renderer);

    render();
    shadows = value;
    if (preferencesDialog)
      preferencesDialog->setShadows(value);
  }
}

void VolumeViewer::setPlane(bool st)
{
  if (usePlane != st)
  {
    usePlane = st;
    if (planeMesh)
    {
      for(size_t i=0; i<modelStates.size(); i++) 
      {
        ospCommit(modelStates[i].model);
        if (usePlane)
          ospAddGeometry(modelStates[i].model, planeMesh);
        else
          ospRemoveGeometry(modelStates[i].model, planeMesh);
        ospCommit(modelStates[i].model);
      }
    }
    render();
    if (preferencesDialog)
      preferencesDialog->setPlane(st);
  }
}

void VolumeViewer::setAOWeight(double value)
{
  ospSet1f(renderer, "aoWeight", value);  
  if(rendererInitialized)
    ospCommit(renderer);
  render();
}

void VolumeViewer::setAOSamples(int value)
{
  if (aoSamples != value)
  {
    ospSet1i(renderer, "aoSamples", value);  
    if(rendererInitialized)
      ospCommit(renderer);
    render();
    aoSamples = value;
    if (preferencesDialog)
      preferencesDialog->setAOSamples(value);
  }
}

void VolumeViewer::setSPP(int value)
{
  if (spp != value)
  {
    ospSet1i(renderer, "spp", value);  
    if(rendererInitialized)
      ospCommit(renderer);
    render();
    spp = value;
    if (preferencesDialog)
      preferencesDialog->setSPP(value);
  }
}

//! Set gradient shading flag on all volumes.
void VolumeViewer::setAdaptiveScalar(double value)
{
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet1f(modelStates[i].volumes[j]->handle, "adaptiveScalar", value);
      ospCommit(modelStates[i].volumes[j]->handle);
  }
  render();
}


//! Set gradient shading flag on all volumes.
void VolumeViewer::setAdaptiveMaxSamplingRate(double value)
{
  if (adaptiveMaxSamplingRate != value)
  {
    for(size_t i=0; i<modelStates.size(); i++)
      for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
        ospSet1f(modelStates[i].volumes[j]->handle, "adaptiveMaxSamplingRate", value);
        ospCommit(modelStates[i].volumes[j]->handle);
    }
    render();
    adaptiveMaxSamplingRate = value;
    if (preferencesDialog)
      preferencesDialog->setAdaptiveMaxSamplingRate(value);
  }
}


//! Set gradient shading flag on all volumes.
void VolumeViewer::setAdaptiveBacktrack(double value)
{
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet1f(modelStates[i].volumes[j]->handle, "adaptiveBacktrack", value);
      ospCommit(modelStates[i].volumes[j]->handle);
  }

  render();
}

//! Set gradient shading flag on all volumes.
void VolumeViewer::setAdaptiveSampling(bool value)
{
  if (value != adaptiveSampling)
  {
    for(size_t i=0; i<modelStates.size(); i++)
      for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
        ospSet1i(modelStates[i].volumes[j]->handle, "adaptiveSampling", value);
        ospCommit(modelStates[i].volumes[j]->handle);
      }

      render();
      adaptiveSampling = value;
      if (preferencesDialog)
        preferencesDialog->setAdaptiveSampling(value);
  }
}

void VolumeViewer::setSamplingRate(double value)
{
  if (samplingRate != value)
  {
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet1f(modelStates[i].volumes[j]->handle, "samplingRate", value);
      ospCommit(modelStates[i].volumes[j]->handle);
    }

  render();
  samplingRate = value;
  if (preferencesDialog)
    preferencesDialog->setSamplingRate(value);
}
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
  ospray::importer::Group *imported = ospray::importer::import(filename);
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
    
    ospray::importer::Volume *vol = imported->volume[i];
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
    ospGetType(objects[i], nullptr, &type);
    
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
  exitOnCondition(renderer == nullptr, "could not create OSPRay renderer object");

  // Set renderer defaults (if not using 'aoX' renderers)
  if (renderer_type[0] != 'a' && renderer_type[1] != 'o') {
    ospSet1i(renderer, "aoSamples", 1);
    ospSet1i(renderer, "shadowsEnabled", 1);
    ospSet1i(renderer, "aoTransparencyEnabled", 1);
  }

  // Create OSPRay ambient and directional lights. GUI elements will modify their parameters.
  ambientLight = ospNewLight(renderer, "AmbientLight");
  exitOnCondition(ambientLight == nullptr, "could not create ambient light");
  ospSet3f(ambientLight, "color", 0.3f, 0.5f, 1.f);
  ospCommit(ambientLight);

  directionalLight = ospNewLight(renderer, "DirectionalLight");
  exitOnCondition(directionalLight == nullptr,
                  "could not create directional light");
  ospSet3f(directionalLight, "color", 1.f, 0.9f, 0.4f);
  ospCommit(directionalLight);

  // Set the light sources on the renderer.
  std::vector<OSPLight> lights;
  lights.push_back(ambientLight);
  lights.push_back(directionalLight);

  ospSetData(renderer, "lights", ospNewData(lights.size(), OSP_OBJECT, &lights[0]));

  // Create an OSPRay transfer function.
  auto tfFromEnv = getEnvVar<std::string>("OSPRAY_USE_TF_TYPE");

  if (tfFromEnv.first) {
    transferFunction = ospNewTransferFunction(tfFromEnv.second.c_str());
  } else {
    transferFunction = ospNewTransferFunction("piecewise_linear");
  }
  exitOnCondition(transferFunction == nullptr,
                  "could not create OSPRay transfer function object");
  ospCommit(transferFunction);

  // Load OSPRay objects from files.
  for (size_t i=0 ; i < objectFileFilenames.size() ; i++)
    importObjectsFromFile(objectFileFilenames[i]);

  boundingBox = ospcommon::empty;
  if (!modelStates.empty()) {
    for (size_t i=0; i<modelStates[0].volumes.size(); i++) 
      boundingBox.extend(modelStates[0].volumes[i]->boundingBox);
  } else {
    modelStates.push_back(ospNewModel());
  }
  PRINT(boundingBox);

  //add plane
  OSPMaterial planeMaterial = ospNewMaterial(renderer,"default");
  ospSet3f(planeMaterial,"Kd",.5,.5,.5);
  ospSet3f(planeMaterial,"Ks",0,0,0);
  ospSet1f(planeMaterial,"Ns",0);
  ospCommit(planeMaterial);

  osp::vec3f *vertices = new osp::vec3f[4];
  float ps = 1000.f;
  float py = boundingBox.upper.y+0.1f;
#if 1
  vertices[0] = osp::vec3f{-ps, -ps, py};
  vertices[1] = osp::vec3f{-ps,  ps, py};
  vertices[2] = osp::vec3f{ ps, -ps, py};
  vertices[3] = osp::vec3f{ ps,  ps, py};
#else
  vertices[0] = osp::vec3f{-ps, py, -ps};
  vertices[1] = osp::vec3f{-ps, py, ps};
  vertices[2] = osp::vec3f{ps, py, -ps};
  vertices[3] = osp::vec3f{ps, py, ps};
#endif

  planeMesh = ospNewGeometry("triangles");
  OSPData position = ospNewData(4, OSP_FLOAT3, &vertices[0]);
  ospCommit(position);
  ospSetData(planeMesh, "vertex", position);

  osp::vec3i *triangles = new osp::vec3i[2];
  triangles[0] = osp::vec3i{0,1,2};
  triangles[1] = osp::vec3i{1,2,3};

  OSPData index = ospNewData(2, OSP_INT3, &triangles[0]);
  ospCommit(index);
  ospSetData(planeMesh, "index", index); 
  delete[] triangles;

  ospSetMaterial(planeMesh, planeMaterial);
  ospCommit(planeMesh);
  setPlane(usePlane);
  ospRelease(index);

  osp::vec3f specular = osp::vec3f{0.135f,0.135f,0.135f};
  for(size_t i=0; i<modelStates.size(); i++)
    for(size_t j=0; j<modelStates[i].volumes.size(); j++) {
      ospSet3fv(modelStates[i].volumes[j]->handle, "specular", &specular.x);
      ospCommit(modelStates[i].volumes[j]->handle);
  }
}

void VolumeViewer::initUserInterfaceWidgets()
{
  // Create a toolbar at the top of the window.
  QToolBar *toolbar = addToolBar("toolbar");

  // Add preferences widget and callback.
  preferencesDialog = new PreferencesDialog(this, boundingBox);
  QAction *showPreferencesAction = new QAction("Preferences", this);
  connect(showPreferencesAction, SIGNAL(triggered()), preferencesDialog, SLOT(show()));
  toolbar->addAction(showPreferencesAction);
  preferencesDialog->setSamplingRate(samplingRate);
  preferencesDialog->setAdaptiveMaxSamplingRate(adaptiveMaxSamplingRate);

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

  // Create the transfer function editor dock widget, this widget modifies
  // the transfer function directly.
  QDockWidget *transferFunctionEditorDockWidget = new QDockWidget("Transfer Function", this);
  transferFunctionEditor = new TransferFunctionEditor(transferFunction);
  transferFunctionEditorDockWidget->setWidget(transferFunctionEditor);
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(commitVolumes()));
  connect(transferFunctionEditor, SIGNAL(committed()), this, SLOT(render()));
  addDockWidget(Qt::LeftDockWidgetArea, transferFunctionEditorDockWidget);

  // Set the transfer function editor widget to its minimum allowed height,
  // to leave room for other dock widgets.
  transferFunctionEditor->setMaximumHeight(transferFunctionEditor->minimumSize().height());

  // Create slice editor dock widget.
  QDockWidget *sliceEditorDockWidget = new QDockWidget("Slices", this);
  sliceEditor = new SliceEditor(boundingBox);
  sliceEditorDockWidget->setWidget(sliceEditor);
  connect(sliceEditor, SIGNAL(slicesChanged(std::vector<SliceParameters>)),
          this, SLOT(setSlices(std::vector<SliceParameters>)));
  addDockWidget(Qt::LeftDockWidgetArea, sliceEditorDockWidget);

  // Create isosurface editor dock widget.
  QDockWidget *isosurfaceEditorDockWidget = new QDockWidget("Isosurfaces", this);
  isosurfaceEditor = new IsosurfaceEditor();
  isosurfaceEditorDockWidget->setWidget(isosurfaceEditor);
  connect(isosurfaceEditor, SIGNAL(isovaluesChanged(std::vector<float>)),
          this, SLOT(setIsovalues(std::vector<float>)));
  addDockWidget(Qt::LeftDockWidgetArea, isosurfaceEditorDockWidget);

  // Create the light editor dock widget, this widget modifies the light directly.
  QDockWidget *lightEditorDockWidget = new QDockWidget("Lights", this);
  lightEditor = new LightEditor(ambientLight, directionalLight);
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
