#undef NDEBUG

#include "hbvh.h"
// stl
#include <set>
#include <algorithm>
#include <map>
#include <iterator>
#include <stack>
// ospray
#include "common/model.h"
// embree
#include "kernels/common/ray8.h"

// #if 1
// # define COMPUTEOVERLAP(a,b,c) computeOverlap_outer((a),(b),(c))
// # define BOUNDS(a) a.bounds_outer()
// #else
// # define COMPUTEOVERLAP(a,b,c) computeOverlap_inner((a),(b),(c))
// # define BOUNDS(a) a.bounds_inner()
// #endif

namespace ospray {
  namespace hair {

    FILE *file = NULL;

    const int try_leaf_threshold = 10000;
    const float hairlet_threshold_in_avg_radius = 4.f;
    const float hairlet_max_depth = 30;

    size_t numSegsProcessed = 0;

    const float travCost = 1.f;
    const float isecCost = 4.f;
    float avgRadius = 0.f;

    long numQuadChildren = 0;
    long numQuadNodes = 0;
    long totalTmpQuadNodes = 0;

    bool notEmpty3(const box4f &b)
    {
      if (b.lower.x > b.upper.x) return false;
      if (b.lower.y > b.upper.y) return false;
      if (b.lower.z > b.upper.z) return false;
      return true;
    }

    // stats:
    long sumInitialRefs = 0;
    long sumInitialNodes = 0;
    long sumCollapsedRefs = 0;
    long sumCollapsedNodes = 0;

    /*! tmpnodes are used for temporarily computing a binary BVH in a hairlet */
    struct TmpNode {
      std::vector<int> segStart;
      box3f bounds;
      TmpNode *l, *r;

      TmpNode *&operator[](int i) { return i?r:l; }

      bool isLeaf() const { return !segStart.empty(); }

      TmpNode() : bounds(embree::empty), l(NULL), r(NULL) {}
      TmpNode(std::vector<int> &segStart, const box3f &bounds) 
        : segStart(segStart), bounds(bounds), l(NULL), r(NULL)
      {}
      ~TmpNode() {
        if (l) delete l;
        if (r) delete r;
      }
      void split(Hairlet *hairlet, int depthRemaining, const float maxWidth) 
      {
        if (depthRemaining <= 0) return; // cannot split ...

        vec3f size = bounds.size();
        int dim = 0;
        if (size[1] > size[dim]) dim = 1;
        if (size[2] > size[dim]) dim = 2;
        if (size[dim] <= maxWidth) return; // fine enough - should not split
          
        // create two children ...
        l = new TmpNode;
        r = new TmpNode;
        
        box3f lSpace=bounds,rSpace=bounds;
        float split = center(bounds)[dim];
        lSpace.upper[dim] = std::min(lSpace.upper[dim],split);
        rSpace.lower[dim] = std::max(rSpace.lower[dim],split);

        // ... and partition, computing the childrens' IDs and bounds
        for (int i=0;i<segStart.size();i++) {
          Segment seg = hairlet->hg->getSegment(segStart[i]);
          box3f segBounds = seg.bounds3f_outer();
          box3f lOverlap = embree::empty;
          box3f rOverlap = embree::empty;
          if (computeOverlap(seg,lSpace,lOverlap)) {
            l->segStart.push_back(segStart[i]);
            l->bounds.extend(lOverlap);
          }
          if (computeOverlap(seg,rSpace,rOverlap)) {
            r->segStart.push_back(segStart[i]);
            r->bounds.extend(rOverlap);
          }
        }
        Assert(!l->segStart.empty());
        Assert(!r->segStart.empty());
        // now, have partitioned input geometry and have *actual*
        // boudns in children.  since we do spatial partitioning we
        // can now clamp those boudns to the spatial boudns we're
        // building over
        l->bounds = intersect(l->bounds,lSpace);
        r->bounds = intersect(r->bounds,rSpace);

        segStart.clear();
        l->split(hairlet,depthRemaining-1,maxWidth);
        r->split(hairlet,depthRemaining-1,maxWidth);
      }
    };

    inline float area(TmpNode *node)
    {
      vec3f size = node->bounds.size();
      return size.x*size.y+size.y*size.z+size.x*size.z;
    }

