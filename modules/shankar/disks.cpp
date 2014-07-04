#undef NDEGBUG

// ospray
#include "disks.h"
#include "common/data.h"
#include "common/model.h"
// ispc-generated files
#include "disks_ispc.h"

namespace ospray {

  Disks::Disks()
  {
    this->ispcEquivalent = ispc::Disks_create(this);
  }

  void Disks::finalize(Model *model) 
  {
    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerDisk    = getParam1i("bytes_per_disk",4*sizeof(float));
    offset_center     = getParam1i("offset_center",0);
    offset_normal     = getParam1i("offset_normal",3*sizeof(float));
    offset_radius     = getParam1i("offset_radius",6*sizeof(float));
    offset_materialID = getParam1i("offset_materialID",-1);
    data              = getParamData("disks",NULL);
    
    if (data == NULL) 
      throw std::runtime_error("#ospray:geometry/disks: no 'disks' data specified");
    numDisks = data->numBytes / bytesPerDisk;
    std::cout << "#osp: creating 'disks' geometry, #disks = " << numDisks << std::endl;
    
    ispc::DisksGeometry_set(getIE(),model->getIE(),
                            data->data,numDisks,bytesPerDisk,
                            radius,materialID,
                            offset_center,offset_normal,
                            offset_radius,offset_materialID);
  }


  OSP_REGISTER_GEOMETRY(Disks,disks);
}
