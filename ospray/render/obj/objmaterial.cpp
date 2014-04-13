#include "objmaterial.h"

namespace ospray {
  namespace obj {

    //! \brief commit the material's parameters
    void OBJMaterial::commit()
    {
      Kd = this->getParam3f("Kd",Kd);
      Ks = this->getParam3f("Ks",Ks);

      Ns = this->getParam1f("Ns",Ns);
      d  = this->getParam1f("d", d);
    }
    
  }
}