    inline float area(const box4f &bounds)
    {
      const vec4f size = bounds.size();
      return size.x*size.y+size.y*size.z+size.x*size.z;
    }
    inline float area(const box3f &bounds)
    {
      const vec3f size = bounds.size();
      return size.x*size.y+size.y*size.z+size.x*size.z;
    }



    struct TmpQuadNode {
      struct Node {
        TmpNode *src;
        box3f bounds;
        std::vector<int> segStart; // for leaf node
        TmpQuadNode *ptr; // for inner node

        Node() 
          : ptr(NULL), src(NULL)
        {};
        Node(const Node &other) 
          : ptr(other.ptr), src(other.src), segStart(other.segStart), bounds(other.bounds)
        {};
        ~Node() 
        { 
          if (ptr)  {
            // cout << "recursively deleting quad " << ptr << " from node " << this << endl; 
            delete ptr; 
            ptr = NULL; 
          }
        }
        
        inline bool isLeaf() const { return !segStart.empty(); }
        TmpQuadNode::Node &operator=(const TmpQuadNode::Node &b)
        {
          this->segStart = b.segStart;
          this->ptr = b.ptr;
          this->src = b.src;
          this->bounds = b.bounds;
          return *this;
        }
        void clear() 
        {
          ptr = NULL;
          src = NULL;
          segStart.clear();
        }
      };
      Node child[4];
      int size;
      TmpQuadNode() : size(0) { ++totalTmpQuadNodes; };
      void clear() { Assert(size <= 4); for (int i=0;i<4;i++) child[i].clear(); size = 0; }
      ~TmpQuadNode() {

        --totalTmpQuadNodes;
        // cout << "deleting " << (void*)this << endl;
        Assert(size <= 4); 
        // PRINT(size);
        // for (int i=0;i<4;i++)
        //   PRINT(child[i].ptr);
        //   // for (int i=0;i<4;i++)
        //   //   if (child[i].ptr) delete child[i].ptr; 
      }
    };

    void checkTree(TmpQuadNode *qn)
    {
      Assert2(qn,"NULL NODE");
      Assert2(qn->size <= 4,"invalid node size");
      for (int i=0;i<qn->size;i++) {
        if (qn->child[i].ptr)
          checkTree(qn->child[i].ptr);
      }
      for (int i=qn->size;i<4;i++)
        Assert2(qn->child[i].ptr == NULL,"non-cleared node");
    }

    hair::TmpQuadNode *makeQBVH(TmpNode *node)
    {
      TmpQuadNode *qn = new TmpQuadNode;
      qn->child[0].src = node;
      qn->size = 1;

      while (qn->size < 4) {
        int bestCand = -1;
        float bestGain = -1.f; // < 0 to allow even zero-area segments
        for (int cand=0;cand<qn->size;cand++) {
          if (qn->child[cand].src->isLeaf())
            // cannot split leaf nodes
            continue;
          float candGain = area(qn->child[cand].src);
          if (candGain > bestGain) {
            bestCand = cand;
            bestGain = candGain;
          }
        }
        if (bestCand == -1) 
          // could not find split candidate
          break;
        TmpNode *candSrc = qn->child[bestCand].src;
        qn->child[bestCand].src = candSrc->l;
        qn->child[qn->size].src = candSrc->r;
        ++qn->size;
      }
      // PING;
      // checkTree(qn);

      // if (qn->size == 1) cout << "ERROR" << endl;
      // PRINT(qn->size);
      // PRINT(node->isLeaf());
      // PRINT(node->bounds);
      // PRINT(area(node));
      // PRINT(node->l);
      // PRINT(node->r);
      // PRINT(node->segStart.size());
      numQuadNodes++;
      numQuadChildren += qn->size;
      // PRINT(numQuadChildren/double(numQuadNodes));
      for (int i=0;i<qn->size;i++) {
        qn->child[i].bounds = qn->child[i].src->bounds;
        if (qn->child[i].src->isLeaf()) {
          qn->child[i].segStart = qn->child[i].src->segStart;
        } else {
          qn->child[i].ptr = makeQBVH(qn->child[i].src);
        }
        qn->child[i].src = NULL;
      }


      // // -------------------------------------------------------
      // // post-process that merged two dual-child nodes...
      // std::vector<int> twoNodeChildren;
      // for (int i=0;i<qn->size;i++) {
      //   if (!qn->child[i].isLeaf() && qn->child[i].ptr->size == 2)
      //     twoNodeChildren.push_back(i);
      // }
      // if (twoNodeChildren.size() >= 2) {
      //   // one new quad node appears:
      //   numQuadNodes += 1;
      //   numQuadChildren += 4;
      //   // two old pair-nodes disappear
      //   numQuadNodes -= 2;
      //   numQuadChildren -= 2*2;
      //   // the current node loses two chilren ...
      //   numQuadChildren -= 2;
      //   // but gains one new one
      //   numQuadChildren += 1;
      // }
      
      // PING;
      // checkTree(qn);
      return qn;
    }
    
    
    Hairlet::Hairlet(HairGeom *hg, std::vector<int> &segStart)
      : hg(hg), segStart(segStart)
    {}

