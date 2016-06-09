#include "CameraParser.h"

bool DefaultCameraParser::parse(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--camera" || arg == "-c") {
      m_cameraType = av[++i];
    } else if (arg == "-vp" || arg == "--eye") {
      auto &pos = m_eye;
      pos.x = atof(av[++i]);
      pos.y = atof(av[++i]);
      pos.z = atof(av[++i]);
    } else if (arg == "-vu" || arg == "--up") {
      auto &up = m_up;
      up.x = atof(av[++i]);
      up.y = atof(av[++i]);
      up.z = atof(av[++i]);
    } else if (arg == "-vi" || arg == "--gaze") {
      auto &at = m_gaze;
      at.x = atof(av[++i]);
      at.y = atof(av[++i]);
      at.z = atof(av[++i]);
    }
  }

  finalize();

  return true;
}

ospray::cpp::Camera DefaultCameraParser::camera()
{
  return m_camera;
}

void DefaultCameraParser::finalize()
{
  if (m_cameraType.empty())
    m_cameraType = "perspective";

  m_camera = ospray::cpp::Camera(m_cameraType.c_str());
  m_camera.set("pos", m_eye);
  m_camera.set("up",  m_up);
  m_camera.set("dir", m_gaze - m_eye);
  m_camera.commit();
}
