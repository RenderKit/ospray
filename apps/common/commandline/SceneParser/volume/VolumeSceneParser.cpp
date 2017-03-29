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

#include "VolumeSceneParser.h"

#include <ospray/ospray_cpp/Data.h>

#include "common/importer/Importer.h"
#include "common/tfn_lib/tfn_lib.h"

#include <iostream>
#include <cstdlib>

namespace commandline {

  using namespace ospray;
  using namespace ospcommon;

  using std::cerr;
  using std::endl;

  // SceneParser definitions ////////////////////////////////////////////////////

  VolumeSceneParser::VolumeSceneParser(cpp::Renderer renderer) :
    renderer(renderer)
  {
  }

  bool VolumeSceneParser::parse(int ac, const char **&av)
  {
    bool loadedScene = false;
    bool loadedTransferFunction = false;

    FileName scene;

    for (int i = 1; i < ac; i++) {
      const std::string arg = av[i];
      if (arg == "-s" || arg == "--sampling-rate") {
        samplingRate = atof(av[++i]);
      } else if (arg == "-tfc" || arg == "--tf-color") {
        ospcommon::vec4f color;
        color.x = atof(av[++i]);
        color.y = atof(av[++i]);
        color.z = atof(av[++i]);
        color.w = atof(av[++i]);
        tf_colors.push_back(color);
      } else if (arg == "-tfs" || arg == "--tf-scale") {
        tf_scale = atof(av[++i]);
      } else if (arg == "-tff" || arg == "--tf-file") {
        importTransferFunction(std::string(av[++i]));
        loadedTransferFunction = true;
      } else if (arg == "-is" || arg == "--surface") {
        isosurfaces.push_back(atof(av[++i]));
      } else {
        FileName fn = arg;
        if (fn.ext() == "osp") {
          scene = arg;
          loadedScene = true;
        }
      }
    }

    if (loadedScene) {
      sceneModel = make_unique<cpp::Model>();
      if (!loadedTransferFunction) {
        createDefaultTransferFunction();
      }
      importObjectsFromFile(scene, loadedTransferFunction);
    }

    return loadedScene;
  }

  std::deque<cpp::Model> VolumeSceneParser::model() const
  {
    std::deque<cpp::Model> models;
    models.push_back(sceneModel == nullptr ? cpp::Model() : *sceneModel);
    return models;
    // return sceneModel.get() == nullptr ? cpp::Model() : *sceneModel;
  }

  std::deque<box3f> VolumeSceneParser::bbox() const
  {
    std::deque<ospcommon::box3f> boxes;
    boxes.push_back(sceneBbox);
    return boxes;
  }

  void VolumeSceneParser::importObjectsFromFile(const std::string &filename,
                                                bool loadedTransferFunction)
  {
    auto &model = *sceneModel;

    // Load OSPRay objects from a file.
    ospray::importer::Group *imported = ospray::importer::import(filename);

    // Iterate over geometries
    for (size_t i = 0; i < imported->geometry.size(); i++) {
      auto geometry = ospray::cpp::Geometry(imported->geometry[i]->handle);
      geometry.commit();
      model.addGeometry(geometry);
    }

    // Iterate over volumes
    for (size_t i = 0 ; i < imported->volume.size(); i++) {
      ospray::importer::Volume *vol = imported->volume[i];
      auto volume = ospray::cpp::Volume(vol->handle);

      // For now we set the same transfer function on all volumes.
      volume.set("transferFunction", transferFunction);
      volume.set("samplingRate", samplingRate);
      volume.commit();

      // Add the loaded volume(s) to the model.
      model.addVolume(volume);

      // Set the minimum and maximum values in the domain for both color and
      // opacity components of the transfer function if we didn't load a transfer
      // function for a file (in that case this is already set)
      if (!loadedTransferFunction) {
        transferFunction.set("valueRange", vol->voxelRange.x, vol->voxelRange.y);
        transferFunction.commit();
      }

      //sceneBbox.extend(vol->bounds);
      sceneBbox = vol->bounds;

      // Create any specified isosurfaces
      if (!isosurfaces.empty()) {
        auto isoValueData = ospray::cpp::Data(isosurfaces.size(), OSP_FLOAT,
                                              isosurfaces.data());
        auto isoGeometry = ospray::cpp::Geometry("isosurfaces");

        isoGeometry.set("isovalues", isoValueData);
        isoGeometry.set("volume", volume);
        isoGeometry.commit();

        model.addGeometry(isoGeometry);
      }
    }

    model.commit();
  }

