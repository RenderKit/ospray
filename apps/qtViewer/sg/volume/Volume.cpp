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

#include "Volume.h"
#include "sg/common/World.h"
#include "sg/common/Integrator.h"

namespace ospray {
  namespace sg {

    // =======================================================
    // base volume class
    // =======================================================

    bool Volume::useDataDistributedVolume = false;

    /*! \brief returns a std::string with the c++ name of this class */
    std::string Volume::toString() const
    { return "ospray::sg::Volume"; }

    void Volume::serialize(sg::Serialization::State &state)
    {
      Node::serialize(state);
      if (transferFunction) 
        transferFunction->serialize(state);
    }
    
    // =======================================================
    // structured volume class
    // =======================================================

    //! constructor
    StructuredVolume::StructuredVolume()
      : dimensions(-1), voxelType("<undefined>"), volume(NULL), mappedPointer(NULL)
    {}

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolume::toString() const
    { return "ospray::sg::StructuredVolume"; }
    
    //! return bounding box of all primitives
    box3f StructuredVolume::getBounds()
    { return box3f(vec3f(0.f),vec3f(getDimensions())); }

      //! \brief Initialize this node's value from given XML node 
    void StructuredVolume::setFromXML(const xml::Node *const node, 
                                      const unsigned char *binBasePtr)
    {
      Assert2(binBasePtr,
              "mapped binary file is NULL, in XML node that "
              "needs mapped binary data (sg::StructuredVolume)");
      voxelType = node->getProp("voxelType");
      if (node->hasProp("ofs"))
        mappedPointer = binBasePtr + node->getPropl("ofs");
      dimensions = parseVec3i(node->getProp("dimensions"));

      if (voxelType != "float" && voxelType != "uint8") 
        throw std::runtime_error("unknown StructuredVolume.voxelType (currently only supporting 'float' and 'uint8')");

      if (!transferFunction) 
        setTransferFunction(new TransferFunction);

      std::cout << "#osp:sg: created StructuredVolume from XML file, dimensions = " 
                << getDimensions() << std::endl;
    }
    
    /*! \brief 'render' the object to ospray */
    void StructuredVolume::render(RenderContext &ctx)
    {
      if (volume) return;

      if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0)
        throw std::runtime_error("StructuredVolume::render(): invalid volume dimensions");
      
      volume = ospNewVolume(useDataDistributedVolume 
                            ? "data_distributed_volume"
                            : "block_bricked_volume");
      if (!volume)
        THROW_SG_ERROR("could not allocate volume");

      ospSetString(volume,"voxelType",voxelType.c_str());
      ospSetVec3i(volume,"dimensions",(const osp::vec3i&)dimensions);
      size_t nPerSlice = (size_t)dimensions.x*(size_t)dimensions.y;
      assert(mappedPointer != NULL);

      for (int z=0;z<dimensions.z;z++) {
        float *slice = (float*)(((unsigned char *)mappedPointer)+z*nPerSlice*sizeof(float));
        vec3i region_lo(0,0,z), region_sz(dimensions.x,dimensions.y,1);
        ospSetRegion(volume,slice,
                     (const osp::vec3i&)region_lo,
                     (const osp::vec3i&)region_sz);
      }

      transferFunction->render(ctx);

      ospSetObject(volume,"transferFunction",transferFunction->getOSPHandle());
      ospCommit(volume);
      ospAddVolume(ctx.world->ospModel,volume);
    }

    OSP_REGISTER_SG_NODE(StructuredVolume);


    // =======================================================
    // structured volume that is stored in a separate file (ie, a file
    // other than the ospbin file)
    // =======================================================

    //! constructor
    StructuredVolumeFromFile::StructuredVolumeFromFile()
      : dimensions(-1), fileName(""), voxelType("<undefined>"), volume(NULL)
    {}

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolumeFromFile::toString() const
    { return "ospray::sg::StructuredVolumeFromFile"; }
    
    //! return bounding box of all primitives
    box3f StructuredVolumeFromFile::getBounds()
    { return box3f(vec3f(0.f),vec3f(getDimensions())); }