    int countNodes(TmpNode *node)
    {
      if (!node) return 0;
      return 1+countNodes(node->l)+countNodes(node->r);
    }
    int countRefs(TmpNode *node)
    {
      if (!node) return 0;
      return node->segStart.size()+countRefs(node->l)+countRefs(node->r);
    }

    float collapseSAH(TmpNode *node, std::set<int> &parentColl)
    {
      Assert(node);
      if (node->segStart.empty()) {
        std::set<int> collapsed;
        float lCost = collapseSAH(node->l,collapsed);
        float rCost = collapseSAH(node->r,collapsed);
        float cost = //travCost+
          (lCost * area(node->l) + rCost * area(node->r)) / area(node);
        float collapsedCost = isecCost*collapsed.size();
        if (cost > collapsedCost) {
          std::copy(collapsed.begin(),collapsed.end(),
                    std::back_insert_iterator<std::vector<int> >(node->segStart));
          delete node->l;
          delete node->r;
          node->l = NULL;
          node->r = NULL;
          std::copy(node->segStart.begin(),node->segStart.end(),
                    std::inserter(parentColl,parentColl.begin()));
          std::copy(collapsed.begin(),collapsed.end(),
                    std::inserter(parentColl,parentColl.begin()));
          return collapsedCost;
        }
        std::copy(collapsed.begin(),collapsed.end(),
                  std::inserter(parentColl,parentColl.begin()));
        return cost;
      } else {
        std::copy(node->segStart.begin(),node->segStart.end(),
                  std::inserter(parentColl,parentColl.begin()));
        return isecCost*node->segStart.size();
      }
    }

