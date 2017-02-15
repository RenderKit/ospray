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

#pragma once

// sg
#include "sg/transferFunction/TransferFunction.h"

namespace ospray {
  namespace sg {

    /*! a geometry node - the generic geometry node */
    struct Volume : public sg::Renderable {
      Volume() : volume(nullptr) {
      };

      virtual void init() override
      {
        Renderable::init();
        add(createNode("transferFunction", "TransferFunction"));
        add(createNode("gradientShadingEnabled", "bool", true));
        add(createNode("preIntegration", "bool", true));
        add(createNode("singleShade", "bool", true));
        add(createNode("adaptiveSampling", "bool", true));
        add(createNode("adaptiveScalar", "float", 15.f));
        add(createNode("adaptiveBacktrack", "float", 0.03f));
        add(createNode("samplingRate", "float", 0.125f));
        add(createNode("adaptiveMaxSamplingRate", "float", 2.f));
        add(createNode("volumeClippingBoxLower", "vec3f", vec3f(0.f)));
        add(createNode("volumeClippingBoxUpper", "vec3f", vec3f(0.f)));
        add(createNode("specular", "vec3f", vec3f(0.3f)));
        add(createNode("gridOrigin", "vec3f", vec3f(0.0f)));
        add(createNode("gridSpacing", "vec3f", vec3f(0.002f)));
        
        transferFunction = std::dynamic_pointer_cast<TransferFunction>(children["transferFunction"].node);
      }

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override;

      //! return bounding box of all primitives
      virtual box3f getBounds() override = 0;

      //! serialize into given serialization state
      virtual void serialize(sg::Serialization::State &state) override;

      virtual void preRender(RenderContext &ctx) override;

      static bool useDataDistributedVolume;

      //! ospray volume object handle
      public:
        OSPVolume volume;

      protected:
        std::shared_ptr<TransferFunction> transferFunction;
    };

    /*! a plain old structured volume */
    struct StructuredVolume : public Volume {
      //! constructor
      StructuredVolume();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override;

      //! return bounding box of all primitives
      virtual box3f getBounds() override;

      //! \brief Initialize this node's value from given XML node
      virtual void setFromXML(const xml::Node &node,
                              const unsigned char *binBasePtr) override;

      /*! \brief 'render' the object to ospray */
      virtual void render(RenderContext &ctx);

      virtual void postCommit(RenderContext &ctx) override;

      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions)
      SG_NODE_DECLARE_MEMBER(std::string,voxelType,ScalarType)

      const unsigned char *mappedPointer;
    };

    /*! a plain old structured volume */
    struct StructuredVolumeFromFile : public Volume {
      //! constructor
      StructuredVolumeFromFile();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override;

      //! return bounding box of all primitives
      virtual box3f getBounds() override;

      //! \brief Initialize this node's value from given XML node
      virtual void setFromXML(const xml::Node &node,
                              const unsigned char *binBasePtr) override;

      /*! \brief 'render' the object to ospray */
      virtual void render(RenderContext &ctx) override;

      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;


      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions);
      SG_NODE_DECLARE_MEMBER(std::string,fileName,FileName);
      SG_NODE_DECLARE_MEMBER(std::string,voxelType,ScalarType);

    public:
      //! \brief file name of the xml doc when the node was loaded/parsed from xml
      /*! \detailed we need this to properly resolve relative file names */
      FileName fileNameOfCorrespondingXmlDoc;
    };

    /*! a structured volume whose input comes from a set of stacked RAW files */
    struct StackedRawSlices : public Volume {

      StackedRawSlices();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override;

      //! return bounding box of all primitives
      virtual box3f getBounds() override;

      //! \brief Initialize this node's value from given XML node
      virtual void setFromXML(const xml::Node &node,
                              const unsigned char *binBasePtr) override;

      /*! \brief 'render' the object to ospray */
      virtual void render(RenderContext &ctx) override;

      /*! resolution (X x Y) of each slice */
      SG_NODE_DECLARE_MEMBER(vec2i,sliceResolution,SliceResolution);
      /*! base path name for the slices, in "printf format" (e.g., "/mydir/slice%04i.raw") */
      SG_NODE_DECLARE_MEMBER(std::string,baseName,BaseName);
      SG_NODE_DECLARE_MEMBER(int32_t,firstSliceID,FirstSliceID);
      SG_NODE_DECLARE_MEMBER(int32_t,numSlices,numSlices);
      SG_NODE_DECLARE_MEMBER(std::string,voxelType,ScalarType);

      //! actual dimensions after the data is loaded in - to be computed from sliceResolutiona nd numSlices
      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions);
    };

  } // ::ospray::sg
} // ::ospray


