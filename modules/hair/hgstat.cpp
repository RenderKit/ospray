#undef NDEBUG

#include "hairgeom.h"

namespace ospray {
  namespace hair {
    HairGeom hg;

    float minSize[4];
    float maxSize[4];
    long  numRead = 0;
    long  nextPing = 1;

    void printStats() {
      cout << "--------------------------------------------" << endl;
      PRINT(hg.bounds);
      PRINT(hg.curve.size());
      PRINT(numRead);
      for (int d=0;d<4;d++) {
        PRINT(d);
        PRINT(minSize[d]);
        PRINT(maxSize[d]);
        PRINT(int(log2f(hg.bounds.size()[d] / maxSize[d])));
      }
    }

    void stats(const std::string &fileName)
    {
      hg.load(fileName);
      hg.transformToOrigin();

      numRead = 0;
      for (int d=0;d<4;d++) {
        maxSize[d] = 0.f;
        minSize[d] = std::numeric_limits<float>::infinity();
      }
      
      // PRINT(hg.curve.size());
      for (int cID=0;cID<hg.curve.size();cID++) {
        // PRINT(hg.curve[cID].numSegments);
        for (int sID=0;sID<hg.curve[cID].numSegments;sID++) {
          // int segStart = hg.curve[cID].firstVertex+3*sID;
          Segment s = hg.getSegment(cID,sID);
          box4f sbounds = s.bounds_inner();
          vec4f ssize = sbounds.size();
          ++numRead;
          for (int d=0;d<4;d++) {
            if (ssize[d] < minSize[d]) minSize[d] = ssize[d];
            if (ssize[d] > maxSize[d]) maxSize[d] = ssize[d];
          }

          if (numRead == nextPing) {
            printStats();
            nextPing *= 2;
          }
        }
      }
      printStats();
    }
  }
}

int main(int ac, char **av)
{
  Assert(ac == 2);
  ospray::hair::stats(av[1]);
}
