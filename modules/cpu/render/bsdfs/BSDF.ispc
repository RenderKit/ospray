// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/bsdfs/BSDF.ih"

#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/Dielectric.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/LambertTransmission.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "render/bsdfs/MicrofacetDielectric.ih"
#include "render/bsdfs/MicrofacetDielectricLayer.ih"
#include "render/bsdfs/MicrofacetSheenLayer.ih"
#include "render/bsdfs/Minneart.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/OrenNayar.ih"
#include "render/bsdfs/Reflection.ih"
#include "render/bsdfs/RobustDielectric.ih"
#include "render/bsdfs/RobustThinDielectric.ih"
#include "render/bsdfs/Scale.ih"
#include "render/bsdfs/Specular.ih"
#include "render/bsdfs/ThinDielectric.ih"
#include "render/bsdfs/ThinMicrofacetDielectric.ih"
#include "render/bsdfs/Transmission.ih"
#include "render/bsdfs/Velvety.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL BSDF_EvalRes BSDF_dispatch_eval(
    const varying BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
  cl::sycl::ext::oneapi::experimental::printf("BSDF_dispatch_eval\n");
#endif
  switch (self->bsdfType) {
  case BSDF_TYPE_MULTI_BSDF:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_MULTI_BSDF\n");
#endif
    return MultiBSDF_eval(self, wo, wi);
  default:
    return BSDF_dispatch_eval_nomulti(self, wo, wi);
  }
}

SYCL_EXTERNAL BSDF_SampleRes BSDF_dispatch_sample(
    const varying BSDF *uniform self, const vec3f &wo, const vec2f &s, float ss)
{
  switch (self->bsdfType) {
  case BSDF_TYPE_MULTI_BSDF:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_MULTI_BSDF\n");
#endif
    return MultiBSDF_sample(self, wo, s, ss);
  default:
    return BSDF_dispatch_sample_nomulti(self, wo, s, ss);
  }
}

SYCL_EXTERNAL BSDF_EvalRes BSDF_dispatch_eval_nomulti(
    const varying BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
  cl::sycl::ext::oneapi::experimental::printf("BSDF_dispatch_eval_nomulti\n");
#endif
  switch (self->bsdfType) {
  case BSDF_TYPE_DIELECTRIC_LAYER:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_DIELECTRIC_LAYER\n");
#endif
    return DielectricLayer_eval(self, wo, wi);
  case BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER\n");
#endif
    return MicrofacetDielectricLayer_eval(self, wo, wi);
  case BSDF_TYPE_SCALE:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_SCALE\n");
#endif
    return Scale_eval(self, wo, wi);
  case BSDF_TYPE_MICROFACET_SHEEN_LAYER:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_SHEEN_LAYER\n");
#endif
    return MicrofacetSheenLayer_eval(self, wo, wi);
  default:
    return BSDF_dispatch_eval_base(self, wo, wi);
  }
}

SYCL_EXTERNAL BSDF_SampleRes BSDF_dispatch_sample_nomulti(
    const varying BSDF *uniform self, const vec3f &wo, const vec2f &s, float ss)
{
  switch (self->bsdfType) {
  case BSDF_TYPE_DIELECTRIC_LAYER:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_DIELECTRIC_LAYER\n");
#endif
    return DielectricLayer_sample(self, wo, s, ss);
  case BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER\n");
#endif
    return MicrofacetDielectricLayer_sample(self, wo, s, ss);
  case BSDF_TYPE_SCALE:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_SCALE\n");
#endif
    return Scale_sample(self, wo, s, ss);
  case BSDF_TYPE_MICROFACET_SHEEN_LAYER:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_SHEEN_LAYER\n");
#endif
    return MicrofacetSheenLayer_sample(self, wo, s, ss);
  default:
    return BSDF_dispatch_sample_base(self, wo, s, ss);
  }
}

/* TODO: There's tons of BSDF types and having them all inlined into the kernel
 * in a huge switch is not going to be good for spill spize
 */