    void treeRot(TmpNode *node)
    {
      if (!node) return;
      if (node->isLeaf()) return;

      treeRot(node->l);
      treeRot(node->r);

      int bestChild = -1;
      int bestGrandChild = -1;
      float bestGain = 1e-6f;

      for (int cID=0;cID<2;cID++) {
        TmpNode *child = (*node)[cID];
        TmpNode *sibling = (*node)[1-cID];
        if (child->isLeaf()) continue;
        // try swapping sibling with grandchild, grandsibling stays
        for (int gcID=0;gcID<2;gcID++) {
          TmpNode *grandChild   = (*child)[gcID];
          TmpNode *grandSibling = (*child)[1-gcID];

          float grandSiblingArea = area(grandSibling);
          float grandChildArea   = area(grandChild);
          float siblingArea      = area(sibling);

          float newSiblingArea = grandChildArea;
          float newGrandChildArea = siblingArea;
          box3f newChildBounds = embree::merge(grandSibling->bounds,sibling->bounds);
          float newChildArea = area(newChildBounds);
          float newGrandSiblingArea = grandSiblingArea; // grand sibling stays...

          float oldArea = area(sibling)+area(child)+area(grandChild)+area(grandSibling);
          float newArea = newSiblingArea+newChildArea+newGrandChildArea+newGrandSiblingArea;
          float gain = oldArea - newArea;
          if (gain > bestGain) {
            bestChild = cID;
            bestGrandChild = gcID;
            bestGain = gain;
          }
        } 
      }
      if (bestChild >= 0) {
        TmpNode *&child = bestChild?node->r:node->l;
        TmpNode *&sibling = bestChild?node->l:node->r;
        TmpNode *&grandChild = bestGrandChild?child->r:child->l;
        TmpNode *&grandSibling = bestGrandChild?child->l:child->r;

        // #if 1
        //         TmpNode *oldSibling = sibling;
        //         TmpNode *oldGrandChild = grandChild;
        //         if (bestChild==1)
        //           node->l = oldGrandChild;
        //         else
        //           node->r = oldGrandChild;
        //         if (bestGrandChild==1)
        //           child->r = oldSibling;
        //         else
        //           child->l = oldSibling;
        // #endif

          std::swap(sibling,grandChild);
          child->bounds = merge(child->l->bounds,child->r->bounds);

          // and try again ...
          // PRINT(bestGain);
          // PRINT(node);
          treeRot(node);
      }
    }
    bool mergeOneUp(TmpQuadNode *qn) 
    {
      if (qn->size == 4) return false;
      float bestGain = -1;
      int   bestChild = -1;
      for (int i=0;i<qn->size;i++) {
        if (qn->child[i].isLeaf()) 
          // can't merge up children
          continue;
        if ((qn->child[i].ptr->size - 1 + qn->size) > 4)
          // too big
          continue;
        float gain = area(qn->child[i].bounds);
        if (gain > bestGain) {
          bestGain = gain;
          bestChild = i;
        }
      }
      if (bestChild >= 0) {
        TmpQuadNode *c = qn->child[bestChild].ptr;
        for (int i=1;i<c->size;i++) {
          qn->child[qn->size].bounds = c->child[i].bounds;
          qn->child[qn->size].segStart = c->child[i].segStart;
          qn->child[qn->size].ptr = c->child[i].ptr;
          c->child[i].ptr = NULL;
          c->child[i].segStart.clear();
          qn->size++;
        }
        qn->child[bestChild].bounds = c->child[0].bounds;
        qn->child[bestChild].segStart = c->child[0].segStart;
        qn->child[bestChild].ptr = c->child[0].ptr;
        c->child[0].ptr = NULL;
        c->child[0].segStart.clear();

        // current node gains 'c.size-1'
        numQuadChildren += c->size-1; 
        // old node loses c.size
        numQuadChildren -= c->size;
        numQuadNodes    -= 1;
        delete c;
        return true;
      }
      return false;
    }
    bool mergeTwoChildren(TmpQuadNode *qn)
    {
      if (!qn) return false;

      int best_i = -1;
      int best_j = -1;
      float bestGain = -std::numeric_limits<float>::infinity();
      for (int i=0;i<qn->size;i++) {
        if (qn->child[i].isLeaf()) continue;
        TmpQuadNode *ni = qn->child[i].ptr;
        for (int j=i+1;j<qn->size;j++) {
          if (qn->child[j].isLeaf()) continue;
          TmpQuadNode *nj = qn->child[j].ptr;
          Assert(ni != nj);
          if ((ni->size + nj->size) == 4) {
            float gain = area(qn->child[i].bounds)+area(qn->child[j].bounds)
              -area(merge(qn->child[i].bounds,qn->child[j].bounds));
            if (gain > bestGain) {
              best_i = i;
              best_j = j;
              bestGain = gain;
            }
          }
        }
      }
      if (best_i == -1) return false;

      // PRINT(best_i);
      // PRINT(best_j);
      // PRINT(qn->size);

      TmpQuadNode *ni = qn->child[best_i].ptr;
      TmpQuadNode *nj = qn->child[best_j].ptr;
      // PING;
      // PRINT(ni);
      // PRINT(nj);
      // PRI(ni->size);
      // PRINT(nj->size);

      TmpQuadNode *newNode = new TmpQuadNode;
      for (int i=0;i<ni->size;i++) 
        { newNode->child[newNode->size] = ni->child[i]; newNode->size++; }
      for (int j=0;j<nj->size;j++) 
        { newNode->child[newNode->size] = nj->child[j]; newNode->size++; }
      Assert(newNode->size <= 4);

      // PRINT(ni->size);
      // PRINT(nj->size);
      // PRINT(newNode->size);

      // have a new node here
      numQuadChildren += newNode->size;
      numQuadNodes    ++;
      // current node loses one child
      numQuadChildren += (+1-2);
      // two old children get lost
      numQuadNodes -= 2;
      numQuadChildren -= ni->size;
      numQuadChildren -= nj->size;

      qn->child[best_i].bounds = merge(qn->child[best_i].bounds,qn->child[best_j].bounds);
      qn->child[best_i].ptr = newNode;
      if (best_j != qn->size-1) {
        qn->child[best_j] = qn->child[qn->size-1];
      } 
      qn->child[qn->size-1] = TmpQuadNode::Node();
      qn->size--;
      Assert(qn->size < 5);

      ni->clear(); delete ni;
      nj->clear(); delete nj;

      return true;
    }
    void collapseQBVH(TmpQuadNode *qn)
    {
      if (!qn) return;
      for (int i=0;i<qn->size;i++)
        collapseQBVH(qn->child[i].ptr);
      // PING;
      // checkTree(qn);
      while (1) {
        if (mergeOneUp(qn)) {
          // PING;
          // checkTree(qn);
          continue;
        }
        if (mergeTwoChildren(qn)) {
          // PING;
          // checkTree(qn);
          continue;
        }
        break;
      }
      // PING;
      // checkTree(qn);
    }
    template<int dim>
    void encodeBoundsDim(Hairlet *hairlet, Hairlet::QuadNode &out, TmpQuadNode *in,
                         int childID)
    {
      if (childID >= in->size) {
#if QUAD_NODE_AOS
        out.set(dim,childID,255,0);
        out.child[childID] = (uint16)-1;
#else
        out.bounds[childID].lower[dim] = 255;
        out.bounds[childID].upper[dim] = 0;
        out.child[childID] = (uint16)-1;
#endif
      } else if (hairlet->bounds.lower[dim] == hairlet->bounds.upper[dim]) {
#if QUAD_NODE_AOS
        out.set(dim,childID,0,0);
#else
        out.bounds[childID].lower[dim] = 0;
        out.bounds[childID].upper[dim] = 0;
#endif
      } else {
        float in_lo = in->child[childID].bounds.lower[dim];
        float in_hi = in->child[childID].bounds.upper[dim];
        float dom_lo = hairlet->bounds.lower[dim];
        float dom_hi = hairlet->bounds.upper[dim];

        int quant_lo = int(floor((255.*((in_lo-dom_lo))/double(dom_hi-dom_lo))));
        int quant_hi = int(ceil((255.*((in_hi-dom_lo))/double(dom_hi-dom_lo))));
        if (quant_hi > 255) quant_hi = 255;
#if QUAD_NODE_AOS
        out.set(dim,childID,quant_lo,quant_hi);
#else
        out.bounds[childID].lower[dim] = quant_lo;//int(floorf((255*(in_lo-dom_lo))/(dom_hi-dom_lo)));
        out.bounds[childID].upper[dim] = quant_hi;//int(ceilf((255*(in_hi-dom_lo))/(dom_hi-dom_lo)));
#endif
      }
    }
    void encodeBounds(Hairlet *hairlet, Hairlet::QuadNode &out, TmpQuadNode *in)
    {
      for (int i=0;i<4;i++) {
        encodeBoundsDim<0>(hairlet,out,in,i);
        encodeBoundsDim<1>(hairlet,out,in,i);
        encodeBoundsDim<2>(hairlet,out,in,i);
      }

      // PRINT((int*)(uint32&)out.lo_x[0]);
      // PRINT((int*)(uint32&)out.lo_y[0]);
      // PRINT((int*)(uint32&)out.lo_z[0]);
      // PRINT((int*)(uint32&)out.hi_x[0]);
      // PRINT((int*)(uint32&)out.hi_y[0]);
      // PRINT((int*)(uint32&)out.hi_z[0]);
    }

