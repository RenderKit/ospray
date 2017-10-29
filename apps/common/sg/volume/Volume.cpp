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

#include <cstdio>
#include <thread>
#include <atomic>
#include <mutex>
#include "Volume.h"
#include "sg/common/World.h"

namespace ospray {
  namespace sg {

    /*! helper function to help build voxel ranges during parsing */
    template<typename T>
    inline void extendVoxelRange(ospcommon::vec2f &voxelRange,
                                 const T *voxel, size_t num)
    {
      for (size_t i = 0; i < num; ++i) {
        voxelRange.x = std::min(voxelRange.x, static_cast<float>(voxel[i]));
        voxelRange.y = std::max(voxelRange.y, static_cast<float>(voxel[i]));
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
      createChild("preIntegration", "bool", true);
      createChild("singleShade", "bool", true);
      createChild("voxelRange", "vec2f",
                  vec2f(std::numeric_limits<float>::infinity(),
                        -std::numeric_limits<float>::infinity()));
      createChild("adaptiveSampling", "bool", true);
      createChild("adaptiveScalar", "float", 15.f);
      createChild("adaptiveBacktrack", "float", 0.03f);
      createChild("samplingRate", "float", 0.125f);
      createChild("adaptiveMaxSamplingRate", "float", 2.f);
      createChild("volumeClippingBoxLower", "vec3f", vec3f(0.f));
      createChild("volumeClippingBoxUpper", "vec3f", vec3f(0.f));
      createChild("specular", "vec3f", vec3f(0.3f));
      createChild("ns", "float", 20.f);
      createChild("gridOrigin", "vec3f", vec3f(0.0f));
      createChild("gridSpacing", "vec3f", vec3f(1.f));
      createChild("isosurfaceEnabled", "bool", false);
      createChild("isosurface", "float",
                  -std::numeric_limits<float>::infinity(),
                  NodeFlags::valid_min_max |
                  NodeFlags::gui_slider).setMinMax(0.f,255.f);
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
        ospAddVolume(ctx.world->ospModel(), ospVolume);
        if (child("isosurfaceEnabled").valueAs<bool>() && isosurfacesGeometry)
          ospAddGeometry(ctx.world->ospModel(), isosurfacesGeometry);
      }
    }

    // =======================================================
    // structured volume class
    // =======================================================

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolume::toString() const
    {
      return "ospray::sg::StructuredVolume";
    }

    //! return bounding box of all primitives
    box3f StructuredVolume::bounds() const
    {
      return {vec3f(0.f),
              vec3f(dimensions)*child("gridSpacing").valueAs<vec3f>()};
    }

    //! \brief Initialize this node's value from given XML node

    void StructuredVolume::setFromXML(const xml::Node &, const unsigned char *)
    {
      NOT_IMPLEMENTED;
    }

    OSP_REGISTER_SG_NODE(StructuredVolume);

    // =======================================================
    // structured volume that is stored in a separate file (ie, a file
    // other than the ospbin file)
    // =======================================================

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolumeFromFile::toString() const
    {
      return "ospray::sg::StructuredVolumeFromFile";
    }

    //! \brief Initialize this node's value from given XML node
    void StructuredVolumeFromFile::setFromXML(const xml::Node &node,
                                              const unsigned char *)
    {
      voxelType = node.getProp("voxelType");
      if (voxelType == "uint8") voxelType = "uchar";
      dimensions = toVec3i(node.getProp("dimensions").c_str());
      fileName = node.getProp("fileName");

      if (fileName.empty()) {
        throw std::runtime_error("sg::StructuredVolumeFromFile: "
                                 "no 'fileName' specified");
      }
      if (unsupportedVoxelType(voxelType)) {
        THROW_SG_ERROR("unknown StructuredVolume.voxelType '"+voxelType+"'");
      }

      fileNameOfCorrespondingXmlDoc = node.doc->fileName;

      std::cout << "#osp:sg: created StructuredVolume from XML file, "
                << "dimensions = " << dimensions << std::endl;
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

      if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
        throw std::runtime_error("StructuredVolume::render(): "
                                 "invalid volume dimensions");
      }