SYCL_EXTERNAL BSDF_EvalRes BSDF_dispatch_eval_base(
    const varying BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
  cl::sycl::ext::oneapi::experimental::printf("BSDF_dispatch_eval_base\n");
#endif
  switch (self->bsdfType) {
  case BSDF_TYPE_REALLY_UNKNOWN:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_REALLY_UNKNOWN\n");
#endif
    break;
  case BSDF_TYPE_CONDUCTOR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_CONDUCTOR\n");
#endif
    return Conductor_eval(self, wo, wi);
  case BSDF_TYPE_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_DIELECTRIC\n");
#endif
    return Dielectric_eval(self, wo, wi);
  case BSDF_TYPE_LAMBERT:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_LAMBERT\n");
#endif
    return Lambert_eval(self, wo, wi);
  case BSDF_TYPE_LAMBERT_TRANSMISSION:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_LAMBERT_TRANSMISSION\n");
#endif
    return LambertTransmission_eval(self, wo, wi);
  case BSDF_TYPE_MICROFACET_CONDUCTOR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_CONDUCTOR\n");
#endif
    return MicrofacetConductor_eval(self, wo, wi);
  case BSDF_TYPE_MICROFACET_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_DIELECTRIC\n");
#endif
    return MicrofacetDielectric_eval(self, wo, wi);
  case BSDF_TYPE_MINNEART:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_MINNEART\n");
#endif
    return Minneart_eval(self, wo, wi);
  case BSDF_TYPE_OREN_NAYAR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_OREN_NAYAR\n");
#endif
    return OrenNayar_eval(self, wo, wi);
  case BSDF_TYPE_SPECULAR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_SPECULAR\n");
#endif
    return Specular_eval(self, wo, wi);
  case BSDF_TYPE_REFLECTION:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_REFLECTION\n");
#endif
    return Reflection_eval(self, wo, wi);
  case BSDF_TYPE_ROBUST_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_ROBUST_DIELECTRIC\n");
#endif
    return RobustDielectric_eval(self, wo, wi);
  case BSDF_TYPE_ROBUST_THIN_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_ROBUST_THIN_DIELECTRIC\n");
#endif
    return RobustThinDielectric_eval(self, wo, wi);
  case BSDF_TYPE_THIN_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_THIN_DIELECTRIC\n");
#endif
    return ThinDielectric_eval(self, wo, wi);
  case BSDF_TYPE_THIN_MICROFACET_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_THIN_MICROFACET_DIELECTRIC\n");
#endif
    return ThinMicrofacetDielectric_eval(self, wo, wi);
  case BSDF_TYPE_TRANSMISSION:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_TRANSMISSION\n");
#endif
    return Transmission_eval(self, wo, wi);
  case BSDF_TYPE_VELVETY:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_VELVETY\n");
#endif
    return Velvety_eval(self, wo, wi);
  default:
#ifndef OSPRAY_TARGET_DPCPP
    return self->eval(self, wo, wi);
#else
    break;
#endif
  }
  return make_BSDF_EvalRes_zero();
}

SYCL_EXTERNAL BSDF_SampleRes BSDF_dispatch_sample_base(
    const varying BSDF *uniform self, const vec3f &wo, const vec2f &s, float ss)
{
  switch (self->bsdfType) {
  case BSDF_TYPE_CONDUCTOR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_CONDUCTOR\n");
#endif
    return Conductor_sample(self, wo, s, ss);
  case BSDF_TYPE_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_DIELECTRIC\n");
#endif
    return Dielectric_sample(self, wo, s, ss);
  case BSDF_TYPE_LAMBERT:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_LAMBERT\n");
#endif
    return Lambert_sample(self, wo, s, ss);
  case BSDF_TYPE_LAMBERT_TRANSMISSION:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_LAMBERT_TRANSMISSION\n");
#endif
    return LambertTransmission_sample(self, wo, s, ss);
  case BSDF_TYPE_MICROFACET_CONDUCTOR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_CONDUCTOR\n");
#endif
    return MicrofacetConductor_sample(self, wo, s, ss);
  case BSDF_TYPE_MICROFACET_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_MICROFACET_DIELECTRIC\n");
#endif
    return MicrofacetDielectric_sample(self, wo, s, ss);
  case BSDF_TYPE_MINNEART:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_MINNEART\n");
#endif
    return Minneart_sample(self, wo, s, ss);
  case BSDF_TYPE_OREN_NAYAR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_OREN_NAYAR\n");
#endif
    return OrenNayar_sample(self, wo, s, ss);
  case BSDF_TYPE_SPECULAR:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_SPECULAR\n");
#endif
    return Specular_sample(self, wo, s, ss);
  case BSDF_TYPE_REFLECTION:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_REFLECTION\n");
#endif
    return Reflection_sample(self, wo, s, ss);
  case BSDF_TYPE_ROBUST_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_ROBUST_DIELECTRIC\n");
#endif
    return RobustDielectric_sample(self, wo, s, ss);
  case BSDF_TYPE_ROBUST_THIN_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_ROBUST_THIN_DIELECTRIC\n");
#endif
    return RobustThinDielectric_sample(self, wo, s, ss);
  case BSDF_TYPE_THIN_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_THIN_DIELECTRIC\n");
#endif
    return ThinDielectric_sample(self, wo, s, ss);
  case BSDF_TYPE_THIN_MICROFACET_DIELECTRIC:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf(
        "BSDF_TYPE_THIN_MICROFACET_DIELECTRIC\n");
#endif
    return ThinMicrofacetDielectric_sample(self, wo, s, ss);
  case BSDF_TYPE_TRANSMISSION:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_TRANSMISSION\n");
#endif
    return Transmission_sample(self, wo, s, ss);
  case BSDF_TYPE_VELVETY:
#if 0 //#ifdef OSPRAY_TARGET_DPCPP
    cl::sycl::ext::oneapi::experimental::printf("BSDF_TYPE_VELVETY\n");
#endif
    return Velvety_sample(self, wo, s, ss);
  default:
#ifndef OSPRAY_TARGET_DPCPP
    return self->sample(self, wo, s, ss);
#else
    break;
#endif
  }
  return make_BSDF_SampleRes_zero();
}

OSPRAY_END_ISPC_NAMESPACE
