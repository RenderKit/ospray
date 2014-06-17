#include "light.ih"

static void Light_Constructor(uniform Light *uniform light,
                              void *uniform cppEquivalent)
{
  light->cppEquivalent = cppEquivalent;
}

extern void Light_Destructor(uniform Light *uniform light) {}
