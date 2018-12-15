// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "Volume.h"
#include "../common/Model.h"
// core ospray
#include "ospray/common/OSPCommon.h"

namespace ospray {
  namespace sg {

    /*! helper function to help build voxel ranges during parsing */
    template<typename T>
    inline void extendVoxelRange(ospcommon::vec2f &voxelRange,
                                 const T *voxel, size_t num)
    {
      for (size_t i = 0; i < num; ++i) {
        if (!std::isnan(static_cast<float>(voxel[i]))) {
          voxelRange.x = std::min(voxelRange.x, static_cast<float>(voxel[i]));
          voxelRange.y = std::max(voxelRange.y, static_cast<float>(voxel[i]));
        }
      }
    }

    //! Convenient wrapper that will do the template dispatch for you based on
    //  the voxelType passed
    inline void extendVoxelRange(ospcommon::vec2f &voxelRange,
                                 const OSPDataType voxelType,
                                 const unsigned char *voxels,
                                 const size_t numVoxels)
    {
      switch (voxelType) {
        case OSP_UCHAR:
          extendVoxelRange(voxelRange, voxels, numVoxels);
          break;
        case OSP_SHORT:
          extendVoxelRange(voxelRange,
                           reinterpret_cast<const short*>(voxels),
                           numVoxels);
          break;
        case OSP_USHORT:
          extendVoxelRange(voxelRange,
                           reinterpret_cast<const unsigned short*>(voxels),
                           numVoxels);
          break;
        case OSP_FLOAT:
          extendVoxelRange(voxelRange,
                           reinterpret_cast<const float*>(voxels),
                           numVoxels);
          break;
        case OSP_DOUBLE:
          extendVoxelRange(voxelRange,
                           reinterpret_cast<const double*>(voxels),
                           numVoxels);
          break;
        default:
          throw std::runtime_error("sg::extendVoxelRange: unsupported voxel type!");
      }
    }

    bool unsupportedVoxelType(const std::string &type) {
      return type != "uchar" && type != "ushort" && type != "short"
        && type != "float" && type != "double";
    }

    // =======================================================
    // base volume class
    // =======================================================

    Volume::Volume()
    {
      setValue((OSPVolume)nullptr);

      createChild("transferFunction", "TransferFunction");
      createChild("gradientShadingEnabled", "bool", true);
      createChild("preIntegration", "bool", false);
      createChild("singleShade", "bool", true);
      createChild("voxelRange", "vec2f",
                  vec2f(std::numeric_limits<float>::infinity(),
                        -std::numeric_limits<float>::infinity()));
      createChild("adaptiveSampling", "bool", true);
      createChild("adaptiveScalar", "float", 15.f);
      createChild("adaptiveBacktrack", "float", 0.03f);
      createChild("samplingRate", "float", 0.06f);
      createChild("adaptiveMaxSamplingRate", "float", 0.5f);
      createChild("volumeClippingBoxLower", "vec3f", vec3f(0.f));
      createChild("volumeClippingBoxUpper", "vec3f", vec3f(0.f));
      createChild("specular", "vec3f", vec3f(0.3f));
      createChild("ns", "float", 20.f);
      createChild("gridOrigin", "vec3f", vec3f(0.0f));
      createChild("gridSpacing", "vec3f", vec3f(1.f));
      createChild("isosurfaceEnabled", "bool", false);
      createChild("isosurface", "float",
                  128.f,
                  NodeFlags::gui_slider).setMinMax(0.f,255.f);
    }

    Volume::~Volume()
    {
      if (isosurfacesGeometry)
        ospRelease(isosurfacesGeometry);
    }

    std::string Volume::toString() const
    {
      return "ospray::sg::Volume";
    }

    void Volume::serialize(sg::Serialization::State &state)
    {
      Node::serialize(state);
    }

    void Volume::postCommit(RenderContext &)
    {
      auto ospVolume = valueAs<OSPVolume>();
      ospSetObject(ospVolume, "transferFunction",
                   child("transferFunction").valueAs<OSPTransferFunction>());
      ospCommit(ospVolume);
    }

