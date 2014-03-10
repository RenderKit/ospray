//#undef NDEBUG

// hair
#include "hbvh.h"
// embree
//#include "kernels/common/globals.h"
#include "kernels/common/ray8.h"
#include "kernels/xeon/bvh4/bvh4.h"
#include "kernels/xeon/bvh4/bvh4_intersector8_hybrid.h"

#define STATS(a) 

// do not use: for some reason it isn't working yet !?
//#define OFFSETS 1

namespace embree
{
  extern avxf coeff0[4];
  extern avxf coeff1[4];
}

namespace ospray {
  namespace hair {
    using namespace embree;

    long numIsecs = 0;
    long numIsecRecs = 0;
    long numHairletTravs = 0;

    struct SegStack {
      ssef v0,v1,v2,v3;
      int depth;
    };

    __forceinline 
    bool finalLineTest(const ssef &a, const ssef &b,
                       Ray8 &ray8,const size_t k)
    {
      const ssef minP = min(a,b);
      const ssef maxP = max(a,b);
      const ssef maxR = shuffle<3,3,3,3>(maxP);
      // test the box over "line seg plus maxR"
      if (movemask((minP > maxR) | (maxP < -maxR)) & 0x3) return false;

      // actual line distance test (in 2D)
      const vec2f Nl(b[1]-a[1],a[0]-b[0]);
      const vec2f Pl(a[0],a[1]);
      if (sqr(dot(Pl,Nl)) > sqr(maxR[3])*dot(Nl,Nl))
      // const float dist2 = sqr(dot(Pl,Nl)) / dot(Nl,Nl);
      // if (dist2 > sqr(maxR[3]))
        return false;

      // depth test
      const float z = minP[2];
      if (z >= ray8.tfar[k]) return false;

      ray8.tfar[k] = z;
      ray8.Ng.x[k] = b[0] - a[0];
      ray8.Ng.y[k] = b[1] - a[1];
      ray8.Ng.z[k] = b[2] - a[2];
      return true;
    }

    __forceinline 
    bool finalSegmentTest(const ssef &v0,
                          const ssef &v1,
                          const ssef &v2,
                          const ssef &v3,
                          Ray8 &ray8,
                          const size_t k)
    {
      return 
        finalLineTest(v0,v1,ray8,k) |
        finalLineTest(v1,v2,ray8,k) |
        finalLineTest(v2,v3,ray8,k);
    }

    __forceinline 
    bool segmentTest(const ssef &v0,
                     const ssef &v1,
                     const ssef &v2,
                     const ssef &v3,
                     const Ray8 &ray8,
                     const size_t k)
    {
      const ssef min4 = min(v0,v1,v2,v3);
      const ssef max4 = max(v0,v1,v2,v3);
      const ssef maxR = shuffle<3,3,3,3>(max4);
      if (movemask((min4 > maxR) | (max4 < -maxR)) & 3) return false;
      if (min4[2] > ray8.tfar[k]) return false;
      return true;
    }

