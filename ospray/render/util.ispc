#include "ospray/render/util.ih"

uniform float precomputedHalton[3][NUM_PRECOMPUTED_HALTON_VALUES];
uniform bool  precomputedHalton_initialized = false;

inline uint32 radicalInverse(const uint32 _v, const uint32 prime)  
{
  uint32 v = _v;
  float nom = 0.f;
  float den = 1.f;
  while(v != 0) {
    uint32 r = v % prime;    
    v = v / prime;
    
    nom = nom * prime + r;
    den = den * prime;
  }
  float ret = nom / den;
  // print("radicalInverse(%,%)=%\n",_v,prime,ret);
  return ret;
}

inline void precomputedHalton_create() {
  if (precomputedHalton_initialized) 
    return;
  foreach (i=0 ... NUM_PRECOMPUTED_HALTON_VALUES) {
    precomputedHalton[0][i] = radicalInverse(i,2);
    precomputedHalton[1][i] = radicalInverse(i,3);
    precomputedHalton[2][i] = radicalInverse(i,5);
  }
  precomputedHalton_initialized = true;
};


