// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "ospray/common/OSPCommon.h"
#include "ospray/render/util.h"

using namespace ospray;

#define V(x) (x - 8)

int main(int ac, char **av)
{
  std::string mtlName = "alphaTestCubes.mtl";
  std::string objName = "alphaTestCubes.obj";
  FILE *mtl           = fopen(mtlName.c_str(), "w");
  FILE *obj           = fopen(objName.c_str(), "w");

  fprintf(obj, "mtllib %s\n\n", mtlName.c_str());

  vec3i numCubes(4);
  if (ac > 1)
    numCubes.x = numCubes.y = numCubes.z = atoi(av[1]);
  if (ac > 2)
    numCubes.y = numCubes.z = atoi(av[2]);
  if (ac > 3)
    numCubes.z = atoi(av[3]);

  int cubeID;
  for (int iz = 0; iz < numCubes.z; iz++)
    for (int iy = 0; iy < numCubes.y; iy++)
      for (int ix = 0; ix < numCubes.x; ix++, cubeID++) {
        box3f bounds;
        bounds.lower.x = (ix + .1f) / float(numCubes.x);
        bounds.upper.x = (ix + .9f) / float(numCubes.x);

        bounds.lower.y = (iy + .1f) / float(numCubes.y);
        bounds.upper.y = (iy + .9f) / float(numCubes.y);

        bounds.lower.z = (iz + .1f) / float(numCubes.z);
        bounds.upper.z = (iz + .9f) / float(numCubes.z);

        vec3f color = makeRandomColor(cubeID);

        fprintf(mtl, "newmtl material%i\n", cubeID);
        fprintf(mtl, "\tKd %f %f %f\n", color.x, color.y, color.z);
        fprintf(mtl, "\td .5f\n");
        fprintf(mtl, "\n");
        fprintf(obj, "usemtl material%i\n", cubeID);

        fprintf(obj,
                "v %f %f %f\n",
                bounds.lower.x,
                bounds.lower.y,
                bounds.lower.z);
        fprintf(obj,
                "v %f %f %f\n",
                bounds.lower.x,
                bounds.lower.y,
                bounds.upper.z);
        fprintf(obj,
                "v %f %f %f\n",
                bounds.lower.x,
                bounds.upper.y,
                bounds.lower.z);
        fprintf(obj,
                "v %f %f %f\n",
                bounds.lower.x,
                bounds.upper.y,
                bounds.upper.z);
        fprintf(obj,
                "v %f %f %f\n",
                bounds.upper.x,
                bounds.lower.y,
                bounds.lower.z);
        fprintf(obj,
                "v %f %f %f\n",
                bounds.upper.x,
                bounds.lower.y,
                bounds.upper.z);
        fprintf(obj,
                "v %f %f %f\n",
                bounds.upper.x,
                bounds.upper.y,
                bounds.lower.z);
        fprintf(obj,
                "v %f %f %f\n",
                bounds.upper.x,
                bounds.upper.y,
                bounds.upper.z);

        // front
        fprintf(obj, "f %i %i %i %i\n", V(0), V(1), V(3), V(2));
        // back
        fprintf(obj, "f %i %i %i %i\n", V(4), V(5), V(7), V(6));
        // left
        fprintf(obj, "f %i %i %i %i\n", V(0), V(1), V(5), V(4));
        // right
        fprintf(obj, "f %i %i %i %i\n", V(2), V(3), V(7), V(6));
        // bottom
        fprintf(obj, "f %i %i %i %i\n", V(0), V(2), V(6), V(4));
        // top
        fprintf(obj, "f %i %i %i %i\n", V(1), V(3), V(7), V(5));
      }
  fclose(mtl);
  fclose(obj);

  printf("test cubes obj/mtl files written to %s/%s\n",
         objName.c_str(),
         mtlName.c_str());
}
