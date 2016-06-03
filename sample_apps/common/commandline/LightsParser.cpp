#include "LightsParser.h"

#include <ospray_cpp/Data.h>

#include <vector>

using namespace ospcommon;

DefaultLightsParser::DefaultLightsParser(ospray::cpp::Renderer renderer) :
  m_renderer(renderer),
  m_defaultDirLight_direction(.3, -1, -.2)
{
}

bool DefaultLightsParser::parse(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--sun-dir") {
      if (!strcmp(av[i+1],"none")) {
        m_defaultDirLight_direction = vec3f(0.f);
      } else {
        m_defaultDirLight_direction.x = atof(av[++i]);
        m_defaultDirLight_direction.y = atof(av[++i]);
        m_defaultDirLight_direction.z = atof(av[++i]);
      }
    }
  }

  finalize();

  return true;
}

void DefaultLightsParser::finalize()
{
  //TODO: Need to figure out where we're going to read lighting data from
  std::vector<OSPLight> lights;
  if (m_defaultDirLight_direction != vec3f(0.f)) {
    auto ospLight = m_renderer.newLight("DirectionalLight");
    if (ospLight.handle() == nullptr) {
      throw std::runtime_error("Failed to create a 'DirectionalLight'!");
    }
    ospLight.set("name", "sun");
    ospLight.set("color", 1.f, 1.f, 1.f);
    ospLight.set("direction", m_defaultDirLight_direction);
    ospLight.set("angularDiameter", 0.53f);
    ospLight.commit();
    lights.push_back(ospLight.handle());
  }

  auto lightArray = ospray::cpp::Data(lights.size(), OSP_OBJECT, lights.data());
  //lightArray.commit();
  m_renderer.set("lights", lightArray);
}
