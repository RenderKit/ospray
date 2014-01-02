/*! \file ospray/render/util.h \brief Utility-functions for shaders */

inline vec3f make_random_color(const int i)
{
  const int mx = 13*17*43;
  const int my = 11*29;
  const int mz = 7*23*63;
  const uint g = (i * (3*5*127)+12312314);
  return make_vec3f((g % mx)*(1.f/(mx-1)),
                    (g % my)*(1.f/(my-1)),
                    (g % mz)*(1.f/(mz-1)));
}

