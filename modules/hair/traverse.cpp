#undef NDEBUG

#include "hbvh.h"
// stl
// #include <set>
// #include <algorithm>
// #include <map>
// #include <iterator>
// #include <stack>
// ospray
// #include "common/model.h"
// embree
#include "kernels/common/ray8.h"
// embree
#include "kernels/xeon/bvh4/bvh4.h"
#include "kernels/xeon/bvh4/bvh4_intersector8_hybrid.h"

//#define REAL_INTERSECTION 1

namespace ospray {
  namespace hair {
    using namespace embree;

    // void intersectSegment1(const Segment &seg,
    //                        const size_t k,
    //                        Ray8 &ray,
    //                        int subdivDepth=3)
    // {
    //   if (subdivDepth == 0) {
    //   } else {
    //     Segment s0, s1;
    //     seg.subdivide(s0,s1);
    //   }
    // }

    void intersectRec(Ray8 &ray,
                      const size_t k,
                      int startVertex,
                      const vec3f &v0,
                      const vec3f &v1,
                      const vec3f &v2,
                      const vec3f &v3,
                      const float maxRad,
                      int remainingDepth=1
                      )
    {
      box3f bounds = embree::empty;
      bounds.extend(v0);
      bounds.extend(v1);
      bounds.extend(v2);
      bounds.extend(v3);
      
      if (bounds.upper.x < -maxRad) return;
      if (bounds.upper.y < -maxRad) return;

      if (bounds.lower.x > +maxRad) return;
      if (bounds.lower.y > +maxRad) return;

        const vec3f v00 = v0;
        const vec3f v01 = v1;
        const vec3f v02 = v2;
        const vec3f v03 = v3;

        const vec3f v10 = 0.5f*(v00+v01);
        const vec3f v11 = 0.5f*(v01+v02);
        const vec3f v12 = 0.5f*(v02+v03);

        const vec3f v20 = 0.5f*(v10+v11);
        const vec3f v21 = 0.5f*(v11+v12);

        const vec3f v30 = 0.5f*(v20+v21);

      if (remainingDepth > 0) {
        intersectRec(ray,k,startVertex,v00,v10,v20,v30,maxRad,remainingDepth-1);
        intersectRec(ray,k,startVertex,v30,v21,v12,v03,maxRad,remainingDepth-1);
      } else {
        // if (max(v0.x,v1.x) < -maxRad) return;
        // if (max(v0.y,v1.y) < -maxRad) return;
        // if (max(v1.x,v2.x) < -maxRad) return;
        // if (max(v1.y,v2.y) < -maxRad) return;
        // if (max(v2.x,v3.x) < -maxRad) return;
        // if (max(v2.y,v3.y) < -maxRad) return;

        // if (min(v0.x,v1.x) > +maxRad) return;
        // if (min(v0.y,v1.y) > +maxRad) return;
        // if (min(v1.x,v2.x) > +maxRad) return;
        // if (min(v1.y,v2.y) > +maxRad) return;
        // if (min(v2.x,v3.x) > +maxRad) return;
        // if (min(v2.y,v3.y) > +maxRad) return;
        
        const float z = v30.z;
        if (z > ray.tfar[k]) return;

        ray.primID[k] = (startVertex*13)>>4;
        ray.geomID[k] = (startVertex*13)>>4;
        ray.tfar[k]   = z;
      }
    }
      // vec3f v0(dot(seg.v0.x