    void Volume::postRender(RenderContext &ctx)
    {
      auto ospVolume = valueAs<OSPVolume>();

      if (ospVolume) {
        ospAddVolume(ctx.world->valueAs<OSPModel>(), ospVolume);
        if (child("isosurfaceEnabled").valueAs<bool>() && isosurfacesGeometry)
          ospAddGeometry(ctx.world->valueAs<OSPModel>(), isosurfacesGeometry);
      }
    }

    // =======================================================
    // structured volume class
    // =======================================================

    StructuredVolume::StructuredVolume()
    {
      createChild("dimensions", "vec3i", NodeFlags::gui_readonly);
      createChild("voxelType", "string",
                  std::string("<undefined>"), NodeFlags::gui_readonly);
    }

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolume::toString() const
    {
      return "ospray::sg::StructuredVolume";
    }

    //! return bounding box of all primitives
    box3f StructuredVolume::computeBounds() const
    {
      box3f bbox = bounds();

      if (bbox != box3f(empty))
        return bbox;

      auto dimensions  = child("dimensions").valueAs<vec3i>();
      auto gridSpacing = child("gridSpacing").valueAs<vec3f>();

      bbox = box3f(vec3f(0.f), dimensions * gridSpacing);

      child("bounds") = bbox;

      return bbox;
    }

    void StructuredVolume::preCommit(RenderContext &)
    {
      auto ospVolume = valueAs<OSPVolume>();

      if (ospVolume) {
        ospCommit(ospVolume);
        if (child("isosurfaceEnabled").valueAs<bool>() == true
            && isosurfacesGeometry) {
          OSPData isovaluesData = ospNewData(1, OSP_FLOAT,
            &child("isosurface").valueAs<float>());
          ospSetData(isosurfacesGeometry, "isovalues", isovaluesData);
          ospCommit(isosurfacesGeometry);
        }
        return;
      }

      auto voxelType  = child("voxelType").valueAs<std::string>();
      auto dimensions = child("dimensions").valueAs<vec3i>();

      if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
        throw std::runtime_error("StructuredVolume::render(): "
                                 "invalid volume dimensions");
      }

      ospVolume = ospNewVolume("shared_structured_volume");
      setValue(ospVolume);

      if (isosurfacesGeometry)
        ospRelease(isosurfacesGeometry);

      isosurfacesGeometry = ospNewGeometry("isosurfaces");
      ospSetObject(isosurfacesGeometry, "volume", ospVolume);

      vec2f voxelRange(std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity());
      const OSPDataType ospVoxelType = typeForString(voxelType);

      const size_t nVoxels =
          (size_t)dimensions.x * (size_t)dimensions.y * (size_t)dimensions.z;

      auto voxels_data_node = child("voxelData").nodeAs<DataBuffer>();

      auto *voxels = static_cast<uint8_t*>(voxels_data_node->base());

      extendVoxelRange(voxelRange, ospVoxelType, voxels, nVoxels);

      child("voxelRange") = voxelRange;
      child("transferFunction")["valueRange"] = voxelRange;

      child("isosurface").setMinMax(voxelRange.x, voxelRange.y);
      float iso = child("isosurface").valueAs<float>();
      if (iso < voxelRange.x || iso > voxelRange.y)
        child("isosurface") = (voxelRange.y - voxelRange.x) / 2.f;
    }

    OSP_REGISTER_SG_NODE(StructuredVolume);

    // =======================================================
    // structured volume that is stored in a separate file (ie, a file
    // other than the ospbin file)
    // =======================================================

    StructuredVolumeFromFile::StructuredVolumeFromFile()
    {
      createChild("blockBricked", "bool", true, NodeFlags::gui_readonly);
    }

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolumeFromFile::toString() const
    {
      return "ospray::sg::StructuredVolumeFromFile";
    }

    void StructuredVolumeFromFile::preCommit(RenderContext &)
    {
      auto ospVolume = valueAs<OSPVolume>();

      if (ospVolume) {
        ospCommit(ospVolume);
        if (child("isosurfaceEnabled").valueAs<bool>() == true
            && isosurfacesGeometry) {
          OSPData isovaluesData = ospNewData(1, OSP_FLOAT,
            &child("isosurface").valueAs<float>());
          ospSetData(isosurfacesGeometry, "isovalues", isovaluesData);
          ospCommit(isosurfacesGeometry);
        }
        return;
      }

      bool useBlockBricked = child("blockBricked").valueAs<bool>();
      ospVolume = ospNewVolume(useBlockBricked ? "block_bricked_volume" :
                                                 "shared_structured_volume");

      setValue(ospVolume);
    }

