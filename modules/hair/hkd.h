#pragma once

#include "hairgeom.h"

namespace ospray {
  namespace hair {
    struct HairGeom;

    struct HairKD {
      struct Item {
        uint32 eol    :  1; // end of list indicator
        uint32 segID  : 31;
        // uint32 curveID   : 19;
        // uint32 hairID    :  4; // first vertex is hairID*4
      };
      struct Node {
        float  split;
        uint32 isLeaf: 1; 
        uint32 dim   : 2; 
        uint32 offset:29;
      };

      HairGeom        *hg;
      
      std::vector<Node> nodeVec;
      Node             *node;
      std::vector<Item> itemVec;
      Item             *item;

      Segment getSegment(long startVertex) const { return hg->getSegment(startVertex); }

      HairKD(HairGeom *h) : hg(h),item(NULL),node(NULL) {};
      void build();
    };
  }
}