    void intersectSegment1(const AffineSpace3f &space,
                           const vec3f &org,
                           const Hairlet *hl, //const BVH4* bvh, 
                           const uint32 &startVertex,
                           const size_t k,
                           Ray8 &ray)
    {
      Segment seg = hl->hg->getSegment(startVertex);
      // vec3f org(ray.org.x[k],ray.org.y[k],ray.org.z[k]);
      // vec3f dir(ray.dir.x[k],ray.dir.y[k],ray.dir.z[k]);

      const vec3f _v0 = seg.v0.pos - org;
      const vec3f _v1 = seg.v1.pos - org;
      const vec3f _v2 = seg.v2.pos - org;
      const vec3f _v3 = seg.v3.pos - org;
      
      const vec3f v0(dot(_v0,space.l.vx),dot(_v0,space.l.vy),dot(_v0,space.l.vz));
      const vec3f v1(dot(_v1,space.l.vx),dot(_v1,space.l.vy),dot(_v1,space.l.vz));
      const vec3f v2(dot(_v2,space.l.vx),dot(_v2,space.l.vy),dot(_v2,space.l.vz));
      const vec3f v3(dot(_v3,space.l.vx),dot(_v3,space.l.vy),dot(_v3,space.l.vz));

      float maxRad = max(max(max(seg.v0.radius,seg.v1.radius),seg.v2.radius),seg.v3.radius);
      intersectRec(ray,k,startVertex,v0,v1,v2,v3,maxRad);
    }

