#include "hkd.h"

namespace ospray {
  namespace hair {

    const int BITS_PER_DIM = 10;
    const int BITS_TOTAL = 3*BITS_PER_DIM;

    /*! recursively build kd-tree; seglist is list of segments
        specified via resp 'first vertex id' */
    void buildRec(HairKD *kd, 
                  int nodeID, std::vector<int> &item,
                  const box3f &bounds, int dim, int depth)
    {
#if 0
      if (item.empty()) {
        kd->nodeVec[nodeID].dim = 3;
        kd->nodeVec[nodeID].offset = -1;
      } else if (depth == BITS_TOTAL) {
        kd->nodeVec[nodeID].dim = 3;
        int offs = kd->itemVec.size();
        kd->nodeVec[nodeID].offset = offs;
        for (int i=0;i<item.size();i++) 
          kd->itemVec.push_back(HairKD::Item());
        for (int i=0;i<item.size();i++) {
          kd->itemVec[offs+i].segID = item[i];
          kd->itemVec[offs+i].eol = 0;
        }
        kd->itemVec[offs+item.size()-1].eol = 1;
        item.clear();
      } else {
        int offs = kd->nodeVec.size();

        Assert(bounds.lower[dim] != bounds.upper[dim]);

        float split = 0.5f*(bounds.lower[dim]+bounds.upper[dim]);

        box3f lBounds = bounds, rBounds = bounds;
        lBounds.upper[dim] = rBounds.lower[dim] = split;
        std::vector<int> lItem, rItem;
        for (int i=0;i<item.size();i++) {
          Segment seg = kd->getSegment(item[i]);
          if (overlaps(seg,lBounds)) lItem.push_back(item[i]);
          if (overlaps(seg,rBounds)) rItem.push_back(item[i]);
        }
        // optimization: remove 'useless' splits
        if (lItem == rItem) {
          kd->nodeVec[nodeID].dim = 3;
          int offs = kd->itemVec.size();
          kd->nodeVec[nodeID].offset = offs;
          for (int i=0;i<item.size();i++) 
            kd->itemVec.push_back(HairKD::Item());
          for (int i=0;i<item.size();i++) {
            kd->itemVec[offs+i].segID = item[i];
            kd->itemVec[offs+i].eol = 0;
          }
          kd->itemVec[offs+item.size()-1].eol = 1;
          item.clear();
        } else {
          if (depth < 3)
            cout << "level " << depth << " : partitioned " << item.size() << " -> " << lItem.size() << " + " << rItem.size() << endl;
          
          kd->nodeVec[nodeID].offset = offs;
          kd->nodeVec.push_back(HairKD::Node());
          kd->nodeVec.push_back(HairKD::Node());
          kd->nodeVec[nodeID].dim = dim;
          kd->nodeVec[nodeID].split = split;
          
          item.clear();
          item.resize(0);
          buildRec(kd,kd->nodeVec[nodeID].offset+0,lItem,lBounds,(dim+1)%3,depth+1);
          buildRec(kd,kd->nodeVec[nodeID].offset+1,rItem,rBounds,(dim+1)%3,depth+1);
        }
      }
#endif
    }

    void HairKD::build() {
#if 0
      Assert(item == NULL);
      Assert(node == NULL);

      cout << "Transforming hair geometry to origin ..." << endl;
      // box4f one(vec4f(0.f),vec4f(1.f));
      box4f orgSpace = hg->bounds;
      PRINT(orgSpace);
      hg->transformToOrigin();
      PRINT(hg->bounds);

      cout << "starting to build ..." << endl;
      box3f domain;
      domain.lower = (vec3f&)hg->bounds.lower;
      domain.upper = (vec3f&)hg->bounds.upper;
      std::vector<int> item;

      nodeVec.push_back(Node());
      nodeVec.push_back(Node());
      for (int i=0;i<hg->curve.size();i++)
        for (int j=0;j<hg->curve[i].numSegments;j++)
          item.push_back(hg->curve[i].firstVertex+3*j);
      cout << "starting to build over " << item.size() << " fragments" << endl;
      buildRec(this,0,item,domain,0,0);
      PRINT(nodeVec.size());
      PRINT(itemVec.size());
      this->item = &itemVec[0];
      this->node = &nodeVec[0];
#endif
    }

  }
}

