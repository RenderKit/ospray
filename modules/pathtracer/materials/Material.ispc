#include "Material.ih"

void PathTraceMaterial__selectNextMedium(uniform const PathTraceMaterial *uniform _THIS,
                                varying Medium& currentMedium) 
{ 
}

void PathTraceMaterial__Constructor(uniform PathTraceMaterial *uniform THIS,
                           // uniform DestructorFunc destructor,
                           uniform PathTraceMaterial__shade shade,
                           uniform PathTraceMaterial__selectNextMediumFunc selectNextMedium)
{
  THIS->shade = shade;
  if (selectNextMedium) THIS->selectNextMedium = selectNextMedium; 
  else                  THIS->selectNextMedium = PathTraceMaterial__selectNextMedium;
}
