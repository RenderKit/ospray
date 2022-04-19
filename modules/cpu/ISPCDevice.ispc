// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

OSPRAY_BEGIN_ISPC_NAMESPACE

#ifdef OSPRAY_TARGET_DPCPP
int ISPCDevice_programCount()
{
  // TODO: This should query the GPU width for ISPC,
  // But for DPC++ we're in a scalar context
  return 1;
}

int ISPCDevice_isa()
{
  // TODO: GPU ISA/width detection
  return 0;
}

#else
export uniform int ISPCDevice_programCount()
{
  return programCount;
}

export uniform int ISPCDevice_isa()
{
#if defined(ISPC_TARGET_SSE2)
  return 1;
#elif defined(ISPC_TARGET_SSE4)
  return 2;
#elif defined(ISPC_TARGET_AVX)
  return 3;
#elif defined(ISPC_TARGET_AVX2)
  return 4;
#elif defined(ISPC_TARGET_AVX512KNL)
  return 5;
#elif defined(ISPC_TARGET_AVX512SKX)
  return 6;
#elif defined(ISPC_TARGET_NEON)
  return 7;
#else
  return 0;
#endif
}
#endif
OSPRAY_END_ISPC_NAMESPACE
