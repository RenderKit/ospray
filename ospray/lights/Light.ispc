#include "Light.ih"

varying vec3f defaultEvalEnv(const uniform Light *uniform,
                             const varying vec3f &)
{
  return make_vec3f(0.f);
}