    char *encode(Hairlet *hairlet, TmpQuadNode *qnRoot)
    {
      std::vector<Hairlet::Fragment> fragList;
      std::vector<Hairlet::QuadNode> nodeList;

      std::stack<TmpQuadNode *> nodeStack;
      std::stack<int> idStack;

      nodeList.push_back(Hairlet::QuadNode());
      idStack.push(0);
      nodeStack.push(qnRoot);
      // std::vector<int> localSegStart;
      
      while (!nodeStack.empty()) {
        TmpQuadNode *qn = nodeStack.top(); nodeStack.pop();
        int nodeID = idStack.top(); idStack.pop();
        // PRINT(qn->size);
        // PRINT(nodeID);
        nodeList[nodeID].clear();
        for (int cID=0;cID<qn->size;cID++) {
          encodeBounds(hairlet,nodeList[nodeID],qn);
          if (qn->child[cID].isLeaf()) {
            // PRINT(qn->child[cID].segStart.size());
            nodeList[nodeID].child[cID] = fragList.size();
            for (int i=0;i<qn->child[cID].segStart.size();i++) {
              Hairlet::Fragment frag;
              frag.eol    = (i==(qn->child[cID].segStart.size()-1));
              frag.hairID = qn->child[cID].segStart[i];
              fragList.push_back(frag);
            }
          } else {
            nodeList[nodeID].child[cID] = -nodeList.size();
            idStack.push(nodeList.size());
            nodeStack.push(qn->child[cID].ptr);
            nodeList.push_back(Hairlet::QuadNode());
          }
        }
      }
      if (nodeList.size() >= (1UL<<15)) { return "too many nodes"; }
      if (fragList.size() >= (1UL<<15)) { return "too many frags"; }
      // if (localSegStart.size() >= (1<<15)) { return "too many segments for this hairlet"; }

      // hairlet->segStart = localSegStart;
      hairlet->node = nodeList;
      hairlet->leaf = fragList;
      return NULL;
    }
    void Hairlet::build(const box3f &domain)
    {
      this->bounds = domain;
      // box3f bounds=embree::empty;
      // bounds=embree::empty;
      // for (int i=0;i<segStart.size();i++) {
      //   Segment seg = hg->getSegment(segStart[i]);
      //   box3f segBounds = seg.bounds3f();
      //   bounds.extend(segBounds);
      // }
      // this->bounds.lower = (const vec3f&)bounds.lower;
      // this->bounds.upper = (const vec3f&)bounds.upper;
      // // this->bounds.upper.x += 1e-6f;
      // // this->bounds.upper.y += 1e-6f;
      // // this->bounds.upper.z += 1e-6f;
      // vec3ui numCells;
      // numCells.x = int(bounds.size().x/avgRadius+.9999f);
      // numCells.y = int(bounds.size().y/avgRadius+.9999f);
      // numCells.z = int(bounds.size().z/avgRadius+.9999f);

      float maxRadius = 0;
      for (int i=0;i<segStart.size();i++) {
        int startVertex = segStart[i];
        maxRadius = std::max(maxRadius,hg->vertex[startVertex+0].radius);
        maxRadius = std::max(maxRadius,hg->vertex[startVertex+1].radius);
        maxRadius = std::max(maxRadius,hg->vertex[startVertex+2].radius);
        maxRadius = std::max(maxRadius,hg->vertex[startVertex+3].radius);
      }

      TmpNode *root = new TmpNode;
      root->bounds = bounds;
      root->segStart = segStart;
      const float voxel_max_width = maxRadius * hairlet_threshold_in_avg_radius;
      root->split(this,hairlet_max_depth,voxel_max_width);

      numSegsProcessed += segStart.size();

      // PRINT(numCells);
      int initialNodes = countNodes(root);
      int initialRefs = countRefs(root);

      std::set<int> collapsed;
      collapseSAH(root,collapsed);
      treeRot(root);
      collapsed.clear();
      collapseSAH(root,collapsed);

      int collapsedNodes = countNodes(root);
      int collapsedRefs = countRefs(root);

      sumInitialNodes += initialNodes;
      sumInitialRefs  += initialRefs;
      sumCollapsedNodes += collapsedNodes;
      sumCollapsedRefs  += collapsedRefs;

      // cout << "hairlet #=" << segStart.size() << " -> init.nodes:" << initialNodes << ",refs:" << initialRefs << endl;
      // cout << "          " << segStart.size() << " -> coll.nodes:" << collapsedNodes << ",refs:" << collapsedRefs << endl;



      hair::TmpQuadNode *qnRoot = makeQBVH(root);
      // cout << "INTIIAL CHECK" << endl;
      // checkTree(qnRoot);

      delete root;

      collapseQBVH(qnRoot);
      // PRINT(numQuadChildren/double(numQuadNodes));
      char *err = encode(this,qnRoot);
      
      // cout << "deleting root" << endl;
      delete qnRoot;
      // PRINT(totalTmpQuadNodes);
      if (err) throw "error in encoding";
      // cout << "done deleting root" << endl;
    }