    //__noinline
    __forceinline
    bool intersectSegment1(const LinearSpace3<vec3fa> &space,
                           const vec3fa &org,
                           const Hairlet *hl, //const BVH4* bvh, 
                           const uint32 startVertex,
                           const size_t k,
                           Ray8 &ray)
    {
#if 1
      STATS(numIsecs++);
      const Segment &seg = (const Segment &)hl->hg->vertex[startVertex];

      // const ssef orgv = (const ssef &)org;
      const ssef orgv = select(sseb(-1,-1,-1,0),(const ssef &)org,ssef(zero));
      // ssef orgv = (const ssef &)org; orgv[3] = 0.f;
      const ssef v0v = (const ssef &)seg.v0.pos - orgv;
      const ssef v1v = (const ssef &)seg.v1.pos - orgv;
      const ssef v2v = (const ssef &)seg.v2.pos - orgv;
      const ssef v3v = (const ssef &)seg.v3.pos - orgv;
      // now, have four control points (shifted by origin) in AoS.
      ssef vx4, vy4, vz4, vr4;
      transpose(v0v,v1v,v2v,v3v,vx4,vy4,vz4,vr4);
      // now, have the four control points in AoS format (4 x's together, etc)

      const ssef vu4 = madd(vx4,space.vx.x,madd(vy4,space.vx.y,vz4 * space.vx.z));
      const ssef vv4 = madd(vx4,space.vy.x,madd(vy4,space.vy.y,vz4 * space.vy.z));
      const ssef vt4 = madd(vx4,space.vz.x,madd(vy4,space.vz.y,vz4 * space.vz.z));
      // const ssef vu4 = vx4 * space.vx.x + vy4 * space.vx.y + vz4 * space.vx.z;
      // const ssef vv4 = vx4 * space.vy.x + vy4 * space.vy.y + vz4 * space.vy.z;
      // const ssef vt4 = vx4 * space.vz.x + vy4 * space.vz.y + vz4 * space.vz.z;

      // compute 8 line segments' start and end positions
      const avx4f v0(vu4[0],vv4[0],vt4[0],vr4[0]);
      const avx4f v1(vu4[1],vv4[1],vt4[1],vr4[1]);
      const avx4f v2(vu4[2],vv4[2],vt4[2],vr4[2]);
      const avx4f v3(vu4[3],vv4[3],vt4[3],vr4[3]);
      const avx4f p0
        = embree::coeff0[0] * v0 
        + embree::coeff0[1] * v1
        + embree::coeff0[2] * v2
        + embree::coeff0[3] * v3;
      const avx4f p1
        = embree::coeff1[0] * v0 
        + embree::coeff1[1] * v1
        + embree::coeff1[2] * v2
        + embree::coeff1[3] * v3;

      /* approximative intersection with cone */
      const avx4f v = p1-p0;
      const avx4f w = -p0;
      const avxf d0 = w.x*v.x + w.y*v.y;
      const avxf d1 = v.x*v.x + v.y*v.y;
      const avxf u = clamp(d0*rcp(d1),avxf(zero),avxf(one));
      const avx4f p = p0 + u*v;
      const avxf t = p.z;

      const avxf d2 = p.x*p.x + p.y*p.y; 
      const avxf r = p.w; //max(p.w,ray.org.w+ray.dir.w*t);
      const avxf r2 = r*r;
      avxb valid = (d2 <= r2) & (avxf(ray.tnear[k]) < t) & (t < avxf(ray.tfar[k]));

      if (unlikely(none(valid))) {
        return false;
      }
      const float one_over_8 = 1.0f/8.0f;
      size_t i = select_min(valid,t);
      ray.tfar[k] = t[i];
      (int32&)ray.u[k] = i; //(float(i)+u[i])*one_over_8;
      // ray.u[k] = (float(i)+u[i])*one_over_8;
      ray.Ng.x[k] = v.x[i];
      ray.Ng.y[k] = v.y[i];
      ray.Ng.z[k] = v.z[i];
      return true;
#else
      STATS(numIsecs++);
      const Segment &seg = (const Segment &)hl->hg->vertex[startVertex];
      // const ssef _org = (const ssef &)org;
      
      const vec3fa _v0 = (const vec3fa &)seg.v0.pos - org;
      const vec3fa _v1 = (const vec3fa &)seg.v1.pos - org;
      const vec3fa _v2 = (const vec3fa &)seg.v2.pos - org;
      const vec3fa _v3 = (const vec3fa &)seg.v3.pos - org;
      
      const vec4f __v0(dot(_v0,space.vx),dot(_v0,space.vy),dot(_v0,space.vz),seg.v0.radius);
      const vec4f __v1(dot(_v1,space.vx),dot(_v1,space.vy),dot(_v1,space.vz),seg.v1.radius);
      const vec4f __v2(dot(_v2,space.vx),dot(_v2,space.vy),dot(_v2,space.vz),seg.v2.radius);
      const vec4f __v3(dot(_v3,space.vx),dot(_v3,space.vy),dot(_v3,space.vz),seg.v3.radius);

      ssef v0 = (const ssef &)__v0;
      ssef v1 = (const ssef &)__v1;
      ssef v2 = (const ssef &)__v2;
      ssef v3 = (const ssef &)__v3;

      int depth = 2;
      SegStack stack[20];
      SegStack *stackPtr = stack;
      const ssef half = 0.5f;
      bool foundHit = false;
      goto start;
      while (1) {
        if (stackPtr <= stack) break;
        --stackPtr;
        v0 = stackPtr->v0;
        v1 = stackPtr->v1;
        v2 = stackPtr->v2;
        v3 = stackPtr->v3;
        depth = stackPtr->depth;
      start:
        if (!segmentTest(v0,v1,v2,v3,ray,k))
          continue;

        const ssef v10 = half*(v0+v1);
        const ssef v11 = half*(v1+v2);
        const ssef v12 = half*(v2+v3);

        const ssef v20 = half*(v10+v11);
        const ssef v21 = half*(v11+v12);

        const ssef v30 = half*(v20+v21);
        if (depth == 0) {
          foundHit |= finalSegmentTest(v0,v10,v20,v30,ray,k);
          foundHit |= finalSegmentTest(v30,v21,v12,v3,ray,k);
          //   ray.primID[k] = (long(startVertex)*13)>>4;
          //   ray.geomID[k] = (long(startVertex)*13)>>4;

          //   vec3f N
          //     = space.vx * ray.Ng.x[k] 
          //     + space.vy * ray.Ng.y[k] 
          //     + space.vz * ray.Ng.z[k];
          // }
        } else {
          stackPtr[0].v0 = v3;
          stackPtr[0].v1 = v12;
          stackPtr[0].v2 = v21;
          stackPtr[0].v3 = v30;
          stackPtr[0].depth = depth-1;
          
          stackPtr ++;

          v0 = v0;
          v1 = v10;
          v2 = v20;
          v3 = v30;
          depth = depth-1;
          goto start;
        }
      }
      return foundHit;
#endif
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
                        // const avx3i &nearXYZ,
                        const embree::LinearSpace3<avx3f> &ray_space
                        )
    {
      STATS(numHairletTravs++);
      static const int stackSizeSingle = 80;
      typedef int64 NodeRef;

      StackItemInt32<NodeRef> stack[stackSizeSingle];  //!< stack of nodes 
      StackItemInt32<NodeRef>* stackPtr = stack; // + 1;        //!< current stack pointer
      StackItemInt32<NodeRef>* stackEnd = stack + stackSizeSingle;

#if OFFSETS
      const size_t nearOfsX = ray_dir.x[k] > 0.f ? 0 : 5;
      const size_t nearOfsY = ray_dir.y[k] > 0.f ? 0 : 5;
      const size_t nearOfsZ = ray_dir.z[k] > 0.f ? 0 : 5;
#endif

      // data for decoding
#ifdef OSPRAY_TARGET_AVX2
      const ssei shift(0,8,16,24);
      const ssei bitMask(255);
#else
      const ssei shlScale((1<<24),(1<<16),(1<<8),1);
#endif


      uint mailbox[32];
      ((avxi*)mailbox)[0] = avxi(-1);
      ((avxi*)mailbox)[1] = avxi(-1);
      ((avxi*)mailbox)[2] = avxi(-1);
      ((avxi*)mailbox)[3] = avxi(-1);

      // bvh4 traversal...
      const sse3f org(ray_org.x[k], ray_org.y[k], ray_org.z[k]);
      const sse3f rdir(ray_rdir.x[k], ray_rdir.y[k], ray_rdir.z[k]);
      LinearSpace3<vec3fa> raySpace(vec3fa(ray_space.vx.x[k],
                                           ray_space.vx.y[k],
                                           ray_space.vx.z[k]),
                                    vec3fa(ray_space.vy.x[k],
                                           ray_space.vy.y[k],
                                           ray_space.vy.z[k]),
                                    vec3fa(ray_space.vz.x[k],
                                           ray_space.vz.y[k],
                                           ray_space.vz.z[k]));

      // const vec3fa dir_k(ray.dir.x[k],ray.dir.y[k],ray.dir.z[k]);
      const vec3fa org_k(ray.org.x[k],ray.org.y[k],ray.org.z[k]);

      const sse3f org_rdir(org*rdir);
      const ssef  ray_t0(ray.tnear[k]);
      ssef  ray_t1(ray.tfar[k]);
      
      const ssef hl_bounds_min = *(ssef*)&hl->bounds.lower;
      const ssef hl_bounds_max = *(ssef*)&hl->bounds.upper;
      const ssef hl_bounds_scale = (hl_bounds_max-hl_bounds_min)*ssef(1.f/255.f);

      const ssef hl_bounds_scale_x = ssef(hl_bounds_scale[0]);
      const ssef hl_bounds_scale_y = ssef(hl_bounds_scale[1]);
      const ssef hl_bounds_scale_z = ssef(hl_bounds_scale[2]);
      
      const ssef hl_bounds_min_x = ssef(hl_bounds_min[0]);
      const ssef hl_bounds_min_y = ssef(hl_bounds_min[1]);
      const ssef hl_bounds_min_z = ssef(hl_bounds_min[2]);

      //      --stackPtr;
      NodeRef cur = NodeRef(0); //stackPtr->ptr);
      goto firstStepStartsHere;

      /* pop loop */
      while (true) {
      pop:

        /*! pop next node */
        if (unlikely(stackPtr == stack)) break;
        stackPtr--;
        cur = NodeRef(stackPtr->ptr);

        /*! if popped node is too far, pop next one */
        if (unlikely(*(float*)&stackPtr->dist > ray.tfar[k]))//ray.tfar[k]))
          continue;
        
        /* downtraversal loop */
        while (true) {
          /*! stop if we found a leaf */
          if (unlikely(cur >= 0)) break;
        firstStepStartsHere:

          const Hairlet::QuadNode *node = &hl->node[-cur];
#if OFFSETS
          // do not use - something's broken!
          const uint32 *nodeData = (const uint32*)&node->lo_x[0];
          const ssei _node_nr_x = ssei(nodeData[0+nearOfsX]);
          const ssei _node_nr_y = ssei(nodeData[1+nearOfsY]);
          const ssei _node_nr_z = ssei(nodeData[2+nearOfsZ]);
          const ssei _node_fr_x = ssei(nodeData[5-nearOfsX]);
          const ssei _node_fr_y = ssei(nodeData[6-nearOfsY]);
          const ssei _node_fr_z = ssei(nodeData[7-nearOfsZ]);

#if OSPRAY_TARGET_AVX2
          const ssei node_nr_x 
            = ssei(_mm_srav_epi32(_node_nr_x,shift)) & bitMask;
          const ssei node_nr_y 
            = ssei(_mm_srav_epi32(_node_nr_y,shift)) & bitMask;
          const ssei node_nr_z
            = ssei(_mm_srav_epi32(_node_nr_z,shift)) & bitMask;
          const ssei node_fr_x 
            = ssei(_mm_srav_epi32(_node_fr_x,shift)) & bitMask;
          const ssei node_fr_y 
            = ssei(_mm_srav_epi32(_node_fr_y,shift)) & bitMask;
          const ssei node_fr_z
            = ssei(_mm_srav_epi32(_node_fr_z,shift)) & bitMask;
#else
          const ssei node_nr_x
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_nr_x,shlScale),24));
          const ssei node_nr_y
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_nr_y,shlScale),24));
          const ssei node_nr_z
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_nr_z,shlScale),24));
          const ssei node_fr_x
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_fr_x,shlScale),24));
          const ssei node_fr_y
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_fr_y,shlScale),24));
          const ssei node_fr_z
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_fr_z,shlScale),24));
#endif
          const sseb box_valid = (node_nr_x <= node_fr_x);

          const ssef world_nr_x
            = madd(ssef(node_nr_x), hl_bounds_scale_x, hl_bounds_min_x);//-maxRad;
          const ssef world_nr_y
            = madd(ssef(node_nr_y), hl_bounds_scale_y, hl_bounds_min_y);//-maxRad;
          const ssef world_nr_z
            = madd(ssef(node_nr_z), hl_bounds_scale_z, hl_bounds_min_z);//-maxRad;
          const ssef world_fr_x
            = madd(ssef(node_fr_x), hl_bounds_scale_x, hl_bounds_min_x);//+maxRad;
          const ssef world_fr_y
            = madd(ssef(node_fr_y), hl_bounds_scale_y, hl_bounds_min_y);//+maxRad;
          const ssef world_fr_z
            = madd(ssef(node_fr_z), hl_bounds_scale_z, hl_bounds_min_z);//+maxRad;

          // PRINT(maxRad);
          const ssef t_nr_x = msub(world_nr_x,rdir.x,org_rdir.x);
          const ssef t_nr_y = msub(world_nr_y,rdir.y,org_rdir.y);
          const ssef t_nr_z = msub(world_nr_z,rdir.z,org_rdir.z);
          const ssef t_fr_x = msub(world_fr_x,rdir.x,org_rdir.x);
          const ssef t_fr_y = msub(world_fr_y,rdir.y,org_rdir.y);
          const ssef t_fr_z = msub(world_fr_z,rdir.z,org_rdir.z);
          
          const ssef t0 = max(t_nr_x,t_nr_y,t_nr_z,ray_t0);
          const ssef t1 = min(t_fr_x,t_fr_y,t_fr_z,ray_t1);


