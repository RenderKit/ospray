#include "objmaterial.h"
#include "objmaterial_ispc.h"

namespace ospray {
  namespace obj {

    //extern "C" void ispc_OBJMaterial_create(void *cppE);
    //    extern "C" void ispc_OBJMaterial_set(

    //! \brief commit the material's parameters
    void OBJMaterial::commit()
    {
      if (ispcEquivalent == NULL)
        ispcEquivalent = ispc::OBJMaterial_create(this);

      Kd = this->getParam3f("Kd",Kd);
      Ks = this->getParam3f("Ks",Ks);

      Ns = this->getParam1f("Ns",Ns);
      d  = this->getParam1f("d", d);

      ispc::OBJMaterial_set(getIE(),
                            (ispc::vec3f&)Kd,
                            (ispc::vec3f&)Ks,
                            Ns,d);
    }

    OBJMaterial::~OBJMaterial()
    {
      if (getIE() != NULL)
        ispc::OBJMaterial_destroy(getIE());
    }
    
    OSP_REGISTER_MATERIAL(OBJMaterial,OBJMaterial);
  }
}
