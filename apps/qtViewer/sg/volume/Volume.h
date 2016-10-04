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

#pragma once

// sg
#include "sg/common/TransferFunction.h"

namespace ospray {
  namespace sg {

    /*! a geometry node - the generic geometry node */
    struct Volume : public sg::Node {
      Volume() {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const;

      //! return bounding box of all primitives
      virtual box3f getBounds() = 0;

      //! serialize into given serialization state 
      virtual void serialize(sg::Serialization::State &state);

      static bool useDataDistributedVolume;

      SG_NODE_DECLARE_MEMBER(Ref<TransferFunction>,transferFunction,TransferFunction);    
    };

    /*! a plain old structured volume */
    struct StructuredVolume : public Volume {
      //! constructor
      StructuredVolume();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const;

      //! return bounding box of all primitives
      virtual box3f getBounds();

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, const unsigned char *binBasePtr);

      /*! \brief 'render' the object to ospray */
      virtual void render(RenderContext &ctx);

      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions)
      SG_NODE_DECLARE_MEMBER(std::string,voxelType,ScalarType)

      const unsigned char *mappedPointer;

      //! ospray volume object handle
      OSPVolume volume;
    };

    /*! a plain old structured volume */
    struct StructuredVolumeFromFile : public Volume {
      //! constructor
      StructuredVolumeFromFile();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const;

      //! return bounding box of all primitives
      virtual box3f getBounds();

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, const unsigned char *binBasePtr);

      /*! \brief 'render' the object to ospray */
      virtual void render(RenderContext &ctx);

      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions);    
      SG_NODE_DECLARE_MEMBER(std::string,fileName,FileName);    
      SG_NODE_DECLARE_MEMBER(std::string,voxelType,ScalarType);    

      //! \brief file name of the xml doc when the node was loaded/parsed from xml 
      /*! \detailed we need this to properly resolve relative file names */
      FileName fileNameOfCorrespondingXmlDoc;

      //! ospray volume object handle
      OSPVolume volume;
    };

    /*! a structured volume whose input comes from a set of stacked RAW files */
    struct StackedRawSlices : public Volume {

      StackedRawSlices();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const;

      //! return bounding box of all primitives
      virtual box3f getBounds();

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, const unsigned char *binBasePtr);

      /*! \brief 'render' the object to ospray */
      virtual void render(RenderContext &ctx);

      /*! resolution (X x Y) of each slice */
      SG_NODE_DECLARE_MEMBER(vec2i,sliceResolution,SliceResolution);    
      /*! base path name for the slices, in "printf format" (e.g., "/mydir/slice%04i.raw") */
      SG_NODE_DECLARE_MEMBER(std::string,baseName,BaseName);    
      SG_NODE_DECLARE_MEMBER(int32_t,firstSliceID,FirstSliceID);    
      SG_NODE_DECLARE_MEMBER(int32_t,numSlices,numSlices);    
      SG_NODE_DECLARE_MEMBER(std::string,voxelType,ScalarType);    

      //! actual dimensions after the data is loaded in - to be computed from sliceResolutiona nd numSlices
      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions);    

      //! ospray volume object handle
      OSPVolume volume;

    };
    
  } // ::ospray::sg
} // ::ospray