    Hairlet *HairBVH::makeHairlet(std::vector<int> &segStart, const box3f &domain)
    {
      // if we need more than 15 bits we can't encode, anyway
      if (segStart.size() >= (1<<15)) return NULL;

      Hairlet *hairlet = new Hairlet(hg,segStart);
      try {
        hairlet->build(domain);
      } catch (...) {
        delete hairlet;
        return NULL;
      }
      return hairlet;
    }

    Hairlet *Hairlet::load(HairGeom *hg, FILE *file)
    {
      Assert(file);
      Hairlet *hl = new Hairlet(hg);
      try {
        int rc;
        
        rc = fread(&hl->bounds,sizeof(hl->bounds),1,file);
        if (rc != 1) throw 1;
        // rc = fread(&hl->maxRadius,sizeof(hl->maxRadius),1,file);
        // if (rc != 1) throw 1;

        int num_node;
        rc = fread(&num_node,sizeof(num_node),1,file);
        if (rc != 1) throw 1;
        hl->node.resize(num_node);
        rc = fread(&hl->node[0],sizeof(hl->node[0]),num_node,file);
        if (rc != num_node) throw 1;

        int num_leaf;
        rc = fread(&num_leaf,sizeof(num_leaf),1,file);
        if (rc != 1) throw 1;
        hl->leaf.resize(num_leaf);
        rc = fread(&hl->leaf[0],sizeof(hl->leaf[0]),num_leaf,file);
        if (rc != num_leaf) throw 1;

        // int num_segStart;
        // rc = fread(&num_segStart,sizeof(num_segStart),1,file);
        // if (rc != 1) throw 1;
        // hl->segStart.resize(num_segStart);
        // rc = fread(&hl->segStart[0],sizeof(hl->segStart[0]),num_segStart,file);
        // if (rc != num_segStart) throw 1;
        return hl;
      } catch (... ) {
        delete hl;
        return NULL;
      }
    }

