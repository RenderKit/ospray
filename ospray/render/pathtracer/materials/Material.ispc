// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