    __forceinline 
    void hairIntersect1(const int hairletID,
                        const Hairlet *hl, //const BVH4* bvh, 
                        // NodeRef root, 
                        const size_t k, 
                        Ray8& ray, 
                        const avx3f &ray_org, 
                        const avx3f &ray_dir, 
                        const avx3f &ray_rdir, 
                        const avxf &ray_tnear, 
                        const avxf &ray_tfar, 
                        const avx3i& nearXYZ
                        )
    {
#if REAL_INTERSECTION
      uint mailbox[32];
      ((avxi*)mailbox)[0] = avxi(-1);
      ((avxi*)mailbox)[1] = avxi(-1);
#endif

#if 0
      vec3f dir_k(ray_dir.x[k],ray_dir.y[k],ray_dir.z[k]);
      AffineSpace3f space = embree::frame(dir_k);
      space.l.vx = normalize(space.l.vx);
      space.l.vy = normalize(space.l.vy);
      space.l.vz = normalize(space.l.vz);

      // test all segments in this hairlet!
      for (int i=0;i<hl->segStart.size();i++) {
        intersectSegment1(space,hl,hl->segStart[i],k,ray);
      }
#elif 0
      // just test hailet bounds ...
      const sse3f org(ray_org.x[k], ray_org.y[k], ray_org.z[k]);
      const sse3f rdir(ray_rdir.x[k], ray_rdir.y[k], ray_rdir.z[k]);
      const sse3f org_rdir(org*rdir);
      const ssef  ray_t0(ray_tnear[k]);
      ssef  ray_t1(ray_tfar[k]);

      ssef _x0 = (ssef(hl->bounds.lower.x)-org.x)*(rdir.x);
      ssef _x1 = (ssef(hl->bounds.upper.x)-org.x)*(rdir.x);

      ssef _y0 = (ssef(hl->bounds.lower.y)-org.y)*(rdir.y);
      ssef _y1 = (ssef(hl->bounds.upper.y)-org.y)*(rdir.y);

      ssef _z0 = (ssef(hl->bounds.lower.z)-org.z)*(rdir.z);
      ssef _z1 = (ssef(hl->bounds.upper.z)-org.z)*(rdir.z);

      ssef x0 = min(_x0,_x1);
      ssef y0 = min(_y0,_y1);
      ssef z0 = min(_z0,_z1);
      ssef x1 = max(_x0,_x1);
      ssef y1 = max(_y0,_y1);
      ssef z1 = max(_z0,_z1);

      ssef t0 = max(max(ray_t0,x0),max(y0,z0));
      ssef t1 = min(min(ray_t1,x1),min(y1,z1));

      if (!none(t0<=t1)) {
        ray.tfar[k] = t0[0];
        ray.primID[k] = (long(hl)*13)>>4;
        ray.geomID[k] = (long(hl)*13)>>4;
      }
#else
      static const int stackSizeSingle = 200;
      typedef int64 NodeRef;
      // typedef const Hairlet::QuadNode *NodeRef;
      StackItemInt32<NodeRef> stack[stackSizeSingle];  //!< stack of nodes 
      StackItemInt32<NodeRef>* stackPtr = stack + 1;        //!< current stack pointer
      StackItemInt32<NodeRef>* stackEnd = stack + stackSizeSingle;
      stack[0].ptr = 0; // &hl->node[0];
      stack[0].dist = neg_inf;

      // bvh4 traversal...
      const sse3f org(ray_org.x[k], ray_org.y[k], ray_org.z[k]);
      const sse3f rdir(ray_rdir.x[k], ray_rdir.y[k], ray_rdir.z[k]);

#if REAL_INTERSECTION
      vec3f dir_k(ray_dir.x[k],ray_dir.y[k],ray_dir.z[k]);
      vec3f org_k(ray_org.x[k],ray_org.y[k],ray_org.z[k]);
      AffineSpace3f space = embree::frame(dir_k);
      space.l.vx = normalize(space.l.vx);
      space.l.vy = normalize(space.l.vy);
      space.l.vz = normalize(space.l.vz);
#endif

      const sse3f org_rdir(org*rdir);
      const ssef  ray_t0(ray_tnear[k]);
      ssef  ray_t1(ray_tfar[k]);
      
      const ssef hl_bounds_min = *(ssef*)&hl->bounds.lower;
      const ssef hl_bounds_max = *(ssef*)&hl->bounds.upper;
      const ssef hl_bounds_scale = (hl_bounds_max-hl_bounds_min)*ssef(1.f/255.f);

      const ssef hl_bounds_scale_x = ssef(hl_bounds_scale[0]);
      const ssef hl_bounds_scale_y = ssef(hl_bounds_scale[1]);
      const ssef hl_bounds_scale_z = ssef(hl_bounds_scale[2]);
      
      const ssef hl_bounds_min_x = ssef(hl_bounds_min[0]);
      const ssef hl_bounds_min_y = ssef(hl_bounds_min[1]);
      const ssef hl_bounds_min_z = ssef(hl_bounds_min[2]);


      // const int64 LEAF_BIT = (1UL << 15);

      --stackPtr;
      NodeRef cur = NodeRef(stackPtr->ptr);
      goto start;

      // long step = 0;

      /* pop loop */
      while (true) {
      pop:
        /*! pop next node */
        if (unlikely(stackPtr == stack)) break;
        stackPtr--;
        // NodeRef cur = NodeRef(stackPtr->ptr);
        cur = NodeRef(stackPtr->ptr);
        
        /*! if popped node is too far, pop next one */
        if (unlikely(*(float*)&stackPtr->dist > ray_tfar[k]))//ray.tfar[k]))
          continue;
        
        /* downtraversal loop */
        while (true) {
          /*! stop if we found a leaf */
          if (unlikely(cur >= 0)) break;
          // if (unlikely(step != 0 && cur >= 0)) break;
          // if (unlikely(cur & 1)) break;

          // ++step;

        start:
          const Hairlet::QuadNode *node = &hl->node[-cur];
          const ssei shift(0,8,16,24);
          const ssei bitMask(255);

          // PRINT(hairletID);
          // PRINT((int*)(uint32&)node->lo_x[0]);
          // PRINT((int*)(uint32&)node->lo_y[0]);
          // PRINT((int*)(uint32&)node->lo_z[0]);
          // PRINT((int*)(uint32&)node->hi_x[0]);
          // PRINT((int*)(uint32&)node->hi_y[0]);
          // PRINT((int*)(uint32&)node->hi_z[0]);
          const ssei _node_lo_x = ssei((uint32&)node->lo_x[0]);
          const ssei _node_lo_y = ssei((uint32&)node->lo_y[0]);
          const ssei _node_lo_z = ssei((uint32&)node->lo_z[0]);
          const ssei _node_hi_x = ssei((uint32&)node->hi_x[0]);
          const ssei _node_hi_y = ssei((uint32&)node->hi_y[0]);
          const ssei _node_hi_z = ssei((uint32&)node->hi_z[0]);

          const ssei node_lo_x 
            = ssei(_mm_srav_epi32(_node_lo_x,shift)) & bitMask;
          const ssei node_lo_y 
            = ssei(_mm_srav_epi32(_node_lo_y,shift)) & bitMask;
          const ssei node_lo_z
            = ssei(_mm_srav_epi32(_node_lo_z,shift)) & bitMask;
          const ssei node_hi_x 
            = ssei(_mm_srav_epi32(_node_hi_x,shift)) & bitMask;
          const ssei node_hi_y 
            = ssei(_mm_srav_epi32(_node_hi_y,shift)) & bitMask;
          const ssei node_hi_z
            = ssei(_mm_srav_epi32(_node_hi_z,shift)) & bitMask;
          
          const sseb box_valid = (node_lo_x <= node_hi_x);
          
          // const sseb box_valid
          //   = (node_lo_x <= node_hi_x)
          //   & (node_lo_y <= node_hi_y)
          //   & (node_lo_z <= node_hi_z)
          //   ;
          
          // ideally, bake that into a ray transformation ...
          const ssef world_lo_x
            = madd(ssef(node_lo_x), hl_bounds_scale_x, hl_bounds_min_x);
          const ssef world_lo_y
            = madd(ssef(node_lo_y), hl_bounds_scale_y, hl_bounds_min_y);
          const ssef world_lo_z
            = madd(ssef(node_lo_z), hl_bounds_scale_z, hl_bounds_min_z);
          const ssef world_hi_x
            = madd(ssef(node_hi_x), hl_bounds_scale_x, hl_bounds_min_x);
          const ssef world_hi_y
            = madd(ssef(node_hi_y), hl_bounds_scale_y, hl_bounds_min_y);
          const ssef world_hi_z
            = madd(ssef(node_hi_z), hl_bounds_scale_z, hl_bounds_min_z);
          // const ssef world_lo_x = madd(ssef(node_lo_x), hl_bounds_scale[0], hl_bounds_min[0]);
          // const ssef world_lo_y = madd(ssef(node_lo_y), hl_bounds_scale[1], hl_bounds_min[1]);
          // const ssef world_lo_z = madd(ssef(node_lo_z), hl_bounds_scale[2], hl_bounds_min[2]);
          // const ssef world_hi_x = madd(ssef(node_hi_x), hl_bounds_scale[0], hl_bounds_min[0]);
          // const ssef world_hi_y = madd(ssef(node_hi_y), hl_bounds_scale[1], hl_bounds_min[1]);
          // const ssef world_hi_z = madd(ssef(node_hi_z), hl_bounds_scale[2], hl_bounds_min[2]);

          // PRINT(hl_bounds_min);
          // PRINT(hl_bounds_max);
          // PRINT(_node_lo_x);
          // PRINT(_node_lo_y);
          // PRINT(_node_lo_z);
          // PRINT(_node_hi_x);
          // PRINT(_node_hi_y);
          // PRINT(_node_hi_z);
          // PRINT(node_lo_x);
          // PRINT(node_lo_y);
          // PRINT(node_lo_z);
          // PRINT(node_hi_x);
          // PRINT(node_hi_y);
          // PRINT(node_hi_z);
          // PRINT(world_lo_x);
          // PRINT(world_lo_y);
          // PRINT(world_lo_z);
          // PRINT(world_hi_x);
          // PRINT(world_hi_y);
          // PRINT(world_hi_z);
          
          const ssef maxRad(hl->maxRadius);
          const ssef t_lo_x = msub(world_lo_x-maxRad,rdir.x,org_rdir.x);
          const ssef t_lo_y = msub(world_lo_y-maxRad,rdir.y,org_rdir.y);
          const ssef t_lo_z = msub(world_lo_z-maxRad,rdir.z,org_rdir.z);
          const ssef t_hi_x = msub(world_hi_x+maxRad,rdir.x,org_rdir.x);
          const ssef t_hi_y = msub(world_hi_y+maxRad,rdir.y,org_rdir.y);
          const ssef t_hi_z = msub(world_hi_z+maxRad,rdir.z,org_rdir.z);
          
          const ssef t0_x = min(t_lo_x,t_hi_x);
          const ssef t0_y = min(t_lo_y,t_hi_y);
          const ssef t0_z = min(t_lo_z,t_hi_z);
          const ssef t1_x = max(t_lo_x,t_hi_x);
          const ssef t1_y = max(t_lo_y,t_hi_y);
          const ssef t1_z = max(t_lo_z,t_hi_z);
          
          const ssef t0 = max(t0_x,t0_y,t0_z,ray_t0);
          const ssef t1 = min(t1_x,t1_y,t1_z,ray_t1);
          const sseb vmask = (t0 <= t1) & box_valid;
          size_t mask = movemask(vmask);

          /*! if no child is hit, pop next node */
          if (unlikely(mask == 0))
            goto pop;
          
          /*! one child is hit, continue with that child */
          size_t r = __bscf(mask);
          if (likely(mask == 0)) {
            cur = (int16&)node->child[r];//node->child(r);
            // assert(cur != BVH4::emptyNode);
            continue;
          }


              /*! two children are hit, push far child, and continue with closer child */
          NodeRef c0 = (int16&)node->child[r]; //c0.prefetch(); 
          const unsigned int d0 = ((unsigned int*)&t0)[r];
          r = __bscf(mask);
          NodeRef c1 = (int16&)node->child[r]; //c1.prefetch(); 
          const unsigned int d1 = ((unsigned int*)&t0)[r];

          if (likely(mask == 0)) {
            // assert(stackPtr < stackEnd);
            if (d0 < d1) { 
              stackPtr->ptr = c1; stackPtr->dist = d1; stackPtr++; cur = c0; 
              continue; 
            } else { 
              stackPtr->ptr = c0; stackPtr->dist = d0; stackPtr++; cur = c1; 
              continue; 
            }
          }
          
          /*! Here starts the slow path for 3 or 4 hit children. We push
           *  all nodes onto the stack to sort them there. */
          // assert(stackPtr < stackEnd);
          stackPtr->ptr = c0; stackPtr->dist = d0; stackPtr++;
          // assert(stackPtr < stackEnd);
          stackPtr->ptr = c1; stackPtr->dist = d1; stackPtr++;
          
          /*! three children are hit, push all onto stack and sort 3 stack items, continue with closest child */
          // assert(stackPtr < stackEnd);
          r = __bscf(mask);
          NodeRef c = (int16&)node->child[r]; unsigned int d = ((unsigned int*)&t0)[r]; stackPtr->ptr = c; stackPtr->dist = d; stackPtr++;
          
          
          // assert(c0 != BVH4::emptyNode);
          if (likely(mask == 0)) {
            sort(stackPtr[-1], stackPtr[-2], stackPtr[-3]);
            cur = (NodeRef)stackPtr[-1].ptr; stackPtr--;
            continue;
          }
          

          /*! four children are hit, push all onto stack and sort 4 stack items, continue with closest child */
          // assert(stackPtr < stackEnd);
          r = __bscf(mask);
          c = (int16&)node->child[r]; d = ((unsigned int*)&t0)[r]; stackPtr->ptr = c; stackPtr->dist = d; stackPtr++;
          // assert(c != BVH4::emptyNode);
          sort(stackPtr[-1], stackPtr[-2], stackPtr[-3], stackPtr[-4]);
          cur = (NodeRef)stackPtr[-1].ptr; stackPtr--;
          
          // if (none(hitMask)) return;
          // // PRINT(hitMask);
          // size_t bits = movemask(hitMask);
          // for (size_t i=__bsf(bits); bits!=0; bits=__btc(bits,i), i=__bsf(bits)) {
          //   float t = t0[i];
          //   if (t >= ray.tfar[k]) continue;
          //   ray.tfar[k] = t;
          //   ray.primID[k] = i+((long(hl)*13)>>4);
          //   ray.geomID[k] = i+((long(hl)*13)>>4);
          // }
        }

        // /*! this is a leaf node */
        // STAT3(normal.trav_leaves, 1, 1, 1);
        // size_t num; Primitive* prim = (Primitive*)cur.leaf(num);
        // PrimitiveIntersector8::intersect(ray, k, prim, num, bvh->geometry);
        // rayFar = ray.tfar[k];
        // PRINT(cur);
        
#if REAL_INTERSECTION 
        while (1) {
          uint segID = hl->leaf[cur].hairID;// hl->segStart[;
          
          // mailboxing
          uint *mb = &mailbox[segID%32];
          if (*mb != segID) {
            const int startVertex = hl->segStart[segID];
            *mb = segID;
            intersectSegment1(space,org_k,hl,startVertex,k,ray);
          }
          if (hl->leaf[cur].eol) break;
          ++cur;
        }
#else
        const float hit_dist = *(float*)&stackPtr[-1].dist;
        if (hit_dist < ray.tfar[k]) {
          ray.tfar[k] =  hit_dist; //cur.dist; //t0[0];
          // ray.primID[k] = (long(hl)*13)>>4;
          // ray.geomID[k] = (long(hl)*13)>>4;

          int segID = hl->leaf[cur].hairID;// hl->segStart[;
          ray.primID[k] = (long(segID)*13)>>4;
          ray.geomID[k] = (long(segID)*13)>>4;
          ray_t1 = hit_dist;//ray.tfar[k];
        }
#endif
      }
#endif
    }