      //! \brief Initialize this node's value from given XML node 
    void StructuredVolumeFromFile::setFromXML(const xml::Node *const node, 
                                              const unsigned char *binBasePtr)
    {
      voxelType = node->getProp("voxelType");
      if (voxelType == "uint8") voxelType = "uchar";
      dimensions = parseVec3i(node->getProp("dimensions"));
      fileName = node->getProp("fileName");
      if (fileName == "") throw std::runtime_error("sg::StructuredVolumeFromFile: no 'fileName' specified");

      fileNameOfCorrespondingXmlDoc = node->doc->fileName;
      
      if (voxelType != "float" && voxelType != "uchar") 
        throw std::runtime_error("unknown StructuredVolume.voxelType (currently only supporting 'float')");
      
      if (!transferFunction) 
        setTransferFunction(new TransferFunction);
      
      std::cout << "#osp:sg: created StructuredVolume from XML file, dimensions = " 
                << getDimensions() << std::endl;
    }
    
    /*! \brief 'render' the object to ospray */
    void StructuredVolumeFromFile::render(RenderContext &ctx)
    {
      if (volume) return;
      
      if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0)
        throw std::runtime_error("StructuredVolume::render(): invalid volume dimensions");

      bool useBlockBricked = 1;

      if (useDataDistributedVolume) 
        volume = ospNewVolume("data_distributed_volume");
      else
        volume = ospNewVolume(useBlockBricked ? "block_bricked_volume" : "shared_structured_volume");
      if (!volume)
        THROW_SG_ERROR("could not allocate volume");
      
      PING; PRINT(voxelType);
      ospSetString(volume,"voxelType",voxelType.c_str());
      PING; PRINT(dimensions);
      ospSetVec3i(volume,"dimensions",(const osp::vec3i&)dimensions);
      PING;
      
      FileName realFileName = fileNameOfCorrespondingXmlDoc.path()+fileName;
      FILE *file = fopen(realFileName.c_str(),"rb");
      float minValue = +std::numeric_limits<float>::infinity();
      float maxValue = -std::numeric_limits<float>::infinity();
      if (!file) 
        throw std::runtime_error("StructuredVolumeFromFile::render(): could not open file '"
                                 +realFileName.str()+"' (expanded from xml file '"
                                 +fileNameOfCorrespondingXmlDoc.str()
                                 +"' and file name '"+fileName+"')");

      if (useBlockBricked || useDataDistributedVolume) {
        size_t nPerSlice = (size_t)dimensions.x * (size_t)dimensions.y;
        if (voxelType == "float") {
          float *slice = new float[nPerSlice];
          for (int z=0;z<dimensions.z;z++) {
            size_t nRead = fread(slice,sizeof(float),nPerSlice,file);
            if (nRead != nPerSlice)
              throw std::runtime_error("StructuredVolume::render(): read incomplete slice data ... partial file or wrong format!?");
            const vec3i region_lo(0,0,z),region_sz(dimensions.x,dimensions.y,1);
            ospSetRegion(volume,slice,(const osp::vec3i&)region_lo,(const osp::vec3i&)region_sz);
          }
          delete[] slice;
        } else {
          uint8_t *slice = new uint8_t[nPerSlice];
          for (int z=0;z<dimensions.z;z++) {
            size_t nRead = fread(slice,sizeof(uint8_t),nPerSlice,file);
            if (nRead != nPerSlice)
              throw std::runtime_error("StructuredVolume::render(): read incomplete slice data ... partial file or wrong format!?");
            const vec3i region_lo(0,0,z), region_sz(dimensions.x,dimensions.y,1);
            ospSetRegion(volume,slice,
                         (const osp::vec3i&)region_lo,
                         (const osp::vec3i&)region_sz);
          }
          delete[] slice;
        }
      } else {
        size_t nVoxels = (size_t)dimensions.x * (size_t)dimensions.y * (size_t)dimensions.z;
        float *voxels = new float[nVoxels];
        size_t nRead = fread(voxels,sizeof(float),nVoxels,file);
        if (nRead != nVoxels)
          THROW_SG_ERROR("read incomplete data (truncated file or wrong format?!)");
        OSPData data = ospNewData(nVoxels,OSP_FLOAT,voxels,OSP_DATA_SHARED_BUFFER);
        ospSetData(volume,"voxelData",data);
      }
      fclose(file);

