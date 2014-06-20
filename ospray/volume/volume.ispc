#include "volume.ih"

uniform float biggestFloatSmallerThan(const uniform int i)
{
  const uniform float f = i;
  uniform float delta = 1.f;
  while ((f-.5f*delta) != f) {
    delta *= .5f; 
  }
  return f-delta;
}



// #define DEFINE_RESAMPLER(FCT,T,floatToT,setRegionFct)                   \
//   task void FCT#Task(uniform VolumePtr dst,                             \
//                      uniform VolumePtr src)                             \
//   {                                                                     \
//     uniform int tID = taskIndex;                                        \
//                                                                         \
//     const uniform vec3i begin                                           \
//       = make_vec3i(0,tID % dst->size.y,tID / dst->size.y);              \
//     const uniform vec3i size  = make_vec3i(dst->size.x,1,1);            \
//                                                                         \
//     vec3f samplePos;                                                    \
//     samplePos.y = (begin.y / (dst->size.y-1.f)) * src->size.y;          \
//     samplePos.z = (begin.z / (dst->size.z-1.f)) * src->size.z;          \
//                                                                         \
//     uniform T line[16*4096];                                            \
//     foreach (x=0 ... dst->size.x) {                                     \
//       samplePos.x = (x / (dst->size.x-1.f)) * src->size.x;              \
//       float f = src->lerpf(src,samplePos);                              \
//       line[x] = floatToT(f);                                            \
//     }                                                                   \
//     setRegionFct(dst,begin,size,&line[0]);                              \
//   }                                                                     \
//                                                                         \
//   export void FCT(uniform ExtVolumePtr _dst,                            \
//                   uniform ExtVolumePtr _src)                            \
//   {                                                                     \
//     uniform Volume *uniform dst = (uniform Volume *uniform)_dst;        \
//     uniform Volume *uniform src = (uniform Volume *uniform)_src;        \
//     launch[dst->size.y*dst->size.z] resampleVolumeTask(dst,src);        \
//   }

// DEFINE_RESAMPLER(resample_naive32_float,float,,Naive32

// task void resampleVolumeTask(uniform VolumePtr dst, uniform VolumePtr src)
// {
//   // uniform Volume *uniform dst = (uniform Volume *uniform)_dst;
//   // uniform Volume *uniform src = (uniform Volume *uniform)_src;
//   uniform int tID = taskIndex;

//   const uniform vec3i begin = make_vec3i(0,tID % dst->size.y,tID / dst->size.y);
//   const uniform vec3i size  = make_vec3i(dst->size.x,1,1);

//   vec3f samplePos;
//   samplePos.y = (begin.y / (dst->size.y-1.f)) * src->size.y;
//   samplePos.z = (begin.z / (dst->size.z-1.f)) * src->size.z;
  
//   uniform float line[16*1024];
//   foreach (x=0 ... dst->size.x) {
//     samplePos.x = (x / (dst->size.x-1.f)) * src->size.x;
//     line[x] = src->lerpf(src,samplePos);
//   }
//   dst->setRegion(dst,begin,size,&line[0]);
// }

// /*! resample 'src' volume to already-allocated 'dst' volume. note that
//     'dst' *must* be a float volume right now */
// export void resampleVolume(uniform ExtVolumePtr _dst, uniform ExtVolumePtr _src)
// {
//   uniform Volume *uniform dst = (uniform Volume *uniform)_dst;
//   uniform Volume *uniform src = (uniform Volume *uniform)_src;
//   launch[dst->size.y*dst->size.z] resampleVolumeTask(dst,src);
// };


// workaround ispc bug #620.
typedef void *voidPtr;

export void getInternalRepresentation(uniform ExtVolumePtr _vol,
                                      uniform int64 &dataSize,
                                      uniform voidPtr &dataPtr)
{
  uniform Volume *uniform vol = (uniform Volume *uniform)_vol;
  dataSize = vol->dataSize;
  dataPtr  = vol->data;
}

export void _Volume_setTransferFunction(uniform ExtVolumePtr _volume, uniform ExtTransferFunctionPtr _transferFunction)
{
    uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
    uniform TransferFunction *uniform transferFunction = (uniform TransferFunction *uniform)_transferFunction;

    volume->transferFunction = transferFunction;
}
