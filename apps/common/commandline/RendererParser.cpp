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

  m_renderer.commit();
}
