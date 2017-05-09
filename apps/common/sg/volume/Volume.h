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

    struct Volume : public sg::Renderable
    {
      Volume();

      virtual std::string toString() const override;

      //! return bounding box of all primitives
      virtual box3f bounds() const override = 0;

      //! serialize into given serialization state
      virtual void serialize(sg::Serialization::State &state) override;

      virtual void preRender(RenderContext &ctx) override;

      static bool useDataDistributedVolume;

      OSPVolume volume {nullptr};
      OSPGeometry isosurfacesGeometry{nullptr};
    };

    /*! a plain old structured volume */
    struct StructuredVolume : public Volume
    {
      StructuredVolume();

      std::string toString() const override;

      //! return bounding box of all primitives
      box3f bounds() const override;

      //! \brief Initialize this node's value from given XML node
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override;

      void postCommit(RenderContext &ctx) override;

      SG_NODE_DECLARE_MEMBER(vec3i, dimensions, Dimensions);
      SG_NODE_DECLARE_MEMBER(std::string, voxelType, ScalarType);

      const unsigned char *mappedPointer;
    };

    /*! a plain old structured volume */
    struct StructuredVolumeFromFile : public Volume
    {
      StructuredVolumeFromFile();

      std::string toString() const override;

      //! return bounding box of all primitives
      box3f bounds() const override;

      //! \brief Initialize this node's value from given XML node
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override;

      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;


      SG_NODE_DECLARE_MEMBER(vec3i, dimensions, Dimensions);
      SG_NODE_DECLARE_MEMBER(std::string, fileName, FileName);
      SG_NODE_DECLARE_MEMBER(std::string, voxelType, ScalarType);

    public:
      //! \brief file name of the xml doc when the node was loaded from xml
      /*! \detailed we need this to properly resolve relative file names */
      FileName fileNameOfCorrespondingXmlDoc;
    };

    /*! a structured volume whose input comes from a set of stacked RAW files */
    struct StackedRawSlices : public Volume
    {
      StackedRawSlices();

      std::string toString() const override;

      //! return bounding box of all primitives
      box3f bounds() const override;

      //! \brief Initialize this node's value from given XML node
      void setFromXML(const xml::Node &node,
                              const unsigned char *binBasePtr) override;

      /*! resolution (X x Y) of each slice */
      SG_NODE_DECLARE_MEMBER(vec2i, sliceResolution, SliceResolution);

      /*! base path name for the slices, in "printf format"
       *  (e.g., "/mydir/slice%04i.raw")
       */
      SG_NODE_DECLARE_MEMBER(std::string, baseName, BaseName);
      SG_NODE_DECLARE_MEMBER(int32_t, firstSliceID, FirstSliceID);
      SG_NODE_DECLARE_MEMBER(int32_t, numSlices, numSlices);
      SG_NODE_DECLARE_MEMBER(std::string, voxelType, ScalarType);

      //! actual dimensions after the data is loaded in - to be computed from
      //  sliceResolutiona nd numSlices
      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions);
    };

    /*! a structured volume loaded from the Richtmyer-Meshkov .bob files */
    struct RichtmyerMeshkov : public Volume
    {
      RichtmyerMeshkov();

      std::string toString() const override;

      //! return bounding box of all primitives
      box3f bounds() const override;

      //! \brief Initialize this node's value from given XML node
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override;

      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;

      SG_NODE_DECLARE_MEMBER(vec3i, dimensions, Dimensions);
      SG_NODE_DECLARE_MEMBER(std::string, dirName, DirName);
      SG_NODE_DECLARE_MEMBER(int, timeStep, TimeStep);
      // TODO WILL: I don't think we need this
      //SG_NODE_DECLARE_MEMBER(std::string, voxelType, ScalarType);

      //! \brief file name of the xml doc when the node was loaded from xml
      /*! \detailed we need this to properly resolve relative directory names */
      FileName fileNameOfCorrespondingXmlDoc;

    private:
      //! \brief state for the loader threads to use, for picking which block to load
      struct LoaderState {
        std::mutex mutex;
        std::atomic<size_t> nextBlockID;
        bool useGZip;
        FileName fullDirName;
        int timeStep;
        vec2f voxelRange;

        const static size_t BLOCK_SIZE;
        const static size_t NUM_BLOCKS;

        LoaderState(const FileName &fullDirName, const int timeStep);
        //! \brief Load the next RM block and return the id of the loaded block.
        //         If all blocks are loaded, returns a block ID >= NUM_BLOCKS
        size_t loadNextBlock(std::vector<uint8_t> &b);
      };

      //! \brief worker thread function for loading blocks of the RM data
      void loaderThread(LoaderState &state);
    };

  } // ::ospray::sg
} // ::ospray
