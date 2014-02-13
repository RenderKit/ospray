#include "ospray/mpi/mpicommon.h"

namespace ospray {
  namespace dtm {
    
    /*! the kind of triangle mesh that lives with the app */
    struct AppTriangleMesh {
      std::vector<vec4f> vertex;
      std::vector<vec4i> index;
    };

    struct DistributedTriangleMesh 
    {
      struct Component {
        /*! bounds of this component
        box3fa bounds;
        int    owner;

        /*! The actual triangle mesh; NULL if owner != me */
        AppTriangleMesh *mesh;
      };

      std::vector<DistributedTriangleMesh*> component;
      std::vector<DistributedTriangleMesh*> myComponent;

      /*! initialize from *MY* local components. will use MPI to
          exchange component data with other nodes */
      DistributedTriangleMesh(std::vector<DistributedTriangleMesh*> myComponent);
    };
  }
}
