// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#undef NDEBUG

// ospray
#include "common/Data.h"
#include "common/Model.h"
// ispc-generated files
#include "AMRGeometry_ispc.h"
#include "AMRVolume.h"
#include "ospcommon/tasking/parallel_for.h"

namespace ospray {
  namespace amr {
    
    struct OSPRAY_SDK_INTERFACE AMRGeometry : public Geometry {
    
      AMRGeometry();
      ~AMRGeometry();

      //! \brief common function to help printf-debugging
      virtual std::string toString() const { return "ospray::AMRGeometry"; }
    
      /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
      virtual void finalize(Model *model);

      /*! the data array that contains the iso-values we are to look for */
      Ref<Data> isoValueData;

      /*! the amr volume we are building over */
      Ref<AMRVolume> amrVolume;

      /*! list of active leaves, for given list of iso values */
      std::vector<const AMRAccel::Leaf *> activeLeaf;
    };


    AMRGeometry::AMRGeometry()
    {
      this->ispcEquivalent = ispc::AMRGeometry_create(this);
    }
  
    AMRGeometry::~AMRGeometry()
    {
      /*! TODO: properly destroy this geometry */
    }

    bool isActive(const AMRAccel::Leaf &leaf, const float *isoValue, int numIsoValues)
    {
      for (int i=0;i<numIsoValues;i++)
        if (isoValue[i] >= leaf.valueRange.lower && isoValue[i] <= leaf.valueRange.upper)
          return true;
      return false;
    }
    
    /*! \brief integrates this geometry's primitives into the respective
      model's acceleration structure */
    void AMRGeometry::finalize(Model *model)
    {
      isoValueData = getParamData("isoValues");
      if (!isoValueData)
        throw std::runtime_error("finalized a amrgeometry, but no 'isoValueData' set!");
    
      const size_t numIsoValues = isoValueData->numItems;
      const float *const isoValue = (float *)isoValueData->data;
    
      amrVolume = dynamic_cast<AMRVolume *>(getParamObject("amrVolume"));
      if (!amrVolume)
        throw std::runtime_error("finalized a amrgeometry, but no 'amrVolume' set!");

      std::mutex writeMutex;
      int numJobs = divRoundUp(amrVolume->accel->leaf.size(),(size_t)100);
      this->activeLeaf.clear();
      ospcommon::tasking::parallel_for(numJobs,[&](int jobID){
          int begin = (jobID*amrVolume->accel->leaf.size())/numJobs;
          int end = ((jobID+1)*amrVolume->accel->leaf.size())/numJobs;
          std::vector<const AMRAccel::Leaf *> activeLeaf;
          for (int i=begin;i<end;i++)
            if (isActive(amrVolume->accel->leaf[i],isoValue,numIsoValues))
              activeLeaf.push_back(&amrVolume->accel->leaf[i]);
          std::lock_guard<std::mutex> lock(writeMutex);
          for (int i=0;i<activeLeaf.size();i++)
            this->activeLeaf.push_back(activeLeaf[i]);
        });
      ispc::AMRGeometry_set(getIE(),model->getIE(),amrVolume->getIE(),
                               &activeLeaf[0],activeLeaf.size(),
                               isoValue,numIsoValues);
    }

    OSP_REGISTER_GEOMETRY(AMRGeometry,AMRGeometry);
  } // ::ospray::amr
} // ::ospray

