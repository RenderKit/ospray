#include "shankarModel.h"
#include <queue>

#define K_NEAREST_FOR_DISKS 20
#define ADAMSON_ALEXA_RADIUS_SCALE 1.f

namespace ospray {
  namespace shankar {
    using std::cout;
    using std::endl;

    box3f Surface::getBounds() const {
      box3f bounds = embree::empty;
      for (int pID=0;pID<point.size();pID++) 
        bounds.extend(point[pID].pos);
      return bounds;
    }

    void Surface::load(const std::string &fn)
    {
      FILE *file = fopen(fn.c_str(),"rb");
      assert(file);
      int rc;
      Point p;
      while ((rc = fscanf(file,"%f %f %f\n",&p.pos.x,&p.pos.y,&p.pos.z))==3) {
        p.nor = vec3f(0.f);
        p.rad = 0.f;
        point.push_back(p);
      }
      fclose(file);

      computeNormalsAndRadii();
    }

    box3f Model::getBounds() const {
      box3f bounds = embree::empty;
      for (int sID=0;sID<surface.size();sID++) 
        bounds.extend(surface[sID]->getBounds());
      return bounds;
    }
    

    vec3f computeNormal(const std::vector<Point> &point, 
                        const vec3f &orientation=vec3f(0.f))
    {
      vec3f bestNor(0.f);
      float bestLen = 0.f;
      for (int r=0;r<100;r++) {
        int i = int(random()%point.size());
        int j = int(random()%point.size());
        int k = int(random()%point.size());
        const vec3f vi = point[i].pos;
          const vec3f vj = point[j].pos;
            const vec3f vk = point[k].pos;
            vec3f nor = cross(vj-vi,vk-vi);
            float len = length(nor);
            if (len > bestLen) {
              bestLen = len; 
              bestNor = nor;
            }
      }
      if (dot(bestNor,orientation) < 0.f) bestNor = - bestNor;
      return bestNor;
    }

    vec3f computeNormal(const std::vector<Point> &point, 
                        const std::vector<int>   &pointID,
                        const vec3f &orientation=vec3f(0.f))
    {
      vec3f bestNor(0.f);
      float bestLen = 0.f;
      for (int i=0;i<pointID.size();i++) {
        for (int j=i+1;j<pointID.size();j++) {
          for (int k=j+1;k<pointID.size();k++) {
            const vec3f vi = point[pointID[i]].pos;
            const vec3f vj = point[pointID[j]].pos;
            const vec3f vk = point[pointID[k]].pos;
            vec3f nor = cross(vj-vi,vk-vi);
            float len = length(nor);
            if (len > bestLen) {
              bestLen = len; 
              bestNor = nor;
            }
          }
        }
      }
      if (dot(bestNor,orientation) < 0.f) bestNor = - bestNor;
      return bestNor;
    }

    void computeNormalAndRadiusByKNNQuery(Point &p, const std::vector<Point> &point,
                                          const vec3f &surfaceNormal)
    {
      std::priority_queue<std::pair<float,int> > kNN;
      for (int i=0;i<point.size();i++) {
        float dist = length(point[i].pos-p.pos);
        std::pair<float,int> candidate(dist,i);
        kNN.push(candidate);
        if (kNN.size() > K_NEAREST_FOR_DISKS) {
          kNN.pop();
        }
      }
      p.rad = ADAMSON_ALEXA_RADIUS_SCALE * kNN.top().first;
      std::vector<int> pointID;
      while (!kNN.empty()) {
        pointID.push_back(kNN.top().second);
        kNN.pop();
      }
      p.nor = normalize(computeNormal(point,pointID,surfaceNormal));
    }
    
    void Surface::computeNormalsAndRadii()
    {
      cout << "Computing normal and radii" << endl;
      vec3f surfaceNormal = computeNormal(point);
      for (int i=0;i<point.size();i++) 
        computeNormalAndRadiusByKNNQuery(point[i],point,surfaceNormal);
    }
    
  }
}
