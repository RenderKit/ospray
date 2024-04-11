// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#if 0

////////////////////////////////////////////////////////////////////////////////
// pure pseudo random sampler as debugging baseline

struct LDSampler
{
  RandomSampler rng;
};

inline void LDSampler_init(varying LDSampler &self,
    const uint32 seed,
    const uint32 sampleIndex,
    const uint32 sampleOffsetFirstLight = 0,
    const uint32 sampleOffsetLight = 0,
    const uniform uint32 bits = 16,
    const uniform uint32 bitsFirstLight = 16,
    const uniform uint32 bitsLight = 16)
{
  RandomSampler_init(&self.rng, seed, sampleIndex);
}

inline uint32 LDSampler_getIndex(LDSampler &self)
{
  return self.rng.stream; // dummy
}

inline void LDSampler_nextGroup(LDSampler &) {}

inline vec2f RandomSampler_getFloat2(LDSampler &self)
{
  vec2f res;
  res.x = RandomSampler_getFloat(&self.rng);
  res.y = RandomSampler_getFloat(&self.rng);
  return res;
}

inline vec2f LDSampler_get3LightSamples(LDSampler &self, const uint32, const bool, float &ss)
{
  ss = RandomSampler_getFloat(&self.rng);
  return RandomSampler_getFloat2(self);
}

inline vec2f LDSampler_getNext4Samples(LDSampler &self, float &ss, uint32 &)
{
  return LDSampler_get3LightSamples(self, 0, true, ss);
}

inline float LDSampler_finalizeDim3(LDSampler &self, const uint32)
{
  return RandomSampler_getFloat(&self.rng);
}

inline vec2f LDSampler_getNext5Samples(LDSampler &self, vec3ui &)
{
  return RandomSampler_getFloat2(self);
}

inline vec2f LDSampler_finalizeDim23(LDSampler &self, const vec3ui &)
{
  return RandomSampler_getFloat2(self);
}

inline float LDSampler_finalizeDim4(LDSampler &self, const vec3ui &)
{
  return RandomSampler_getFloat(&self.rng);
}

#else

////////////////////////////////////////////////////////////////////////////////
// plain Burley implementation to verify optimized version

struct LDSampler
{
  uint32 index; // sample index
  uint32 scramble; // seed for scrambling the samples
  uint32 baseIndexFirstLight; // when sequence is split (blue noise)
  uint32 baseIndexLight;
  float s0;
  float s1;
  float s2;
  float s3;
  float s4;
  // valid bits TODO should move to global PT Context
  uniform uint32 idxMask;
  uniform uint32 idxMaskFirstLight;
  uniform uint32 idxMaskLight;
};

inline void LDSampler_init(varying LDSampler &self,
    const uint32 seed,
    const uint32 sampleIndex,
    const uint32 sampleOffsetFirstLight = 0,
    const uint32 sampleOffsetLight = 0,
    const uniform uint32 bits = 16,
    const uniform uint32 bitsFirstLight = 16,
    const uniform uint32 bitsLight = 16)
{
  self.index = sampleIndex;
  self.scramble = seed; // avoid zero!
  self.baseIndexFirstLight = sampleOffsetFirstLight;
  self.baseIndexLight = sampleOffsetLight;
  self.idxMask = bits > 31 ? -1u : (1u << bits) - 1;
  self.idxMaskFirstLight =
      bitsFirstLight > 31 ? -1u : (1u << bitsFirstLight) - 1;
  self.idxMaskLight = bitsLight > 31 ? -1u : (1u << bitsLight) - 1;
}

inline void LDSampler_nextGroup(LDSampler &self)
{
  // padding: new decorrelated group
  self.scramble = tripleHash(self.scramble);
}

inline void LDSampler_sampleGroup(LDSampler &self)
{
  const uint32 index =
      OwenScramble2(self.index, self.scramble) & self.idxMask; // shuffle

  // sample
  const uint32 r0 = Sobol_sample(index, 0);
  const uint32 r1 = Sobol_sample(index, 1);
  const uint32 r2 = Sobol_sample(index, 2);
  const uint32 r3 = Sobol_sample(index, 3);
  const uint32 r4 = Sobol_sample(index, 4);

  // scramble
  self.s0 = to_float_unorm(OwenScramble2(r0, self.scramble ^ SCRAMBLE0));
  self.s1 = to_float_unorm(OwenScramble2(r1, self.scramble ^ SCRAMBLE1));
  self.s2 = to_float_unorm(OwenScramble2(r2, self.scramble ^ SCRAMBLE2));
  self.s3 = to_float_unorm(OwenScramble2(r3, self.scramble ^ SCRAMBLE3));
  self.s4 = to_float_unorm(OwenScramble2(r4, self.scramble ^ SCRAMBLE4));
}

inline vec2f LDSampler_get3LightSamples(const LDSampler &self,
    const uniform uint32 idx,
    const bool first,
    float &ss)
{
  LDSampler lds;
  lds.scramble = self.scramble;
  lds.index = (first ? self.baseIndexFirstLight : self.baseIndexLight) + idx;
  // XXX should be: = first ? self.idxMaskFirstLight : self.idxMaskLight;
  lds.idxMask = self.idxMaskFirstLight;
  LDSampler_sampleGroup(lds);

  ss = lds.s2;
  return make_vec2f(lds.s0, lds.s1);
}

inline vec2f LDSampler_getNext4Samples(LDSampler &self, float &ss, uint32 &)
{
  LDSampler_nextGroup(self);
  LDSampler_sampleGroup(self);
  ss = self.s2;
  return make_vec2f(self.s0, self.s1);
}

inline float LDSampler_finalizeDim3(const LDSampler &self, const uint32)
{
  return self.s3;
}

inline vec2f LDSampler_getNext5Samples(LDSampler &self, vec3ui &)
{
  LDSampler_nextGroup(self);
  LDSampler_sampleGroup(self);
  return make_vec2f(self.s0, self.s1);
}

inline vec2f LDSampler_finalizeDim23(const LDSampler &self, const vec3ui &)
{
  return make_vec2f(self.s2, self.s3);
}

inline float LDSampler_finalizeDim4(const LDSampler &self, const vec3ui &)
{
  return self.s4;
}

#endif
