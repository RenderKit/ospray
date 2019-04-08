// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "volume/Volume.h"
#include "Volume_ispc.h"
#include "common/Data.h"
#include "common/Util.h"
#include "transferFunction/TransferFunction.h"

namespace ospray {

  std::string Volume::toString() const
  {
    return "ospray::Volume";
  }

  Volume *Volume::createInstance(const std::string &type)
  {
    return createInstanceHelper<Volume, OSP_VOLUME>(type);
  }

  void Volume::commit()
  {
    // Set the transfer function.
    auto *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error("no transfer function specified on the volume!");

    // Set the volume clipping box (empty by default for no clipping).
    box3f volumeClippingBox =
        box3f(getParam3f("volumeClippingBoxLower", vec3f(0.f)),
              getParam3f("volumeClippingBoxUpper", vec3f(0.f)));

    // Set affine transformation
    AffineSpace3f xfm;
    xfm.l.vx              = getParam3f("xfm.l.vx", vec3f(1.f, 0.f, 0.f));
    xfm.l.vy              = getParam3f("xfm.l.vy", vec3f(0.f, 1.f, 0.f));
    xfm.l.vz              = getParam3f("xfm.l.vz", vec3f(0.f, 0.f, 1.f));
    xfm.p                 = getParam3f("xfm.p", vec3f(0.f, 0.f, 0.f));
    AffineSpace3f rcp_xfm = rcp(xfm);

    vec3f specular =
        getParam3f("specular", getParam3f("ks", getParam3f("Ks", vec3f(0.3f))));

    ispc::Volume_set(ispcEquivalent,
                     getParam1b("gradientShadingEnabled", false),
                     getParam1b("preIntegration", false),
                     getParam1b("singleShade", true),
                     getParam1b("adaptiveSampling", true),
                     getParam1f("adaptiveScalar", 15.0f),
                     getParam1f("adaptiveMaxSamplingRate", 2.0f),
                     getParam1f("adaptiveBacktrack", 0.03f),
                     getParam1f("samplingRate", 0.125f),
                     (const ispc::vec3f &)specular,
                     getParam1f("ns", getParam1f("Ns", 20.f)),
                     transferFunction->getIE(),
                     (const ispc::box3f &)volumeClippingBox,
                     (ispc::AffineSpace3f &)xfm,
                     (ispc::AffineSpace3f &)rcp_xfm);
  }

  void Volume::finish()
  {
    // The ISPC volume container must exist at this point.
    assert(ispcEquivalent != nullptr);

    // Make the volume bounding box visible to the application.
    ispc::box3f boundingBox;
    ispc::Volume_getBoundingBox(&boundingBox, ispcEquivalent);
    setParam("boundingBoxMin", boundingBox.lower);
    setParam("boundingBoxMax", boundingBox.upper);
  }

}  // namespace ospray
