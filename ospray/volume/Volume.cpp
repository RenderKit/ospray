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

// ospray
#include "common/Library.h"
#include "volume/Volume.h"
#include "Volume_ispc.h"
#include "transferFunction/TransferFunction.h"
#include "common/Data.h"
// stl
#include <map>

namespace ospray {

  Volume::~Volume()
  {
  }

  bool Volume::isDataDistributed() const
  {
    return false;
  }

  std::string Volume::toString() const
  {
    return("ospray::Volume");
  }

  Volume *Volume::createInstance(const std::string &type)
  {
    // Function pointer type for creating a concrete instance of a subtype of
    // this class.
    typedef Volume *(*creationFunctionPointer)();

    // Function pointers corresponding to each subtype.
    static std::map<std::string, creationFunctionPointer> symbolRegistry;

    // Find the creation function for the subtype if not already known.
    if (symbolRegistry.count(type) == 0) {

      // Construct the name of the creation function to look for.
      std::string creationFunctionName = "ospray_create_volume_" + type;

      // Look for the named function.
      symbolRegistry[type] =
          (creationFunctionPointer)getSymbol(creationFunctionName);

      // The named function may not be found if the requested subtype is not
      // known.
      if (!symbolRegistry[type] && ospray::logLevel >= 1) {
        std::cerr << "  ospray::Volume  WARNING: unrecognized subtype '" + type
                  << "'." << std::endl;
      }
    }

    // Create a concrete instance of the requested subtype.
    Volume *volume = (symbolRegistry[type]) ? (*symbolRegistry[type])() : NULL;

    // Denote the subclass type in the ManagedObject base class.
    if (volume) volume->managedObjectType = OSP_VOLUME;

    return volume;
  }

  void Volume::computeSamples(float **results,
                              const vec3f *worldCoordinates,
                              const size_t &count)
  {
    // The ISPC volume container must exist at this point.
    assert(ispcEquivalent != NULL);

    // Allocate memory for returned volume samples
    *results = (float *)malloc(count * sizeof(float));
    exitOnCondition(*results == NULL, "error allocating memory");

    // Allocate memory for ISPC-computed volume samples using Embree's new to
    // enforce alignment
    float *ispcResults = new float[count];
    exitOnCondition(ispcResults == NULL, "error allocating memory");

    // Compute the sample values.
    ispc::Volume_computeSamples(ispcEquivalent,
                                &ispcResults,
                                (const ispc::vec3f *)worldCoordinates,
                                count);

    // Copy samples and free ISPC results memory
    memcpy(*results, ispcResults, count * sizeof(float));
    delete[] ispcResults;
  }

  void Volume::finish()
  {
    // The ISPC volume container must exist at this point.
    assert(ispcEquivalent != NULL);

    // Make the volume bounding box visible to the application.
    ispc::box3f boundingBox;
    ispc::Volume_getBoundingBox(&boundingBox,ispcEquivalent);
    set("boundingBoxMin",
        vec3f(boundingBox.lower.x, boundingBox.lower.y, boundingBox.lower.z));
    set("boundingBoxMax",
        vec3f(boundingBox.upper.x, boundingBox.upper.y, boundingBox.upper.z));
  }

  void Volume::updateEditableParameters()
  {
    // Set the gradient shading flag for the renderer.
    ispc::Volume_setGradientShadingEnabled(ispcEquivalent,
                                           getParam1i("gradientShadingEnabled",
                                                      0));

    // Set the recommended sampling rate for ray casting based renderers.
    ispc::Volume_setSamplingRate(ispcEquivalent,
                                 getParam1f("samplingRate", 1.0f));

    // Set the transfer function.
    TransferFunction *transferFunction =
        (TransferFunction *) getParamObject("transferFunction", NULL);
    exitOnCondition(transferFunction == NULL, "no transfer function specified");
    ispc::Volume_setTransferFunction(ispcEquivalent, transferFunction->getIE());

    // Set the volume clipping box (empty by default for no clipping).
    box3f volumeClippingBox = box3f(getParam3f("volumeClippingBoxLower",
                                               vec3f(0.f)),
                                    getParam3f("volumeClippingBoxUpper",
                                               vec3f(0.f)));
    ispc::Volume_setVolumeClippingBox(ispcEquivalent,
                                      (const ispc::box3f &)volumeClippingBox);
  }

} // ::ospray