    void Hairlet::save(FILE*file)
    {
      if (!file) return;
      fwrite(&bounds,sizeof(bounds),1,file);
      // fwrite(&maxRadius,sizeof(maxRadius),1,file);

      int num_node = node.size();
      fwrite(&num_node,sizeof(num_node),1,file);
      fwrite(&node[0],sizeof(node[0]),num_node,file);

      int num_leaf = leaf.size();
      fwrite(&num_leaf,sizeof(num_leaf),1,file);
      fwrite(&leaf[0],sizeof(leaf[0]),num_leaf,file);

      // int num_segStart = segStart.size();
      // fwrite(&num_segStart,sizeof(num_segStart),1,file);
      // fwrite(&segStart[0],sizeof(segStart[0]),num_segStart,file);

      // PRINT(leaf.size());
      // PRINT(node.size());

      static long numWritten = 0;
      static long numHairletsWritten = 0;
      numWritten += segStart.size();
      numHairletsWritten++;
      long pos = ftell(file);
      cout << "done hairlet #" << numHairletsWritten << ", #seg=" << numWritten << ", size/seg = " << (pos/float(numWritten)) << endl;
    }

    void HairBVH::buildRecSpatial(std::vector<int> &segStart, const box3f &domain)
    {
      if (segStart.empty()) return;

      Assert(!segStart.empty());
      if (segStart.size() <= try_leaf_threshold) {
        Hairlet *hairlet = makeHairlet(segStart,domain);
        if (hairlet) {
          this->hairlet.push_back(hairlet);

          // #######################################################$
          if (file) {
            hairlet->save(file);
            delete hairlet;
          }
          segStart.clear();
          return;
        }
      }
      std::vector<int> lSegStart, rSegStart;
      vec3f size = domain.size();
      int dim = 0;
      if (size[1] > size[dim]) dim = 1;
      if (size[2] > size[dim]) dim = 2;

      box3f lSpace=domain,rSpace=domain;
      float split = center(domain)[dim];
      lSpace.upper[dim] = std::min(lSpace.upper[dim],split);
      rSpace.lower[dim] = std::max(rSpace.lower[dim],split);

      box3f lDomain = embree::empty, rDomain = embree::empty;

      for (int i=0;i<segStart.size();i++) {
        Segment seg = hg->getSegment(segStart[i]);
        box3f lOverlap = embree::empty;
        box3f rOverlap = embree::empty;
        // Hairlet *l = new Hairlet(hg);
        // Hairlet *r = new Hairlet(hg);
        // cout << "--------------------------------------------" << endl;
        if (computeOverlap(seg,lSpace,lOverlap)) {
          lSegStart.push_back(segStart[i]);
          // l->bounds.extend(lOverlap);
          lDomain.extend(lOverlap);
        }
        if (computeOverlap(seg,rSpace,rOverlap)) {
          rSegStart.push_back(segStart[i]);
          // r->bounds.extend(rOverlap);
          rDomain.extend(rOverlap);
        }
      }
      cout << segStart.size() << " -> " << lSegStart.size() << " + " << rSegStart.size() << endl;
      // Assert(!lSegStart.empty());
      // Assert(!rSegStart.empty());
      segStart.clear();

      buildRecSpatial(lSegStart,embree::intersect(lDomain,lSpace));
      buildRecSpatial(rSegStart,embree::intersect(rDomain,rSpace));
    }

