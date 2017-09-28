// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "VisItSharedStructuredVolume.h"
//ospray
#include "VisItModuleCommon.h"
#include "VisItSharedStructuredVolume_ispc.h"
#include "StructuredVolume_ispc.h"
//ospray common
#include "common/Data.h"
#include "ospray/ospray.h"

namespace ospray {
  namespace visit {
    VisItSharedStructuredVolume::VisItSharedStructuredVolume()
    {
      if (::ospray::visit::CheckVerbose()) {
	std::cout << "#osp: creating VisIt shared structured volume" 
		  << std::endl;
      }
    }

    VisItSharedStructuredVolume::~VisItSharedStructuredVolume()
    {
      // No longer listen for changes to voxelData.
      if(voxelData) voxelData->unregisterListener(this);
    }

    std::string VisItSharedStructuredVolume::toString() const
    {
      return("ospray::VisItSharedStructuredVolume<" + voxelType + ">");
    }

    void VisItSharedStructuredVolume::finish()
    {
      // Make the voxel value range visible to the application.
      if (findParam("voxelRange") == NULL)
	set("voxelRange", StructuredVolume::voxelRange);
      else
	StructuredVolume::voxelRange = 
	  getParam2f("voxelRange", StructuredVolume::voxelRange);

      if (this->useGridAccelerator) {
	buildAccelerator();
      }

      // Volume finish actions.
      Volume::finish();
    }

    void VisItSharedStructuredVolume::commit()
    {
      // Create the equivalent ISPC volume container.
      if (ispcEquivalent == nullptr) createEquivalentISPC();
      // StructuredVolume commit actions.	    
      // In order to completely disable grid accelerator, we need an ugly hack
      bool trueFinished = StructuredVolume::finished;
      StructuredVolume::finished = true;
      StructuredVolume::commit();
      StructuredVolume::finished = trueFinished;
      // Complete volume initialization (only on first commit).
      if (!StructuredVolume::finished) {
	finish();
	finished = true;
      }
    }

    int VisItSharedStructuredVolume::setRegion(const void *source, 
					       const vec3i &index, 
					       const vec3i &count)
    {
      if (getIE() == nullptr)
	createEquivalentISPC();
      switch (getVoxelType()) {
      case OSP_UCHAR:
	ispc::VisItSharedStructuredVolume_setRegion_uint8
	  (getIE(),source, 
	   (const ispc::vec3i&)index,
	   (const ispc::vec3i&)count);
	break;
      case OSP_SHORT:
	ispc::VisItSharedStructuredVolume_setRegion_int16
	  (getIE(),source,
	   (const ispc::vec3i&)index,
	   (const ispc::vec3i&)count);
	break;
      case OSP_USHORT:
	ispc::VisItSharedStructuredVolume_setRegion_uint16
	  (getIE(),source,
	   (const ispc::vec3i&)index,
	   (const ispc::vec3i&)count);
	break;
      case OSP_FLOAT:
	ispc::VisItSharedStructuredVolume_setRegion_float
	  (getIE(),source,
	   (const ispc::vec3i&)index,
	   (const ispc::vec3i&)count);
	break;
      case OSP_DOUBLE:
	ispc::VisItSharedStructuredVolume_setRegion_double
	  (getIE(),source,
	   (const ispc::vec3i&)index,
	   (const ispc::vec3i&)count);
	break;
      default:
	throw std::runtime_error
	  ("VisItSharedStructuredVolume::setRegion() not "
	   "support for volumes of voxel type '"
	   + voxelType + "'");
      }
      return true;
    }

    void VisItSharedStructuredVolume::createEquivalentISPC()
    {
      // Check if use grid accelerator before building the volume
      useGridAccelerator = getParam1i("useGridAccelerator", 0);
      if (::ospray::visit::CheckVerbose()) {
	std::cout << "#osp: using grid accelerator = " << useGridAccelerator << std::endl;
      }

      // Get the voxel type.
      voxelType = getParamString("voxelType", "unspecified");
      const OSPDataType ospVoxelType = getVoxelType();
      exitOnCondition(ospVoxelType == OSP_UNKNOWN, "unrecognized voxel type");

      // Get the volume dimensions.
      vec3i dimensions = getParam3i("dimensions", vec3i(0));
      exitOnCondition(reduce_min(dimensions) <= 0,
		      "invalid volume dimensions");

      // Get the voxel data.
      voxelData = (Data *)getParamObject("voxelData", nullptr);
    
      if (voxelData) {
	warnOnCondition
	  (!(voxelData->flags & OSP_DATA_SHARED_BUFFER),
	   "The voxel data buffer was not created with the "
	   "OSP_DATA_SHARED_BUFFER flag; "
	   "Use another volume type (e.g. BlockBrickedVolume) for "
	   "better performance");
      }

      // The voxel count.
      size_t voxelCount = (size_t)dimensions.x *
	(size_t)dimensions.y *
	(size_t)dimensions.z;
      allocatedVoxelData = (voxelData == nullptr) ?
	malloc(voxelCount*sizeOf(ospVoxelType)) : nullptr;
    
      // Create an ISPC VisItSharedStructuredVolume object and assign
      // type-specific function pointers.
      ispcEquivalent
	= ispc::VisItSharedStructuredVolume_createInstance
	(this,(int)useGridAccelerator,ospVoxelType,
	 (const ispc::vec3i &)dimensions,
	 voxelData?voxelData->data:allocatedVoxelData);

      // Listen for changes to voxelData.
      if (voxelData)
	voxelData->registerListener(this);
    }

    void VisItSharedStructuredVolume::dependencyGotChanged
    (ManagedObject *object)
    {
      // Rebuild volume accelerator when voxelData is committed.
      if(object == voxelData && ispcEquivalent && useGridAccelerator) {
	StructuredVolume::buildAccelerator();
      }
    }

    // A volume type with XYZ storage order. The voxel data is provided by the
    // application via a shared data buffer.
    OSP_REGISTER_VOLUME(VisItSharedStructuredVolume, 
			visit_shared_structured_volume);

  };
} // ::ospray
