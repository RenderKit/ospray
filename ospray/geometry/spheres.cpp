#undef NDEGBUG

// ospray
#include "spheres.h"
#include "common/data.h"
#include "common/model.h"
// ispc-generated files
#include "spheres_ispc.h"

namespace ospray {

  Spheres::Spheres()
  {
    this->ispcEquivalent = ispc::Spheres_create(this);
  }

  void Spheres::finalize(Model *model) 
  {
    PING;
    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerSphere    = getParam1i("bytes_per_sphere",4*sizeof(float));
    offset_center     = getParam1i("offset_center",0);
    offset_radius     = getParam1i("offset_radius",-1); //3*sizeof(float));
    offset_materialID = getParam1i("offset_materialID",-1);
    data              = getParamData("spheres",NULL);
    PING;
    
    if (data == NULL) 
      throw std::runtime_error("#ospray:geometry/spheres: no 'spheres' data specified");
    numSpheres = data->numBytes / bytesPerSphere;
    std::cout << "#osp: creating 'spheres' geometry, #spheres = " << numSpheres << std::endl;
    
    ispc::SpheresGeometry_set(getIE(),model->getIE(),
                              data->data,numSpheres,bytesPerSphere,
                              radius,materialID,
                              offset_center,offset_radius,offset_materialID);
  }


  OSP_REGISTER_GEOMETRY(Spheres,spheres);
}