#else
          // non-offset based code - working!
          const ssei _node_lo_x = ssei((uint32&)node->lo_x[0]);
          const ssei _node_lo_y = ssei((uint32&)node->lo_y[0]);
          const ssei _node_lo_z = ssei((uint32&)node->lo_z[0]);
          const ssei _node_hi_x = ssei((uint32&)node->hi_x[0]);
          const ssei _node_hi_y = ssei((uint32&)node->hi_y[0]);
          const ssei _node_hi_z = ssei((uint32&)node->hi_z[0]);

#if OSPRAY_TARGET_AVX2
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
#else
          const ssei node_lo_x
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_lo_x,shlScale),24));
          const ssei node_lo_y
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_lo_y,shlScale),24));
          const ssei node_lo_z
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_lo_z,shlScale),24));
          const ssei node_hi_x
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_hi_x,shlScale),24));
          const ssei node_hi_y
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_hi_y,shlScale),24));
          const ssei node_hi_z
            = ssei(_mm_srli_epi32(_mm_mullo_epi32(_node_hi_z,shlScale),24));
#endif
          const sseb box_valid = (node_lo_x <= node_hi_x);
          // const sseb box_valid
          //   = (node_lo_x <= node_hi_x)
          //   & (node_lo_y <= node_hi_y)
          //   & (node_lo_z <= node_hi_z)
          //   ;
          
          // currently we're adding the max radius to each box since
          // the boxes are built over the axis, not the tubes. may
          // change that soon.
          // const ssef maxRad(hl->maxRadius);

          // ideally, bake that into a ray transformation ...
          const ssef world_lo_x
            = madd(ssef(node_lo_x), hl_bounds_scale_x, hl_bounds_min_x);//-maxRad;
          const ssef world_lo_y
            = madd(ssef(node_lo_y), hl_bounds_scale_y, hl_bounds_min_y);//-maxRad;
          const ssef world_lo_z
            = madd(ssef(node_lo_z), hl_bounds_scale_z, hl_bounds_min_z);//-maxRad;
          const ssef world_hi_x
            = madd(ssef(node_hi_x), hl_bounds_scale_x, hl_bounds_min_x);//+maxRad;
          const ssef world_hi_y
            = madd(ssef(node_hi_y), hl_bounds_scale_y, hl_bounds_min_y);//+maxRad;
          const ssef world_hi_z
            = madd(ssef(node_hi_z), hl_bounds_scale_z, hl_bounds_min_z);//+maxRad;

          // PRINT(maxRad);
          const ssef t_lo_x = msub(world_lo_x,rdir.x,org_rdir.x);
          const ssef t_lo_y = msub(world_lo_y,rdir.y,org_rdir.y);
          const ssef t_lo_z = msub(world_lo_z,rdir.z,org_rdir.z);
          const ssef t_hi_x = msub(world_hi_x,rdir.x,org_rdir.x);
          const ssef t_hi_y = msub(world_hi_y,rdir.y,org_rdir.y);
          const ssef t_hi_z = msub(world_hi_z,rdir.z,org_rdir.z);
          
          const ssef t0_x = min(t_lo_x,t_hi_x);
          const ssef t0_y = min(t_lo_y,t_hi_y);
          const ssef t0_z = min(t_lo_z,t_hi_z);
          const ssef t1_x = max(t_lo_x,t_hi_x);
          const ssef t1_y = max(t_lo_y,t_hi_y);
          const ssef t1_z = max(t_lo_z,t_hi_z);
          
          const ssef t0 = max(t0_x,t0_y,t0_z,ray_t0);
          const ssef t1 = min(t1_x,t1_y,t1_z,ray_t1);