  void VolumeSceneParser::importTransferFunction(const std::string &filename)
  {
    tfn::TransferFunction fcn(filename);
    auto colorsData = ospray::cpp::Data(fcn.rgbValues.size(), OSP_FLOAT3,
                                        fcn.rgbValues.data());
    auto tfFromEnv = getEnvVar<std::string>("OSPRAY_USE_TF_TYPE");

    if (tfFromEnv.first) {
      transferFunction = cpp::TransferFunction(tfFromEnv.second);
    } else {
      transferFunction = cpp::TransferFunction("piecewise_linear");
    }
    transferFunction.set("colors", colorsData);

    tf_scale = fcn.opacityScaling;
    // Sample the opacity values, taking 256 samples to match the volume viewer
    // the volume viewer does the sampling a bit differently so we match that
    // instead of what's done in createDefault
    std::vector<float> opacityValues;
    const int N_OPACITIES = 256;
    size_t lo = 0;
    size_t hi = 1;
    for (int i = 0; i < N_OPACITIES; ++i) {
      const float x = float(i) / float(N_OPACITIES - 1);
      float opacity = 0;
      if (i == 0) {
        opacity = fcn.opacityValues[0].y;
      } else if (i == N_OPACITIES - 1) {
        opacity = fcn.opacityValues.back().y;
      } else {
        // If we're over this val, find the next segment
        if (x > fcn.opacityValues[lo].x) {
          for (size_t j = lo; j < fcn.opacityValues.size() - 1; ++j) {
            if (x <= fcn.opacityValues[j + 1].x) {
              lo = j;
              hi = j + 1;
              break;
            }
          }
        }
        const float delta = x - fcn.opacityValues[lo].x;
        const float interval = fcn.opacityValues[hi].x - fcn.opacityValues[lo].x;
        if (delta == 0 || interval == 0) {
          opacity = fcn.opacityValues[lo].y;
        } else {
          opacity = fcn.opacityValues[lo].y + delta / interval
            * (fcn.opacityValues[hi].y - fcn.opacityValues[lo].y);
        }
      }
      opacityValues.push_back(tf_scale * opacity);
    }

    auto opacityValuesData = ospray::cpp::Data(opacityValues.size(),
                                               OSP_FLOAT,
                                               opacityValues.data());
    transferFunction.set("opacities", opacityValuesData);
    transferFunction.set("valueRange", vec2f(fcn.dataValueMin, fcn.dataValueMax));

    // Commit transfer function
    transferFunction.commit();
  }
  void VolumeSceneParser::createDefaultTransferFunction()
  {
    auto tfFromEnv = getEnvVar<std::string>("OSPRAY_USE_TF_TYPE");

    if (tfFromEnv.first) {
      transferFunction = cpp::TransferFunction(tfFromEnv.second);
    } else {
      transferFunction = cpp::TransferFunction("piecewise_linear");
    }

    // Add colors
    std::vector<vec4f> colors;
    if (tf_colors.empty()) {
      colors.emplace_back(0.f, 0.f, 0.f, 0.f);
      colors.emplace_back(0.9f, 0.9f, 0.9f, 1.f);
    } else {
      colors = tf_colors;
    }
    std::vector<vec3f> colorsAsVec3;
    for (auto &c : colors) colorsAsVec3.emplace_back(c.x, c.y, c.z);
    auto colorsData = ospray::cpp::Data(colors.size(), OSP_FLOAT3,
                                        colorsAsVec3.data());
    transferFunction.set("colors", colorsData);

    // Add opacities
    std::vector<float> opacityValues;

    const int N_OPACITIES = 64;//NOTE(jda) - This affects image quality and
    //            performance!
    const int N_INTERVALS = colors.size() - 1;
    const float OPACITIES_PER_INTERVAL = N_OPACITIES / float(N_INTERVALS);
    for (int i = 0; i < N_OPACITIES; ++i) {
      int lcolor = static_cast<int>(i/OPACITIES_PER_INTERVAL);
      int hcolor = lcolor + 1;

      float v0 = colors[lcolor].w;
      float v1 = colors[hcolor].w;
      float t = (i / OPACITIES_PER_INTERVAL) - lcolor;

      float opacity = (1-t)*v0 + t*v1;
      if (opacity > 1.f) opacity = 1.f;
      opacityValues.push_back(tf_scale*opacity);
    }
    auto opacityValuesData = ospray::cpp::Data(opacityValues.size(),
                                               OSP_FLOAT,
                                               opacityValues.data());
    transferFunction.set("opacities", opacityValuesData);

    // Commit transfer function
    transferFunction.commit();
  }

} // ::commandline