    void intersectHairlet(const void* _valid,  /*!< pointer to valid mask */
                          void* ptr,          /*!< pointer to user data */
                          embree::Ray8 &ray,       /*!< ray packet to intersect */
                          size_t item         /*!< item to intersect */)
    {
      avxb valid = *(avxb*)_valid;

      Hairlet *hl = ((HairBVH*)ptr)->hairlet[item];
      avx3f ray_org = ray.org, ray_dir = ray.dir;
      avxf ray_tnear = ray.tnear, ray_tfar  = ray.tfar;
      const avx3f rdir = rcp_safe(ray_dir);
      const avx3f org(ray_org), org_rdir = org * rdir;

      /* compute near/far per ray */
      // iw: not yet using this optimization ...
      avx3i nearXYZ;
      nearXYZ.x = select(rdir.x >= 0.0f,
                         avxi(0*(int)sizeof(ssef)),
                         avxi(1*(int)sizeof(ssef)));
      nearXYZ.y = select(rdir.y >= 0.0f,
                         avxi(2*(int)sizeof(ssef)),
                         avxi(3*(int)sizeof(ssef)));
      nearXYZ.z = select(rdir.z >= 0.0f,
                         avxi(4*(int)sizeof(ssef)),
                         avxi(5*(int)sizeof(ssef)));

      size_t bits = movemask(valid);
      if (bits==0) return;
      // if (unlikely(__popcnt(bits) <= SWITCH_THRESHOLD)) {
      for (size_t i=__bsf(bits); bits!=0; bits=__btc(bits,i), i=__bsf(bits)) {
        hairIntersect1(item,hl,//bvh, curNode, 
                       i, ray, ray_org, ray_dir, rdir, ray_tnear, ray_tfar, nearXYZ);
        ray_tfar = ray.tfar;
      }
    }
  }
}