      bool useBlockBricked = true;
      ospVolume = ospNewVolume(useBlockBricked ? "block_bricked_volume" :
                                                 "shared_structured_volume");

      if (!ospVolume)
        THROW_SG_ERROR("could not allocate volume");

      isosurfacesGeometry = ospNewGeometry("isosurfaces");
      ospSetObject(isosurfacesGeometry, "volume", ospVolume);

      setValue(ospVolume);

      ospSetString(ospVolume,"voxelType",voxelType.c_str());
      ospSetVec3i(ospVolume,"dimensions",(const osp::vec3i&)dimensions);

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
    }

    OSP_REGISTER_SG_NODE(StructuredVolumeFromFile);

    // =======================================================
    // Richtmyer-Meshkov volume class and utils
    // =======================================================

    RichtmyerMeshkov::RichtmyerMeshkov()
    {
      dimensions = vec3i(2048, 2048, 1920);
    }

    std::string RichtmyerMeshkov::toString() const
    {
      return "ospray::sg::RichtmyerMeshkov";
    }

    //! \brief Initialize this node's value from given XML node
    void RichtmyerMeshkov::setFromXML(const xml::Node &node,
                                      const unsigned char *)
    {
      dirName = node.getProp("dirName");
      const std::string t = node.getProp("timeStep");
      if (t.empty()) {
        THROW_SG_ERROR("sg::RichtmyerMeshkov: no 'timeStep' specified");
      }
      timeStep = std::stoi(t);

      if (dirName.empty()) {
        THROW_SG_ERROR("sg::RichtmyerMeshkov: no 'dirName' specified");
      }

      fileNameOfCorrespondingXmlDoc = node.doc->fileName;
    }

    void RichtmyerMeshkov::preCommit(RenderContext &)
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

      ospVolume = ospNewVolume("block_bricked_volume");

      if (!ospVolume)
        THROW_SG_ERROR("could not allocate volume");

      isosurfacesGeometry = ospNewGeometry("isosurfaces");
      ospSetObject(isosurfacesGeometry, "volume", ospVolume);

      setValue(ospVolume);

      ospSetString(ospVolume, "voxelType", "uchar");
      ospSetVec3i(ospVolume, "dimensions", (const osp::vec3i&)dimensions);

      const FileName realFileName = fileNameOfCorrespondingXmlDoc.path() + dirName;

      // Launch loader threads to load the volume
      LoaderState loaderState(fileNameOfCorrespondingXmlDoc.path() + dirName, timeStep);
      std::vector<std::thread> threads;
      createChild("blocksLoaded", "string",
                  "0/" + std::to_string(LoaderState::NUM_BLOCKS));

      for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
        threads.push_back(std::thread([&](){ loaderThread(loaderState); }));

      for (auto &t : threads)
        t.join();

      child("voxelRange") = loaderState.voxelRange;
      child("isosurface").setMinMax(loaderState.voxelRange.x,
                                    loaderState.voxelRange.y);
      float iso = child("isosurface").valueAs<float>();
      if (iso < loaderState.voxelRange.x || iso > loaderState.voxelRange.y) {
        child("isosurface") = (loaderState.voxelRange.y -
                               loaderState.voxelRange.x) / 2.f;
      }

