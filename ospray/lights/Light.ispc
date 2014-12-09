#include "Light.ih"

//! Construct a new ispc side light
static void Light_Constructor(uniform Light *uniform light,
                              void *uniform cppEquivalent)
{
  light->cppEquivalent = cppEquivalent;
}

//! Destroy an ispc side light - stub
extern void Light_Destructor(uniform Light *uniform light) {}
