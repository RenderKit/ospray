#pragma once

#include "ospray/common/model.h"

namespace ospray {
  namespace shankar {
    struct RBFTree : public ospray::Geometry {
      RBFTree();

      //! \brief common function to help printf-debugging 
      virtual std::string toString() const { return "ospray::shankar::RBFTree"; }
      /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
      virtual void finalize(Model *model);
      
      struct RBF {
        vec3f pos;
        float coeff;
      };
      // all RBFs from all surfaces are in one data array; that makes things easier
      Ref<Data> rbfData;

      struct Node {
        int beginRBFs;
        int endRBFs;
        int leftNodeID;
        int rightNodeID;
        int materialID;

        // accel plane:
        vec3f center;
        vec3f normal;
        float lower;
        float upper;
      };
      // all nodes are in one
      Ref<Data> nodeData;

      static void load(const std::string &fileName,
                       OSPData &rbfPointData,
                       OSPData &nodeData,
                       box3f &bounds);
    };
  }
}
