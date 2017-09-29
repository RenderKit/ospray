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
#include "../common/Renderable.h"
#include "../transferFunction/TransferFunction.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Volume : public Renderable
    {
      Volume();

      virtual std::string toString() const override;

      virtual void serialize(sg::Serialization::State &state) override;

      virtual void postCommit(RenderContext &ctx) override;

      virtual void postRender(RenderContext &ctx) override;

      OSPGeometry isosurfacesGeometry{nullptr};
    };

    /*! a plain old structured volume */
    struct OSPSG_INTERFACE StructuredVolume : public Volume
    {
      std::string toString() const override;

      //! return bounding box of all primitives
      box3f bounds() const override;

      //! \brief Initialize this node's value from given XML node
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override;

      vec3i dimensions {-1};
      std::string voxelType = "<undefined>";

      const unsigned char *mappedPointer {nullptr};
    };

    /*! a plain old structured volume */
    struct OSPSG_INTERFACE StructuredVolumeFromFile : public StructuredVolume
    {
      std::string toString() const override;

      //! \brief Initialize this node's value from given XML node
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override;

      void preCommit(RenderContext &ctx) override;

      //! \brief file name of the xml doc when the node was loaded from xml
      /*! \detailed we need this to properly resolve relative file names */
      FileName fileNameOfCorrespondingXmlDoc;

      std::string fileName;
    };

    /*! a structured volume loaded from the Richtmyer-Meshkov .bob files */
    struct OSPSG_INTERFACE RichtmyerMeshkov : public StructuredVolume
    {
      RichtmyerMeshkov();

      std::string toString() const override;

      //! \brief Initialize this node's value from given XML node
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override;

      void preCommit(RenderContext &ctx) override;

      std::string dirName;
      int timeStep {-1};

      //! \brief file name of the xml doc when the node was loaded from xml
      /*! \detailed we need this to properly resolve relative directory names */
      FileName fileNameOfCorrespondingXmlDoc;

    private:

      //! \brief state for the loader threads to use, for picking which block to load
      struct LoaderState
      {
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
