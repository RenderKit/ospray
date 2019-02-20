// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "AMRData.h"

namespace ospray {
  namespace amr {

    /*! a (kd tree based) acceleration structure over the amr
      data. the structure builds a kdtree over uniform areas
      ('uniform' maning that all cells in that area will be from the
      same chmobo block. for each such areas, we store a 'leaf' that
      contains all the different input blocks that overlap this
      area, the finest such block listed first */
    struct AMRAccel
    {
      /*! constructor that constructs the actual accel from the amr data */
      AMRAccel(const AMRData &input);
      /*! destructor that frees all allocated memory */
      ~AMRAccel();

      /*! precomputed values per level, so we can easily compute
          logicla coordinates, find any level's cell width, etc */
      struct Level
      {
        float cellWidth;
        float rcpCellWidth;
        float halfCellWidth;
        int   level;
      };

      /*! each leaf in the tree represents an area in which all cells
        stem from the same input block. note we not only store the
        finest (leaf) block, but also store pointers to all parent
        blocks that overlap this area */
      struct Leaf
      {
        Leaf() {}
        Leaf(const Leaf &o) : brickList(o.brickList), bounds(o.bounds) {}

        /*! list of bricks that overlap this leaf; sorted from finest
          to coarsest level */
        const AMRData::Brick **brickList;

        /*! bounding box of this leaf - note that the bricks will
          likely "stick out" of this bounding box, and the same
          brick may be listed in MULTIPLE leaves */
        box3f bounds;

        /*! value range within this leaf. WARNING: this will only get
            set AFTER all the bricks are being generated (we need all
            bricks before we can even compute the values in a brick */
        range1f valueRange;
      };

      /*! each node in the tree refers to either a pair of child
        nodes (using split plane pos, split plane dim, and offset in
        node[] array), or to a list of 'numItems' bricks (using
        dim=3, and offset pointing to the start of the list of
        numItems brick pointers in 'brickArray[]'. In case of a
        leaf, the 'num' bricks stored in this leaf are (in theory),
        exactly one brick per level, in sorted order */
      struct Node
      {
        inline bool isLeaf() const { return dim == 3; }
        // first dword
        uint32 ofs:30; // offset in node[] array (if inner), or brick ID (if leaf)
        uint32 dim:2;  // upper two bits: split dimension. '3' means 'leaf
        // second dword
        union
        {
          float pos;
          uint32 numItems;
        };
      };

      void buildLevelInfo();

      inline const Level &finestLevel() const { return level.back(); }

      //! list of levels
      std::vector<Level> level;
      //! list of inner nodes
      std::vector<Node> node;
      //! list of leaf nodes
      std::vector<Leaf> leaf;
      //! world bounds of domain
      box3f worldBounds;

    private:
      void makeLeaf(index_t nodeID,
                    const box3f &bounds,
                    const std::vector<const AMRData::Brick *> &brickIDs);
      void makeInner(index_t nodeID, int dim, float pos, int childID);
      void buildRec(int nodeID,const box3f &bounds,
                    std::vector<const AMRData::Brick *> &brick);

    };

  } // ::ospray::amr
} // ::ospray