    void StructuredVolumeFromFile::postCommit(RenderContext &ctx)
    {
      auto ospVolume = valueAs<OSPVolume>();

      auto dimensions = child("dimensions").valueAs<vec3i>();
      if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
        throw std::runtime_error("StructuredVolume::render(): "
                                 "invalid volume dimensions");
      }

      if (!fileLoaded) {
        auto voxelType  = child("voxelType").valueAs<std::string>();

        if (isosurfacesGeometry)
          ospRelease(isosurfacesGeometry);

        isosurfacesGeometry = ospNewGeometry("isosurfaces");
        ospSetObject(isosurfacesGeometry, "volume", ospVolume);

        FileName realFileName = fileNameOfCorrespondingXmlDoc.path() + fileName;
        FILE *file = fopen(realFileName.c_str(),"rb");
        if (!file) {
          throw std::runtime_error("StructuredVolumeFromFile::render(): could not open file '"
                                   +realFileName.str()+"' (expanded from xml file '"
                                   +fileNameOfCorrespondingXmlDoc.str()
                                   +"' and file name '"+fileName+"')");
        }

        vec2f voxelRange(std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity());
        const OSPDataType ospVoxelType = typeForString(voxelType);
        const size_t voxelSize = sizeOf(ospVoxelType);

        bool useBlockBricked = child("blockBricked").valueAs<bool>();

        if (useBlockBricked) {
          const size_t nPerSlice = (size_t)dimensions.x * (size_t)dimensions.y;
          std::vector<uint8_t> slice(nPerSlice * voxelSize, 0);

          for (int z = 0; z < dimensions.z; ++z) {
            if (fread(slice.data(), voxelSize, nPerSlice, file) != nPerSlice) {
              throw std::runtime_error("StructuredVolume::render(): read incomplete slice "
                  "data ... partial file or wrong format!?");
            }
            const vec3i region_lo(0, 0, z);
            const vec3i region_sz(dimensions.x, dimensions.y, 1);
            extendVoxelRange(voxelRange, ospVoxelType, slice.data(), nPerSlice);
            ospSetRegion(ospVolume,
                         slice.data(),
                         (const osp::vec3i&)region_lo,
                         (const osp::vec3i&)region_sz);
          }
        } else {
          const size_t nVoxels = (size_t)dimensions.x * (size_t)dimensions.y * (size_t)dimensions.z;
          uint8_t *voxels = new uint8_t[nVoxels * voxelSize];
          if (fread(voxels, voxelSize, nVoxels, file) != nVoxels) {
            THROW_SG_ERROR("read incomplete data (truncated file or "
                           "wrong format?!)");
          }
          extendVoxelRange(voxelRange, ospVoxelType, voxels, nVoxels);
          OSPData data = ospNewData(nVoxels, ospVoxelType, voxels, OSP_DATA_SHARED_BUFFER);
          ospSetData(ospVolume,"voxelData",data);
        }

        fclose(file);

        child("voxelRange") = voxelRange;
        child("transferFunction")["valueRange"] = voxelRange;

        child("isosurface").setMinMax(voxelRange.x, voxelRange.y);
        float iso = child("isosurface").valueAs<float>();
        if (iso < voxelRange.x || iso > voxelRange.y)
          child("isosurface") = (voxelRange.y - voxelRange.x) / 2.f;

        fileLoaded = true;

        // Double ugly: This is re-done by postCommit, but we need to
        // do it here so the transferFunction is set before we commit the
        // volume, so that it's in a valid state.
        ospSetObject(ospVolume, "transferFunction",
                     child("transferFunction").valueAs<OSPTransferFunction>());
        commit(); //<-- UGLY, recommitting the volume after child changes...
      }
      Volume::postCommit(ctx);
    }

    OSP_REGISTER_SG_NODE(StructuredVolumeFromFile);

  } // ::ospray::sg
} // ::ospray
