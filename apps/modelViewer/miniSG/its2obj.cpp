/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

#include "common/OSPCommon.h"

using namespace ospray;
using namespace std;

int main(int ac, const char **av)
{
  FILE *in = fopen(av[1],"rb");
  FILE *out = fopen(av[2],"w");
  assert(in);
  assert(out);
  
  int rc;

  int numVertices=0;
  rc = fread(&numVertices,sizeof(numVertices),1,in);
  assert(rc == 1);
  PRINT(numVertices);

  int numTriangles=0;
  rc = fread(&numTriangles,sizeof(numTriangles),1,in);
  assert(rc == 1);
  PRINT(numTriangles);

  int numRuns=0;
  rc = fread(&numRuns,sizeof(numRuns),1,in);
  assert(rc == 1);
  PRINT(numRuns);


  std::vector<int> vi;

  while (1) {
    int v;
    int rc = fread(&v,sizeof(int),1,in);
    if (!rc) break;
    vi.push_back(v);
  }

  PING;
  PRINT(vi.size());
  int *intArr = &vi[0];
  int *runEnd = intArr + vi.size();
  int *lastFoundRun = NULL;
  int numRunsFound = 0;
  int numTrisFound = 0;
  while (1) {
    // find next run:

    int i=0;
    int *run = runEnd;
    while (1) {
      if (run <= intArr)
        break;
      --run; i++;
      // PRINT(i);
      //  PRINT(*run);
      if (*run == i-1) {
        cout << "found run #" << (++numRunsFound) << " of " << i << " items" << endl;
        //        PRINT((vi.size() - (run-intArr)));
        lastFoundRun = run;
        runEnd = run;
        numTrisFound += (i-1)-2;
        PRINT(numTrisFound);
        break;
      }
    }
    if (run <= intArr) break;
    // for (int i=0;pi>intArr;) {
      
    // }
  }

  // //   fprintf(out,"v%iv\n",v);
  // for (int i=0;i<numVertices;i++) {
  //   float v[6];
  //   rc = fread(v,sizeof(float),6,in);
  //   fprintf(out,"v %f %f %f\n",v[0],v[1],v[2]);
  //   assert(rc == 6);
  // }

  // for (int i=0;i<numTriangles;i++) {
  //   vec3i v;
  //   rc = fread(&v,sizeof(float),3,in);
  //   fprintf(out,"f %i %i %i\n",v.x+1,v.y+1,v.z+1);
  //   assert(rc == 3);
  // }
}
