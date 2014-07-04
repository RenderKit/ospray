#include "rbfTree.h"
#include <iostream>
#include <fstream>
#include "rbfTree_ispc.h"
#include "ospray/common/data.h"

namespace ospray {
  namespace shankar {

    using std::ifstream;
    using std::cout;
    using std::endl;

    void computeAccelPlane(RBFTree::Node *node,RBFTree::RBF *begin, RBFTree::RBF *end)
    {
      int N = end - begin;
      if (!N) {
        node->center = vec3f(0.f);
        node->normal = vec3f(0.f);
        node->lower = 0.f;
        node->upper = 0.f;
        return;
      }

      node->center = 0.f;
      for (RBFTree::RBF *rbf = begin; rbf != end; rbf++) {
        node->center += rbf->pos;
      }
      node->center = float(1.f/N) * node->center;

      vec3f bestNor(0.f);
      float bestLen = 0.f;
      for (int r=0;r<100;r++) {
        int i = int(random()%N);
        int j = int(random()%N);
        int k = int(random()%N);
        const vec3f vi = begin[i].pos;
        const vec3f vj = begin[j].pos;
        const vec3f vk = begin[k].pos;
        vec3f nor = cross(vj-vi,vk-vi);
        float len = length(nor);
        if (len > bestLen) {
          bestLen = len; 
          bestNor = nor;
        }
      }
      node->normal = normalize(bestNor);
      node->lower = 0.f;
      node->upper = 0.f;
      for (RBFTree::RBF *rbf = begin; rbf != end; rbf++) {
        float f = dot(rbf->pos - node->center,node->normal);
        node->lower = std::min(node->lower,f);
        node->upper = std::max(node->upper,f);
      }
      // PRINT(node->center);
      // PRINT(node->normal);
      // PRINT(node->lower);
      // PRINT(node->upper);
    }

    int readRec(FILE *file,
                std::vector<RBFTree::RBF> &point,
                std::vector<RBFTree::Node> &node)
    {
      char line[10000];
      char *s = fgets(line,10000,file);
      if (!s) throw std::runtime_error("COULD NOT READ LINE!?");

      int matID=0, numRBFs=0;
      int rc = sscanf(line,"NODE material %i, #RBFs %i (x,y,z,fnc,coeff)\n",
                      &matID,&numRBFs);
      if (rc == 0) {
        if (std::string(line) != "NULL\n")
          throw std::runtime_error("could not parse line "+std::string(line));
        return -1;
      }

      if (numRBFs) {
        cout << "surface w/ " << numRBFs << " RBFs" << endl;
      }
      RBFTree::Node n;
      int nodeID = node.size();
      n.beginRBFs = point.size();
      n.endRBFs = n.beginRBFs+numRBFs;

      for (int i=0;i<numRBFs;i++) {
        RBFTree::RBF p;
        float fnc;
        fscanf(file,"%f %f %f %f %f\n",&p.pos.x,&p.pos.y,&p.pos.z,&fnc,&p.coeff);
        point.push_back(p);
      }
      computeAccelPlane(&n,&point[n.beginRBFs],&point[n.endRBFs]);

      node.push_back(n);
      node[nodeID].leftNodeID = readRec(file,point,node);
      node[nodeID].rightNodeID = readRec(file,point,node);
      return nodeID;
    }
                 
    void RBFTree::load(const std::string &fileName,
                       OSPData &rbfPointData,
                       OSPData &nodeData,
                       box3f &bounds)
    {
      bounds = embree::empty;
      FILE *in = fopen(fileName.c_str(),"r");
      if (!in) throw std::runtime_error("could not open file "+fileName);

      std::vector<RBF>   point;
      std::vector<Node>  node;

      readRec(in,point,node);
      for (int i=0;i<point.size();i++)
        bounds.extend(point[i].pos);

      rbfPointData = ospNewData(point.size()*4,OSP_FLOAT,
                                &point[0]);
      nodeData = ospNewData(node.size()*13,OSP_INT,
                            &node[0]);
      fclose(in);
    }
    

    RBFTree::RBFTree()
    {
      this->ispcEquivalent = ispc::RBFTree_create(this);
    }

    void RBFTree::finalize(Model *model) 
    {
      box3f bounds;
      rbfData              = getParamData("RBFs",NULL);
      nodeData             = getParamData("nodes",NULL);
      bounds.lower         = getParam3f("bounds.min",vec3f(0.f));
      bounds.upper         = getParam3f("bounds.max",vec3f(0.f));
      if (rbfData == NULL || nodeData == NULL) 
        throw std::runtime_error("#ospray:geometry/shankar/RBFTree: no data specified");
      if (bounds.lower == bounds.upper)
        throw std::runtime_error("#ospray:geometry/shankar/RBFTres: invalid bounds");

      ispc::RBFTreeGeometry_set(getIE(),model->getIE(),
                                rbfData->data,nodeData->data,(ispc::box3f&)bounds);
      PING;
    }
    
    
    OSP_REGISTER_GEOMETRY(RBFTree,rbfTree);
    
  }


}