    void HairBVH::load(const std::string &fileName, uint32 maxHairletsToLoad)
    {
      FILE *file = fopen(fileName.c_str(),"rb");
      Assert(file);

      for (uint32 i=0;i<maxHairletsToLoad;i++) {
        Hairlet *hl = Hairlet::load(hg,file);
        if (!hl) break;
        hairlet.push_back(hl);
      }
      cout << "Loaded " << hairlet.size() << " hairlets" << endl;
      fclose(file);
    }

    void intersectHairlet(const void* _valid,  /*!< pointer to valid mask */
                          void* ptr,          /*!< pointer to user data */
                          embree::Ray8 &ray,       /*!< ray packet to intersect */
                          size_t item         /*!< item to intersect */);

    void hairletBounds(void* ptr,              /*!< pointer to user data */
                       size_t item,            /*!< item to calculate bounds for */
                       RTCBounds& bounds_o     /*!< returns calculated bounds */)
    {
      HairBVH *hbvh = (HairBVH *)ptr;
      // cout << "hair " << item << " bounds " << hbvh->hairlet[item]->bounds << endl;
      (vec3f&)bounds_o.lower_x = hbvh->hairlet[item]->bounds.lower;
      (vec3f&)bounds_o.upper_x = hbvh->hairlet[item]->bounds.upper;
    }


    void HairBVH::build(FILE *file) 
    {
      hair::file = file;

      //      file = fopen("/tmp/hair.hbvh","wb");
      cout << "Transforming hair geometry to origin ..." << endl;
      // box4f one(vec4f(0.f),vec4f(1.f));
      box4f orgSpace = hg->bounds;
      PRINT(orgSpace);
      hg->transformToOrigin();
      PRINT(hg->bounds);

      cout << "num *curves* found: " << hg->curve.size() << endl;

      std::vector<int> segStart;
      for (int i=0;i<hg->curve.size();i++) {
        for (int j=0;j<hg->curve[i].numSegments;j++)
          segStart.push_back(hg->curve[i].firstVertex+3*j);
      }
      double sumRadii = 0.f;
      for (int i=0;i<hg->vertex.size();i++)
        sumRadii += hg->vertex[i].radius;
      avgRadius = sumRadii / hg->vertex.size();

      size_t numSegs = segStart.size();
      cout << "num *segments* found: " << segStart.size() << endl;
      box3f rootBounds((vec3f&)hg->bounds.lower,(vec3f&)hg->bounds.upper);
      PING;
      PRINT(segStart.size());
      buildRecSpatial(segStart,rootBounds);
      // buildRec(segStart);
      cout << "done building. num hairlets: " << hairlet.size() << endl;
    }

    void HairBVH::finalize(Model *model)
    {
      void  *hgPtr = this->getVoidPtr("hg",NULL);
      Assert(hgPtr != NULL);
      this->hg = (HairGeom*)hgPtr;
      cout << "found hg : " << hg << endl;

      uint32 maxBricks = this->getParam1i("max_bricks",-1);
      std::string hbvhFile = this->getParamString("hbvh","");
      Assert(hbvhFile != "");
      load(hbvhFile,maxBricks);



      cout << "adding hair user geometry" << endl;
      uint32 geomID = rtcNewUserGeometry(model->eScene,hairlet.size());
      rtcSetUserData(model->eScene,geomID,this);
      rtcSetBoundsFunction(model->eScene,geomID,hairletBounds);
      rtcSetIntersectFunction8(model->eScene,geomID,(RTCIntersectFunc8)intersectHairlet);
      rtcEnable(model->eScene,geomID);
    }

    OSP_REGISTER_GEOMETRY(HairBVH,hair_bvh);
  }
}
