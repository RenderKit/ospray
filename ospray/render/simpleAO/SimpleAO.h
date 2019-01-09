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

#pragma once

/*! \defgroup ospray_render_ao16 Simple 16-sample Ambient Occlusion Renderer

  \ingroup ospray_supported_renderers

  \brief Implements a simple renderer that shoots 16 rays (generated
  using a hard-coded set of random numbers) to compute a trivially
  simple and coarse ambient occlusion effect.

  This renderer is intentionally
  simple, and not very sophisticated (e.g., it does not do any
  post-filtering to reduce noise, nor even try and consider
  transparency, material type, etc.

*/

// ospray
#include "render/Renderer.h"

namespace ospray {

  /*! \brief Simple 16-sample Ambient Occlusion Renderer

    \detailed This renderer uses a set of 16 precomputed AO directions
    to shoot shadow rays; for accumulation these 16 directions are
    (semi-)randomly rotated to give different directions every frame
    (this is far from optimal regarding how well the samples are
    distributed, but seems OK for accumulation).
   */
  struct SimpleAO : public Renderer
  {
    SimpleAO(int defaultNumSamples = 1);
    virtual ~SimpleAO() override = default;
    virtual std::string toString() const override;
    virtual void commit() override;

  private:

    int numSamples{1};
  };

} // ::ospray