#endif
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
          stackPtr->ptr = c0; stackPtr->dist = d0; stackPtr++;
          stackPtr->ptr = c1; stackPtr->dist = d1; stackPtr++;
          
          /*! three children are hit, push all onto stack and sort 3
              stack items, continue with closest child */
          r = __bscf(mask);
          NodeRef c = (int16&)node->child[r]; unsigned int d = ((unsigned int*)&t0)[r]; 
          stackPtr->ptr = c; stackPtr->dist = d; stackPtr++;
          
          if (likely(mask == 0)) {
            sort(stackPtr[-1], stackPtr[-2], stackPtr[-3]);
            cur = (NodeRef)stackPtr[-1].ptr; stackPtr--;
            continue;
          }
          

          /*! four children are hit, push all onto stack and sort 4
              stack items, continue with closest child */
          r = __bscf(mask);
          c = (int16&)node->child[r]; d = ((unsigned int*)&t0)[r]; 
          stackPtr->ptr = c; stackPtr->dist = d; stackPtr++;
          sort(stackPtr[-1], stackPtr[-2], stackPtr[-3], stackPtr[-4]);
          cur = (NodeRef)stackPtr[-1].ptr; stackPtr--;
        }
        
        // /*! this is a leaf node */
        while (1) {
          uint segID = hl->leaf[cur].hairID;// hl->segStart[;
          
          // mailboxing
          uint *mb = &mailbox[segID%32];
          if (*mb != segID) {
            const uint32 startVertex = segID; //hl->segStart[segID];
            *mb = segID;
            if (intersectSegment1(raySpace,org_k,hl,startVertex,k,ray)) {
              ray.primID[k] = startVertex; //(long(segID)*13)>>4;
              ray.geomID[k] = 0; //(long(segID)*13)>>4;

              // vec3fa N = vec3fa((ray.Ng.x[k],ray.Ng.y[k],ray.Ng.z[k]));
              vec3fa N 
                = ray.Ng.x[k] * raySpace.vx
                + ray.Ng.y[k] * raySpace.vy
                + ray.Ng.z[k] * raySpace.vz;
              ray.Ng.x[k] = N.x;
              ray.Ng.y[k] = N.y;
              ray.Ng.z[k] = N.z;

              ray_t1 = ray.tfar[k];
            }
          }
          if (hl->leaf[cur].eol) break;
          ++cur;
        }
      }
