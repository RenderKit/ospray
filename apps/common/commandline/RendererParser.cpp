// ======================================================================== //
// Copyright 2016 Intel Corporation                                         //
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

#include "RendererParser.h"

bool DefaultRendererParser::parse(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--renderer" || arg == "-r") {
      assert(i+1 < ac);
      m_rendererType = av[++i];
    } else if (arg == "--spp" || arg == "-spp") {
      m_spp = atoi(av[++i]);
    } else if (arg == "--max-depth") {
      maxDepth = atoi(av[++i]);
    }
  }

  finalize();

  return true;
}

ospray::cpp::Renderer DefaultRendererParser::renderer()
{
  return m_renderer;
}

void DefaultRendererParser::finalize()
{
  if (m_rendererType.empty())
    m_rendererType = "scivis";

  m_renderer = ospray::cpp::Renderer(m_rendererType.c_str());

  // Set renderer defaults (if not using 'aoX' renderers)
  if (m_rendererType[0] != 'a' && m_rendererType[1] != 'o')
  {
    m_renderer.set("aoSamples", 1);
    m_renderer.set("shadowsEnabled", 1);
  }

  m_renderer.set("spp", m_spp);
  m_renderer.set("maxDepth", maxDepth);

  m_renderer.commit();
}
