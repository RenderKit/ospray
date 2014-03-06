#pragma once

#include "hairgeom.h"
// ospray stuff
#include "geometry/geometry.h"

#define QUAD_NODE_AOS 1

namespace ospray {
  namespace hair {
    struct range2uc {
      uint8 lo;
      uint8 hi;
    };
    struct box4uc {
      range2uc x, y, z;
    };
      
    struct Hairlet {
      box3f bounds;
      // this is what is stored in the leaves
      struct Fragment {
        uint16 eol:1;     //! end of list indicator
        uint16 hairID:15; //!< 16 bits to reference to a hair itself
      };
      // a (compressed) quad-node of a quad-BVH
      struct QuadNode {
#if QUAD_NODE_AOS
        uint8 lo_x[4];
        uint8 lo_y[4];
        uint8 lo_z[4];
        uint16 child[4];
        uint8 hi_x[4];
        uint8 hi_y[4];
        uint8 hi_z[4];

        void set(int dim, int i, uint lo, uint hi)
        {
          if (dim == 0) { lo_x[i] = lo; hi_x[i] = hi; }
          if (dim == 1) { lo_y[i] = lo; hi_y[i] = hi; }
          if (dim == 2) { lo_z[i] = lo; hi_z[i] = hi; }
        }
        void clear() { 
          for (int i=0;i<4;i++) lo_x[i] = lo_y[i] = lo_z[i] = 255;
          for (int i=0;i<4;i++) hi_x[i] = hi_y[i] = hi_z[i] = 0;
          for (int i=0;i<4;i++) child[i] = (uint16)-1;
        }
#else
        box3uc  bounds[4]; // 4x3x2=24b;
        uint16   child[4];  // 4x2=8b
        void clear() { 
          for (int i=0;i<4;i++) bounds[i] = embree::empty;
          for (int i=0;i<4;i++) child[i] = (uint16)-1;
        }
#endif
      };
      std::vector<QuadNode> node;
      std::vector<Fragment> leaf;

      HairGeom *hg;

      float maxRadius;
      std::vector<int> segStart;

      void save(FILE *file);
      static Hairlet *load(HairGeom *hg, FILE *file);
      Hairlet(HairGeom *hg, std::vector<int> &segStart);
      Hairlet(HairGeom *hg) : hg(hg) {}
      void build();
    };

    struct HairBVH : public ospray::Geometry {
      std::vector<Hairlet*> hairlet;
      HairGeom *hg;
      HairBVH(HairGeom *hg=NULL) : hg(hg) {};
      void build(FILE *file);
      void buildRec(std::vector<int> &segID);
      Hairlet *makeHairlet(std::vector<int> &segStart);
      void load(const std::string &fileName, uint32 maxHairletsToLoad=(uint32)-1);

      // inherited from Geometry:
      virtual std::string toString() const { return "ospray::HairBVH/*is a Geometry*/"; }
      virtual void finalize(Model *model);

    };
  }
}