// #endif
    }


    void intersectHairlet(const void* _valid,  /*!< pointer to valid mask */
                          void* ptr,          /*!< pointer to user data */
                          embree::Ray8 &ray,       /*!< ray packet to intersect */
                          size_t item         /*!< item to intersect */)
    {
      avxb valid = *(avxb*)_valid;

      Hairlet *hl = ((HairBVH*)ptr)->hairlet[item];
      const avx3f ray_org = ray.org, ray_dir = ray.dir;
      // avxf ray_tnear = ray.tnear, ray.tfar  = ray.tfar;
      const avx3f rdir = rcp_safe(ray_dir);
      const avx3f // org(ray_org), 
        org_rdir = ray_org * rdir;

      const embree::LinearSpace3<avx3f> raySpace = embree::frame(ray_dir);

      /* compute near/far per ray */
      // iw: not yet using this optimization ...
      // avx3i nearXYZ;
      // nearXYZ.x = select(rdir.x >= 0.0f,
      //                    avxi(0*(int)sizeof(ssef)),
      //                    avxi(1*(int)sizeof(ssef)));
      // nearXYZ.y = select(rdir.y >= 0.0f,
      //                    avxi(2*(int)sizeof(ssef)),
      //                    avxi(3*(int)sizeof(ssef)));
      // nearXYZ.z = select(rdir.z >= 0.0f,
      //                    avxi(4*(int)sizeof(ssef)),
      //                    avxi(5*(int)sizeof(ssef)));

      size_t bits = movemask(valid);
      if (bits==0) return;
      for (size_t i=__bsf(bits); bits!=0; bits=__btc(bits,i), i=__bsf(bits)) {
        hairIntersect1(item,hl,
                       i, ray, ray_org, ray_dir, rdir, //ray_tnear, ray_tfar, 
                       // nearXYZ,
                       raySpace);
        // ray_tfar = ray.tfar;
      }
    }
  }
}
