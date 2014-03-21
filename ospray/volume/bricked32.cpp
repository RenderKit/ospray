#include "bricked32.h"
#include "bricked32_ispc.h"

#ifdef LOW_LEVEL_KERNELS
# include <immintrin.h>
#endif

namespace ospray {
  // template<>
  // void BrickedVolumeSampler<float>::setRegion(const vec3i &where, 
  //                                             const vec3i &size, 
  //                                             const float *data)
  // {
  //   uint8 t[long(size.x)*size.y*size.z];
  //   for (long i=0;i<long(size.x)*size.y*size.z;i++)
  //     t[i] = uint8(255.999f*data[i]);
  //   ispc::_Bricked32Volume1uc_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
  //                                       (const ispc::vec3i&)size,t);
  // }
  template<>
  void BrickedVolumeSampler<uint8>::setRegion(const vec3i &where, 
                                              const vec3i &size, 
                                              const uint8 *data)
  {
    ispc::_Bricked32Volume1uc_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
                                        (const ispc::vec3i&)size,data);
  }





  // template<>
  // void BrickedVolumeSampler<float>::setRegion(const vec3i &where, 
  //                                    const vec3i &size, 
  //                                    const uint8 *data)
  // {
  //   float t[long(size.x)*size.y*size.z];
  //   for (long i=0;i<long(size.x)*size.y*size.z;i++)
  //     t[i] = float(255.999f*data[i]);
  //   ispc::_Bricked32Volume1f_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
  //                                      (const ispc::vec3i&)size,t);
  // }

  template<>
  void BrickedVolumeSampler<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    ispc::_Bricked32Volume1f_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,data);
  }

  template<>
  float BrickedVolumeSampler<float>::lerpf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }
  template<>
  float BrickedVolumeSampler<uint8>::lerpf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }

  template struct StructuredVolume<BrickedVolumeSampler<uint8> >;
  template struct StructuredVolume<BrickedVolumeSampler<float> >;

  typedef StructuredVolume<BrickedVolumeSampler<uint8> > BrickedVolume_uint8;
  typedef StructuredVolume<BrickedVolumeSampler<float> > BrickedVolume_float;

  OSP_REGISTER_VOLUME(BrickedVolume_float,bricked_float);
  OSP_REGISTER_VOLUME(BrickedVolume_float,bricked_float32);
}
