#undef NDEBUG

// ospray
#include "Spheres.h"
#include "ospray/common/Data.h"
#include "ospray/common/Model.h"
// ispc-generated files
#include "Spheres_ispc.h"

namespace ospray {

  Spheres::Spheres()
  {
    this->ispcEquivalent = ispc::Spheres_create(this);
    _materialList = NULL;
  }

  void Spheres::finalize(Model *model) 
  {
    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerSphere    = getParam1i("bytes_per_sphere",4*sizeof(float));
    offset_center     = getParam1i("offset_center",0);
    offset_radius     = getParam1i("offset_radius",-1); //3*sizeof(float));
    offset_materialID = getParam1i("offset_materialID",-1);
    data              = getParamData("spheres",NULL);
    materialList      = getParamData("materialList",NULL);
    
    if (data.ptr == NULL) 
      throw std::runtime_error("#ospray:geometry/spheres: no 'spheres' data specified");
    numSpheres = data->numBytes / bytesPerSphere;
    std::cout << "#osp: creating 'spheres' geometry, #spheres = " << numSpheres << std::endl;
    
    if (numSpheres >= (1ULL << 30)) {
      throw std::runtime_error("#ospray::Spheres: too many spheres in this sphere geometry. Consider splitting this geometry in multiple geometries with fewer spheres (you can still put all those geometries into a single model, but you can't put that many spheres into a single geometry without causing address overflows)");
    }

    if (_materialList) {
      free(_materialList);
      _materialList = NULL;
    }

    if (materialList) {
      void **ispcMaterials = (void**) malloc(sizeof(void*) * materialList->numItems);
      for (int i=0;i<materialList->numItems;i++) {
        Material *m = ((Material**)materialList->data)[i];
        ispcMaterials[i] = m?m->getIE():NULL;
      }
      _materialList = (void*)ispcMaterials;
    }
    ispc::SpheresGeometry_set(getIE(),model->getIE(),
                              data->data,_materialList,
                              numSpheres,bytesPerSphere,
                              radius,materialID,
                              offset_center,offset_radius,offset_materialID);
  }

  OSP_REGISTER_GEOMETRY(Spheres,spheres);

} // ::ospray
