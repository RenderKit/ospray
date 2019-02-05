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

#include "ToneMapperPixelOp.h"
#include "ToneMapperPixelOp_ispc.h"

using namespace ospcommon;

namespace ospray {

  ToneMapperPixelOp::ToneMapperPixelOp()
  {
    ispcEquivalent = ispc::ToneMapperPixelOp_create();
  }

  void ToneMapperPixelOp::commit()
  {
    PixelOp::commit();
      
    float exposure = getParam1f("exposure", 1.f);
    
    // Default parameters fitted to the ACES 1.0 grayscale curve (RRT.a1.0.3 + ODT.Academy.Rec709_100nits_dim.a1.0.3)
    // We included exposure adjustment to match 18% middle gray (ODT(RRT(0.18)) = 0.18)
    const float aces_contrast = 1.6773f;
    const float aces_shoulder = 0.9714f;
    const float aces_midIn    = 0.18f;
    const float aces_midOut   = 0.18f;
    const float aces_hdrMax   = 11.0785f;
    
    float a = max(getParam1f("contrast", aces_contrast), 0.0001f);
    float d = clamp(getParam1f("shoulder", aces_shoulder), 0.0001f, 1.f);
    float m = clamp(getParam1f("midIn", aces_midIn), 0.0001f, 1.f);
    float n = clamp(getParam1f("midOut", aces_midOut), 0.0001f, 1.f);
    float w = max(getParam1f("hdrMax", aces_hdrMax), 1.f);
    bool acesColor = getParam1i("acesColor", 1);
    
    // Solve b and c
    float b = -((powf(m, -a*d)*(-powf(m, a) + (n*(powf(m, a*d)*n*powf(w, a) - powf(m, a)*powf(w, a*d))) / (powf(m, a*d)*n - n*powf(w, a*d)))) / n);
    float c = max((powf(m, a*d)*n*powf(w, a) - powf(m, a)*powf(w, a*d)) / (powf(m, a*d)*n - n*powf(w, a*d)), 0.f); // avoid discontinuous curve by clamping to 0
    
    ispc::ToneMapperPixelOp_set(ispcEquivalent, exposure, a, b, c, d, acesColor);
  }

  std::string ToneMapperPixelOp::toString() const
  {
    return "ospray::ToneMapperPixelOp";
  }

  PixelOp::Instance* ToneMapperPixelOp::createInstance(FrameBuffer*, PixelOp::Instance* /*prev*/)
  {
    // FIXME: use prev too
    return new ToneMapperPixelOp::Instance(getIE());
  }

  ToneMapperPixelOp::Instance::Instance(void* ispcInstance)
    : ispcInstance(ispcInstance)
  {
  }

  void ToneMapperPixelOp::Instance::postAccum(Tile& tile)
  {
    ToneMapperPixelOp_apply(ispcInstance, (ispc::Tile&)tile);
  }

  std::string ToneMapperPixelOp::Instance::toString() const
  {
    return "ospray::ToneMapperPixelOp::Instance";
  }

  OSP_REGISTER_PIXEL_OP(ToneMapperPixelOp, tonemapper);

} // ::ospray