      child("transferFunction")["valueRange"] = loaderState.voxelRange;
    }

    void RichtmyerMeshkov::loaderThread(LoaderState &state)
    {
      auto ospVolume = valueAs<OSPVolume>();
      Node &progressLog = child("blocksLoaded");
      std::vector<uint8_t> block(LoaderState::BLOCK_SIZE, 0);
      while (true) {
        const size_t blockID = state.loadNextBlock(block);
        if (blockID >= LoaderState::NUM_BLOCKS) {
          break;
        }

        const int I = blockID % 8;
        const int J = (blockID / 8) % 8;
        const int K = (blockID / 64);

        vec2f blockRange(block[0]);
        extendVoxelRange(blockRange, &block[0], LoaderState::BLOCK_SIZE);
        const vec3i region_lo(I * 256, J * 256, K * 128);
        const vec3i region_sz(256, 256, 128);
        {
          std::lock_guard<std::mutex> lock(state.mutex);
          ospSetRegion(ospVolume,
                       block.data(),
                       (const osp::vec3i&)region_lo,
                       (const osp::vec3i&)region_sz);

          state.voxelRange.x = std::min(state.voxelRange.x, blockRange.x);
          state.voxelRange.y = std::max(state.voxelRange.y, blockRange.y);
          progressLog.setValue(std::to_string(blockID + 1) + "/"
                               + std::to_string(LoaderState::NUM_BLOCKS));
        }
      }
    }

    const size_t RichtmyerMeshkov::LoaderState::BLOCK_SIZE = 256 * 256 * 128;
    const size_t RichtmyerMeshkov::LoaderState::NUM_BLOCKS = 8 * 8 * 15;

    RichtmyerMeshkov::LoaderState::LoaderState(const FileName &fullDirName,
                                               const int timeStep)
      : nextBlockID(0),
        useGZip(getenv("OSPRAY_RM_NO_GZIP") == nullptr),
        fullDirName(fullDirName),
        timeStep(timeStep),
        voxelRange(std::numeric_limits<float>::infinity(),
                   -std::numeric_limits<float>::infinity())
    {}

    size_t RichtmyerMeshkov::LoaderState::loadNextBlock(std::vector<uint8_t> &b)
    {
      const size_t blockID = nextBlockID.fetch_add(1);
      if (blockID >= NUM_BLOCKS) {
        return blockID;
      }

      // The RM Bob filenames are at most 15 characters long,
      // in the case of gzipped ones
      char bobName[16] = {0};
      FILE *file = NULL;
      if (useGZip) {
#ifndef _WIN32
        if (std::snprintf(bobName, 16, "d_%04d_%04li.gz", timeStep, blockID) > 15) {
          THROW_SG_ERROR("sg::RichtmyerMeshkov: Invalid timestep or blockID!");
        }
        const FileName fileName = fullDirName + FileName(bobName);
        const std::string cmd = "gunzip -c " + std::string(fileName);
        file = popen(cmd.c_str(), "r");
        if (!file) {
          THROW_SG_ERROR("sg::RichtmyerMeshkov: could not open popen '"
                         + cmd + "'");
        }
#else
        THROW_SG_ERROR("sg::RichtmyerMeshkov: gzipped RM bob's"
                       " aren't supported on Windows!");
#endif
      } else {
#ifdef _WIN32
        if (_snprintf_s(bobName, 16, 16, "d_%04d_%04li", timeStep, blockID) > 15) {
#else
        if (std::snprintf(bobName, 16, "d_%04d_%04li", timeStep, blockID) > 15) {
#endif
          THROW_SG_ERROR("sg::RichtmyerMeshkov: Invalid timestep or blockID!");
        }
        const FileName fileName = fullDirName + FileName(bobName);
        file = fopen(fileName.c_str(), "rb");
        if (!file) {
          THROW_SG_ERROR("sg::RichtmyerMeshkov: could not open file '"
                         + std::string(fileName) + "'");
        }
      }

      if (b.size() < LoaderState::BLOCK_SIZE) {
        b.resize(LoaderState::BLOCK_SIZE);
      }

      if (std::fread(b.data(), LoaderState::BLOCK_SIZE, 1, file) != 1) {
        if (useGZip) {
#ifndef _WIN32
          pclose(file);
#endif
        } else {
          fclose(file);
        }
        THROW_SG_ERROR("sg::RichtmyerMeshkov: failed to read data from bob "
                       + std::string(bobName));
      }

      if (useGZip) {
#ifndef _WIN32
        pclose(file);
#endif
      } else {
        fclose(file);
      }

      return blockID;
    }

    OSP_REGISTER_SG_NODE(RichtmyerMeshkov);

  } // ::ospray::sg
} // ::ospray