      transferFunction->render(ctx);

      ospSetObject(volume,"transferFunction",transferFunction->getOSPHandle());
      ospCommit(volume);
      ospAddVolume(ctx.world->ospModel,volume);
    }

    OSP_REGISTER_SG_NODE(StructuredVolumeFromFile);





    // =======================================================
    // stacked slices volume class
    // =======================================================

    //! constructor
    StackedRawSlices::StackedRawSlices()
      : dimensions(-1), baseName(""), voxelType("uint8_t"), volume(NULL)
    {}

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StackedRawSlices::toString() const
    { return "ospray::sg::StackedRawSlices"; }
    
    //! return bounding box of all primitives
    box3f StackedRawSlices::getBounds()
    { return box3f(vec3f(0.f),vec3f(getDimensions())); }

      //! \brief Initialize this node's value from given XML node 
    void StackedRawSlices::setFromXML(const xml::Node *const node, const unsigned char *binBasePtr)
    {
      voxelType = node->getProp("voxelType");
      sliceResolution = parseVec2i(node->getProp("sliceResolution"));
      baseName = node->getProp("baseName");
      firstSliceID = node->getPropl("firstSliceID");
      numSlices = node->getPropl("numSlices");
      if (voxelType != "uint8_t") 
        throw std::runtime_error("unknown StackedRawSlices.voxelType (currently only supporting 'uint8_t')");
          
      if (!transferFunction) 
        setTransferFunction(new TransferFunction);
    }
    
    /*! \brief 'render' the object to ospray */
    void StackedRawSlices::render(RenderContext &ctx)
    {
      if (volume) return;

      dimensions.x = sliceResolution.x;
      dimensions.y = sliceResolution.y;
      dimensions.z = numSlices;

      if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0)
        throw std::runtime_error("StackedRawSlices::render(): invalid volume dimensions");
      
      volume = ospNewVolume("block_bricked_volume");
      if (!volume)
        THROW_SG_ERROR("could not allocate volume");

      ospSetString(volume,"voxelType",voxelType.c_str());
      ospSetVec3i(volume,"dimensions",(const osp::vec3i&)dimensions);
      size_t nPerSlice = dimensions.x*dimensions.y;
      uint8_t *slice = new uint8_t[nPerSlice];
      for (int sliceID=0;sliceID<numSlices;sliceID++) {
        char *sliceName = (char*)alloca(strlen(baseName.c_str()) + 20);
        sprintf(sliceName, baseName.c_str(), firstSliceID + sliceID);
        PRINT(sliceName);
        FILE *file = fopen(sliceName,"rb");
        if (!file) 
          throw std::runtime_error("StackedRawSlices::render(): could not open file '"
                                   +std::string(sliceName)+"'");
        size_t nRead = fread(slice,sizeof(float),nPerSlice,file);
        if (nRead != nPerSlice)
          throw std::runtime_error("StackedRawSlices::render(): read incomplete slice data ... partial file or wrong format!?");
        const vec3i region_lo(0,0,sliceID), region_sz(dimensions.x,dimensions.y,1);
        ospSetRegion(volume,slice,
                     (const osp::vec3i&)region_lo,
                     (const osp::vec3i&)region_sz);
        fclose(file);
      }
      delete[] slice;

      transferFunction->render(ctx);

      ospSetObject(volume,"transferFunction",transferFunction->getOSPHandle());
      ospCommit(volume);
      ospAddVolume(ctx.world->ospModel,volume);
    }

    OSP_REGISTER_SG_NODE(StackedRawSlices);

  } // ::ospray::sg
} // ::ospray
