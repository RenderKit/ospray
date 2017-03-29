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

    // =======================================================
    // base volume class
    // =======================================================

    bool Volume::useDataDistributedVolume = false;

    /*! \brief returns a std::string with the c++ name of this class */
    Volume::Volume()
    {
      createChildNode("transferFunction", "TransferFunction");
      createChildNode("gradientShadingEnabled", "bool", true);
      createChildNode("preIntegration", "bool", true);
      createChildNode("singleShade", "bool", true);
      createChildNode("voxelRange", "vec2f",
                      vec2f(std::numeric_limits<float>::infinity(),
                            -std::numeric_limits<float>::infinity()));
      createChildNode("adaptiveSampling", "bool", true);
      createChildNode("adaptiveScalar", "float", 15.f);
      createChildNode("adaptiveBacktrack", "float", 0.03f);
      createChildNode("samplingRate", "float", 0.125f);
      createChildNode("adaptiveMaxSamplingRate", "float", 2.f);
      createChildNode("volumeClippingBoxLower", "vec3f", vec3f(0.f));
      createChildNode("volumeClippingBoxUpper", "vec3f", vec3f(0.f));
      createChildNode("specular", "vec3f", vec3f(0.3f));
      createChildNode("gridOrigin", "vec3f", vec3f(0.0f));
      createChildNode("gridSpacing", "vec3f", vec3f(0.002f));
      createChildNode("isosurfaceEnabled", "bool", false);
      createChildNode("isosurface", "float",
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

    void Volume::preRender(RenderContext &ctx)
    {
      if (volume) {
        ospAddVolume(ctx.world->ospModel,volume);
        if (child("isosurfaceEnabled").valueAs<bool>() == true
            && isosurfacesGeometry)
          ospAddGeometry(ctx.world->ospModel, isosurfacesGeometry);
      }
    }

    // =======================================================
    // structured volume class
    // =======================================================

    //! constructor
    StructuredVolume::StructuredVolume()
      : dimensions(-1), voxelType("<undefined>"), mappedPointer(nullptr)
    {
    }

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolume::toString() const
    { return "ospray::sg::StructuredVolume"; }

    //! return bounding box of all primitives
    box3f StructuredVolume::bounds() const
    {
      return {vec3f(0.f),
            vec3f(getDimensions())*child("gridSpacing").valueAs<vec3f>()};
    }

    //! \brief Initialize this node's value from given XML node

    void StructuredVolume::setFromXML(const xml::Node &node,
                                      const unsigned char *binBasePtr)
    {
      Assert2(binBasePtr,
              "mapped binary file is nullptr, in XML node that "
              "needs mapped binary data (sg::StructuredVolume)");
      voxelType = node.getProp("voxelType");
      if (node.hasProp("ofs"))
        mappedPointer = binBasePtr + std::stoll(node.getProp("ofs","0"));
      dimensions = toVec3i(node.getProp("dimensions").c_str());

      if (voxelType == "uint8")
        voxelType = "uchar";
      if (voxelType != "float" &&
          voxelType != "uint8" &&
          voxelType != "uchar") {
        throw std::runtime_error("unknown StructuredVolume.voxelType (currently"
                                 " only supporting 'float' and 'uint8')");
      }

      std::cout << "#osp:sg: created StructuredVolume from XML file, "
                << "dimensions = " << getDimensions() << std::endl;
    }

    // TODO: why is this a copy-paste of render??
    void StructuredVolume::postCommit(RenderContext &ctx)
    {
      if (volume) return;

      if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0)
        throw std::runtime_error("StructuredVolume::render(): "
                                 "invalid volume dimensions");

      volume = ospNewVolume(useDataDistributedVolume
                            ? "data_distributed_volume"
                            : "block_bricked_volume");

      if (!volume) THROW_SG_ERROR("could not allocate volume");

      ospSetString(volume,"voxelType",voxelType.c_str());
      ospSetVec3i(volume,"dimensions",(const osp::vec3i&)dimensions);
      size_t nPerSlice = (size_t)dimensions.x*(size_t)dimensions.y;
      assert(mappedPointer != nullptr);

      for (int z=0;z<dimensions.z;z++) {
        float *slice =
          (float*)(((unsigned char *)mappedPointer)+z*nPerSlice*sizeof(float));
        vec3i region_lo(0,0,z), region_sz(dimensions.x,dimensions.y,1);
        ospSetRegion(volume,slice,
                     (const osp::vec3i&)region_lo,
                     (const osp::vec3i&)region_sz);
      }

      ospCommit(volume);
    }

    OSP_REGISTER_SG_NODE(StructuredVolume);

    // =======================================================
    // structured volume that is stored in a separate file (ie, a file
    // other than the ospbin file)
    // =======================================================

    //! constructor
    StructuredVolumeFromFile::StructuredVolumeFromFile()
      : dimensions(-1), fileName(""), voxelType("<undefined>")
    {}

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolumeFromFile::toString() const
    {
      return "ospray::sg::StructuredVolumeFromFile";
    }

    //! return bounding box of all primitives
    box3f StructuredVolumeFromFile::bounds() const
    {
      return {vec3f(0.f),
              vec3f(getDimensions())*child("gridSpacing").valueAs<vec3f>()};
    }

    //! \brief Initialize this node's value from given XML node
    void StructuredVolumeFromFile::setFromXML(const xml::Node &node,
                                              const unsigned char *binBasePtr)
    {
      voxelType = node.getProp("voxelType");
      if (voxelType == "uint8") voxelType = "uchar";
      dimensions = toVec3i(node.getProp("dimensions").c_str());
      fileName = node.getProp("fileName");

      if (fileName.empty()) {
        throw std::runtime_error("sg::StructuredVolumeFromFile: "
                                 "no 'fileName' specified");
      }

      fileNameOfCorrespondingXmlDoc = node.doc->fileName;

      if (voxelType != "float" && voxelType != "uchar") {
        throw std::runtime_error("unknown StructuredVolume.voxelType "
                                 "(currently support 'float' and 'uchar')");
      }

      std::cout << "#osp:sg: created StructuredVolume from XML file, "
                << "dimensions = " << getDimensions() << std::endl;
    }

    void StructuredVolumeFromFile::preCommit(RenderContext &ctx)
    {
      if (volume) {
        ospCommit(volume);
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

      bool useBlockBricked = 1;

      if (useDataDistributedVolume)
        volume = ospNewVolume("data_distributed_volume");
      else
        volume = ospNewVolume(useBlockBricked ? "block_bricked_volume" :
                                                "shared_structured_volume");

      if (!volume) THROW_SG_ERROR("could not allocate volume");

      isosurfacesGeometry = ospNewGeometry("isosurfaces");
      ospSetObject(isosurfacesGeometry, "volume", volume);

      setValue((OSPObject)volume);

      ospSetString(volume,"voxelType",voxelType.c_str());
      ospSetVec3i(volume,"dimensions",(const osp::vec3i&)dimensions);

      FileName realFileName = fileNameOfCorrespondingXmlDoc.path()+fileName;
      FILE *file = fopen(realFileName.c_str(),"rb");
      if (!file)
        throw std::runtime_error("StructuredVolumeFromFile::render(): could not open file '"
                                 +realFileName.str()+"' (expanded from xml file '"
                                 +fileNameOfCorrespondingXmlDoc.str()
                                 +"' and file name '"+fileName+"')");

      vec2f voxelRange(std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
      const OSPDataType ospVoxelType = getOSPDataTypeFor(voxelType);
      const size_t voxelSize = sizeOf(ospVoxelType);
      if (useBlockBricked || useDataDistributedVolume) {
        const size_t nPerSlice = (size_t)dimensions.x * (size_t)dimensions.y;
        std::vector<uint8_t> slice(nPerSlice * voxelSize, 0);

        for (size_t z = 0; z < dimensions.z; ++z) {
          if (fread(slice.data(), voxelSize, nPerSlice, file) != nPerSlice) {
            throw std::runtime_error("StructuredVolume::render(): read incomplete slice "
                "data ... partial file or wrong format!?");
          }
          const vec3i region_lo(0, 0, z);
          const vec3i region_sz(dimensions.x, dimensions.y, 1);
          extendVoxelRange(voxelRange, ospVoxelType, slice.data(), nPerSlice);
          ospSetRegion(volume, slice.data(), (const osp::vec3i&)region_lo, (const osp::vec3i&)region_sz);
        }
      } else {
        const size_t nVoxels = (size_t)dimensions.x * (size_t)dimensions.y * (size_t)dimensions.z;
        uint8_t *voxels = new uint8_t[nVoxels * voxelSize];
        if (fread(voxels, voxelSize, nVoxels, file) != nVoxels) {
          THROW_SG_ERROR("read incomplete data (truncated file or wrong format?!)");
        }
        extendVoxelRange(voxelRange, ospVoxelType, voxels, nVoxels);
        OSPData data = ospNewData(nVoxels, ospVoxelType, voxels, OSP_DATA_SHARED_BUFFER);
        ospSetData(volume,"voxelData",data);
      }
      fclose(file);

      child("voxelRange").setValue(voxelRange);
      child("isosurface").setMinMax(voxelRange.x, voxelRange.y);
      float iso = child("isosurface").valueAs<float>();
      if (iso < voxelRange.x || iso > voxelRange.y)
        child("isosurface").setValue((voxelRange.y-voxelRange.x)/2.f);
      child("transferFunction")["valueRange"].setValue(voxelRange);
      child("transferFunction").preCommit(ctx);

      ospSetObject(volume,
                   "transferFunction",
                   child("transferFunction").valueAs<OSPObject>());
      ospCommit(volume);
    }

    void StructuredVolumeFromFile::postCommit(RenderContext &ctx)
    {
      ospSetObject(volume,"transferFunction",
                   child("transferFunction").valueAs<OSPObject>());
      ospCommit(volume);
    }

    OSP_REGISTER_SG_NODE(StructuredVolumeFromFile);

    // =======================================================
    // stacked slices volume class
    // =======================================================

    StackedRawSlices::StackedRawSlices()
      : baseName(""), voxelType("uint8_t"), dimensions(-1)
    {}

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StackedRawSlices::toString() const
    {
      return "ospray::sg::StackedRawSlices";
    }

    //! return bounding box of all primitives
    box3f StackedRawSlices::bounds() const
    {
      return box3f(vec3f(0.f),vec3f(getDimensions()));
    }

    //! \brief Initialize this node's value from given XML node
    void StackedRawSlices::setFromXML(const xml::Node &node,
                                      const unsigned char *binBasePtr)
    {
      voxelType       = node.getProp("voxelType");
      sliceResolution = toVec2i(node.getProp("sliceResolution").c_str());
      baseName        = node.getProp("baseName");
      firstSliceID    = std::stoll(node.getProp("firstSliceID","0"));
      numSlices       = std::stoll(node.getProp("numSlices"));

      if (voxelType != "uint8_t") {
        throw std::runtime_error("unknown StackedRawSlices.voxelType "
                                 "(currently only supporting 'uint8_t')");
      }
    }

    OSP_REGISTER_SG_NODE(StackedRawSlices);

  } // ::ospray::sg
} // ::ospray
