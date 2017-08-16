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

#pragma once

#include "AMRAccel.h"

#define DBG(a) /**/

namespace ospray {
  namespace amr {
    using std::cout;
    using std::endl;
    
    inline vec3f floor(const vec3f &v)
    {
      return vec3f(floorf(v.x),
                   floorf(v.y), 
                   floorf(v.z));
    }
    inline vec3i clamp(const box3i &box, const vec3i &P)
    { return max(box.lower,min(box.upper,P)); }
    inline vec3f clamp(const box3f &box, const vec3f &P)
    { return max(box.lower,min(box.upper,P)); }
    
    struct AMRSampler {
      AMRSampler(const AMRAccel *accel) : accel(accel) {}
      
      /*! a *logical* cell reference; this can be a cell that exists
          OR that is virtual */
      struct CellRef {
        inline CellRef() {};
        inline CellRef(const CellRef &other) : coord(other.coord), level(other.level) {}
        inline CellRef(const vec3i &coord, int level) : coord(coord), level(level) {}
        inline CellRef &operator=(const CellRef &other) {
          coord = other.coord;
          level = other.level;
          return *this;
        }
        static inline CellRef logicalCellOnLevelOf(const AMRData::Brick *brick,
                                                   const vec3f &P);
        static inline CellRef logicalCellOn(const AMRAccel::Level &level,
                                            const vec3f &P);
        static inline CellRef logicalDualCellOn(const AMRAccel::Level &level,
                                                const vec3f &P);
        
        //! integer coords on given level
        vec3i coord;
        
        //! level
        int level;
      };

      /*! an actual cell that actually exists in the input */
      struct ActualCell {
        inline ActualCell() {};
        inline ActualCell(const ActualCell &other)
          : logical(other.logical), actual(other.actual), value(other.value)
        {}
        inline ActualCell(const CellRef &logical,
                          const CellRef &actual,
                          float value)
          : logical(logical), actual(actual), value(value)
        {}
        static inline ActualCell getActual(const AMRData::Brick *brick,
                                           const CellRef &logical);
        //! logical coordinates requested for this cell
        CellRef logical;
        //! actual coordinates found for this cell
        CellRef actual;
        //! value for the _actual_ coords
        float value;
      };
      
      struct DualCell {
        inline float lerp() const;
        inline float lerpOpacity() const;
        
        CellRef logical;
        ActualCell corner[8];
        /*! the interpolation weights for the poit P we used to find this cell */
        vec3f lerpWeights;
      };
     
      /*! find actual leaf cell containing point P */
      inline ActualCell findLeafCell(const vec3f &P) const;
      /*! find actual cell on given level, or closest existing
          equivalent if it doesn't */
      inline ActualCell findCellOn(const AMRAccel::Level &level, const vec3f &P) const;
      /*! find dual cell (including all actual corner cells) */
      inline DualCell findDualCell(const AMRAccel::Level &level, const vec3f &P) const;

      /*! compute 3d position of the dual cell's given corner */
      inline vec3f cornerPos(const DualCell &D, const vec3i cornerID) const;

      const AMRAccel *accel;
    };

    inline std::ostream &operator<<(std::ostream &o, const AMRSampler::CellRef &c)
    { o << "(coord=" << c.coord << ", l=" <<c.level<<")"; return o; }
    
    inline std::ostream &operator<<(std::ostream &o, const AMRSampler::ActualCell &c)
    { o << "Cell(log=" << c.logical << ", act=" << c.actual << ", val=" << c.value << ")"; return o; }
    
    // -------------------------------------------------------
    /*! compute 3d position of the dual cell's given corner */
    inline vec3f AMRSampler::cornerPos(const DualCell &dual, const vec3i cornerID) const
    {
      const vec3i cornerCellCoord = dual.logical.coord + cornerID;
      const AMRAccel::Level &level = accel->level[dual.logical.level];
      return level.halfCellWidth + level.cellWidth * vec3f(cornerCellCoord);
    }
    
    // -------------------------------------------------------
    inline AMRSampler::CellRef
    AMRSampler::CellRef::logicalCellOnLevelOf(const AMRData::Brick *brick,
                                                 const vec3f &P)
    {
      const float scale = brick->gridToWorldScale;
      const vec3i coord = vec3i(floor(P * scale));
      return AMRSampler::CellRef(coord,brick->level);
    }
    
    // -------------------------------------------------------
    inline AMRSampler::CellRef
    AMRSampler::CellRef::logicalCellOn(const AMRAccel::Level &level,
                                          const vec3f &P)
    {
      const vec3f xformed = P * level.rcpCellWidth;
      const vec3i coord   = vec3i(floor(xformed));
      return AMRSampler::CellRef(coord,level.level);
    }
    
    // -------------------------------------------------------
    inline AMRSampler::CellRef
    AMRSampler::CellRef::logicalDualCellOn(const AMRAccel::Level &level,
                                              const vec3f &P)
    {
      const vec3f xformed = (P - level.halfCellWidth) * level.rcpCellWidth;
      const vec3i coord   = vec3i(floor(xformed));
      return AMRSampler::CellRef(coord,level.level);
    }
    
    // -------------------------------------------------------
    inline AMRSampler::ActualCell
    AMRSampler::ActualCell::getActual(const AMRData::Brick *brick,
                                         const CellRef &logical)
    {
      assert(logical.level == brick->level);
      // PING;
      // PRINT(logical);
      const CellRef actual(clamp(brick->box,logical.coord),brick->level);
      // PRINT(brick->box);
      // PRINT(actual);
      const vec3i   localCoord = actual.coord - brick->box.lower;
      const int     localIndex = localCoord.x+brick->dims.x*(localCoord.y+brick->dims.y*(localCoord.z));
      const float   value      = brick->value[localIndex];
      return ActualCell(logical,actual,value);
    }

    // -------------------------------------------------------
    /*! find actual leaf cell containing point P */
    inline AMRSampler::ActualCell
    AMRSampler::findLeafCell(const vec3f &P) const
    {
      // PING;
      // PRINT(P);
      ActualCell C;
      const AMRAccel::Node *node = &accel->node[0];
      assert(node);
      while (!node->isLeaf()) {
        node = &accel->node[node->ofs + ((P[node->dim] >= node->pos)?1:0)];
        assert(node);
      }
      const AMRAccel::Leaf *leaf = &accel->leaf[node->ofs];
      // for (int i=0;i<node->numItems;i++) {
      //   PRINT(leaf->brickList[i]->level);
      //   PRINT(leaf->brickList[i]->box);
      //   PRINT(leaf->brickList[i]->value[0]);
      // }
        
      assert(leaf);
      const AMRData::Brick *brick = leaf->brickList[0];
      assert(brick);
      // PRINT(brick);
      // PRINT(brick->box);
      const CellRef logical = CellRef::logicalCellOnLevelOf(brick,P);
      C = ActualCell::getActual(brick,logical);
      // PRINT(logical);
      // PRINT(C);
      
      return C;
    }
    
    // -------------------------------------------------------
    /*! find dual cell (including all actual corner cells) */
    inline AMRSampler::DualCell
    AMRSampler::findDualCell(const AMRAccel::Level &level, const vec3f &P) const
    {
      DualCell D;
      D.logical = CellRef::logicalDualCellOn(level,P);
      const vec3f lo = cornerPos(D,vec3i(0,0,0));
      const vec3f hi = cornerPos(D,vec3i(1,1,1));
      // PING;
      // PRINT(lo);
      // PRINT(hi);
      D.corner[0] = findCellOn(level,vec3f(lo.x,lo.y,lo.z));
      D.corner[1] = findCellOn(level,vec3f(hi.x,lo.y,lo.z));
      D.corner[2] = findCellOn(level,vec3f(lo.x,hi.y,lo.z));
      D.corner[3] = findCellOn(level,vec3f(hi.x,hi.y,lo.z));
      D.corner[4] = findCellOn(level,vec3f(lo.x,lo.y,hi.z));
      D.corner[5] = findCellOn(level,vec3f(hi.x,lo.y,hi.z));
      D.corner[6] = findCellOn(level,vec3f(lo.x,hi.y,hi.z));
      D.corner[7] = findCellOn(level,vec3f(hi.x,hi.y,hi.z));
      D.lerpWeights = (clamp(box3f(lo,hi),P)-lo) * level.rcpCellWidth;

      // PRINT(D.logical.coord);
      // PRINT(D.logical.level);
      // PRINT(lo);
      // PRINT(hi);
      // PRINT(D.corner[0].logical);
      // PRINT(D.corner[1].logical);
      // PRINT(D.corner[2].logical);
      // PRINT(D.corner[3].logical);
      // PRINT(D.corner[4].logical);
      // PRINT(D.corner[5].logical);
      // PRINT(D.corner[6].logical);
      // PRINT(D.corner[7].logical);
      // PRINT(D.corner[0].actual);
      // PRINT(D.corner[1].actual);
      // PRINT(D.corner[2].actual);
      // PRINT(D.corner[3].actual);
      // PRINT(D.corner[4].actual);
      // PRINT(D.corner[5].actual);
      // PRINT(D.corner[6].actual);
      // PRINT(D.corner[7].actual);
      // PRINT(D.corner[0].value);
      // PRINT(D.corner[1].value);
      // PRINT(D.corner[2].value);
      // PRINT(D.corner[3].value);
      // PRINT(D.corner[4].value);
      // PRINT(D.corner[5].value);
      // PRINT(D.corner[6].value);
      // PRINT(D.corner[7].value);
      // PRINT(D.lerpWeights);
      return D;
    }
      
    // -------------------------------------------------------
    /*! find actual cell on given level, or closest existing
      equivalent if it doesn't */
    inline AMRSampler::ActualCell
    AMRSampler::findCellOn(const AMRAccel::Level &level, const vec3f &P) const
    {
      // PING;
      // PRINT(P);
      
      ActualCell C;
      const AMRAccel::Node *node = &accel->node[0];
      while (!node->isLeaf()) {
        node = &accel->node[node->ofs + ((P[node->dim] >= node->pos)?1:0)];
      }
      const AMRAccel::Leaf *leaf = &accel->leaf[node->ofs];
      const AMRData::Brick **brickList = leaf->brickList;
      // PRINT(brickList[0]->level);
      while ((*brickList)->level > level.level)
        ++brickList;
      const AMRData::Brick *brick = *brickList;
      const CellRef logical = CellRef::logicalCellOnLevelOf(brick,P);
      C = ActualCell::getActual(brick,logical);
      return C;
    }
    

    inline float AMRSampler::DualCell::lerp() const
    {
      const float v000 = corner[0].value;
      const float v001 = corner[1].value;
      const float v010 = corner[2].value;
      const float v011 = corner[3].value;
      const float v100 = corner[4].value;
      const float v101 = corner[5].value;
      const float v110 = corner[6].value;
      const float v111 = corner[7].value;

      const float v00  = v000 + lerpWeights.x * (v001-v000);
      const float v01  = v010 + lerpWeights.x * (v011-v010);
      const float v10  = v100 + lerpWeights.x * (v101-v100);
      const float v11  = v110 + lerpWeights.x * (v111-v110);

      const float v0  = v00 + lerpWeights.y * (v01-v00);
      const float v1  = v10 + lerpWeights.y * (v11-v10);

      const float v = v0 + lerpWeights.z * (v1-v0);

      return v;
    }
    
    inline float AMRSampler::DualCell::lerpOpacity() const
    {
      const float v000 = (corner[0].actual.level == corner[0].logical.level) ? 1.f:0.f;
      const float v001 = (corner[1].actual.level == corner[1].logical.level) ? 1.f:0.f;
      const float v010 = (corner[2].actual.level == corner[2].logical.level) ? 1.f:0.f;
      const float v011 = (corner[3].actual.level == corner[3].logical.level) ? 1.f:0.f;
      const float v100 = (corner[4].actual.level == corner[4].logical.level) ? 1.f:0.f;
      const float v101 = (corner[5].actual.level == corner[5].logical.level) ? 1.f:0.f;
      const float v110 = (corner[6].actual.level == corner[6].logical.level) ? 1.f:0.f;
      const float v111 = (corner[7].actual.level == corner[7].logical.level) ? 1.f:0.f;

      const float v00  = v000 + lerpWeights.x * (v001-v000);
      const float v01  = v010 + lerpWeights.x * (v011-v010);
      const float v10  = v100 + lerpWeights.x * (v101-v100);
      const float v11  = v110 + lerpWeights.x * (v111-v110);

      const float v0  = v00 + lerpWeights.y * (v01-v00);
      const float v1  = v10 + lerpWeights.y * (v11-v10);

      const float v = v0 + lerpWeights.z * (v1-v0);

      return v;
    }
        


    struct Sampler_nearest : public AMRSampler {
      Sampler_nearest(const AMRAccel *accel) : AMRSampler(accel) {};
      
      /*! compute scalar field value at given location */
      inline float sample(const vec3f &P)
      {
        const ActualCell C = findLeafCell(P);
        return C.value;
      }

            /*! compute scalar field value at given location */
      inline float sampleLevel(const vec3f &P, float& width)
      {
        const ActualCell C = findLeafCell(P);
        width = float(C.actual.level);
        return C.value;
      }

      /*! compute gradient at given location */
      inline vec3f gradient(const vec3f &P)
      { throw std::runtime_error("not implemented"); }
    };


    
    struct Sampler_finestLevel : public AMRSampler {
      Sampler_finestLevel(const AMRAccel *accel) : AMRSampler(accel) {};
      
      /*! compute scalar field value at given location */
      inline float sample(const vec3f &P)
      {
        const DualCell D = findDualCell(accel->finestLevel(),P);
        return D.lerp();
      }

      /*! compute scalar field value at given location */
      inline float sampleLevel(const vec3f &P, float& width)
      {
        const DualCell D = findDualCell(accel->finestLevel(),P);
        const ActualCell C = findLeafCell(P);
        width = float(C.actual.level);
        return D.lerp();
      }

      /*! compute gradient at given location */
      inline vec3f gradient(const vec3f &P)
      { throw std::runtime_error("not implemented"); }
    };
    
    struct Sampler_currentLevel : public AMRSampler {
      Sampler_currentLevel(const AMRAccel *accel) : AMRSampler(accel) {};
      
      /*! compute scalar field value at given location */
      inline float sample(const vec3f &P)
      {
        const ActualCell C = findLeafCell(P);
        const DualCell D = findDualCell(accel->level[C.actual.level],P);
        float f = D.lerp();
        return f;
      }

      /*! compute scalar field value at given location */
      inline float sampleLevel(const vec3f &P, float& width)
      {
        const ActualCell C = findLeafCell(P);
        width = float(C.actual.level);
        const DualCell D = findDualCell(accel->level[C.actual.level],P);
        float f = D.lerp();
        return f;
      }

      /*! compute gradient at given location */
      inline vec3f gradient(const vec3f &P)
      { throw std::runtime_error("not implemented"); }
    };
    
    struct Sampler_blendLevels : public AMRSampler {
      Sampler_blendLevels(const AMRAccel *accel) : AMRSampler(accel) {};
      
      /*! compute scalar field value at given location */
      inline float sample(const vec3f &P)
      {
        // const ActualCell C = findLeafCell(P);
        // return C.value;
        float result = 0.f;
        for (int l=0;l<accel->level.size();l++) {
          const DualCell D = findDualCell(accel->level[l],P);
          const float f = D.lerp();
          const float a = D.lerpOpacity();

          if (a == 0.f)
            break;
          result = a*f + (1.f-a)*result;
        }
        return result;
      }

      /*! compute scalar field value at given location */
      inline float sampleLevel(const vec3f &P, float& width)
      {
        const ActualCell C = findLeafCell(P);
        width = float(C.actual.level);
        // return C.value;
        float result = 0.f;
        for (int l=0;l<accel->level.size();l++) {
          const DualCell D = findDualCell(accel->level[l],P);
          const float f = D.lerp();
          const float a = D.lerpOpacity();

          if (a == 0.f)
            break;
          result = a*f + (1.f-a)*result;
        }
        return result;
      }

      /*! compute gradient at given location */
      inline vec3f gradient(const vec3f &P)
      { throw std::runtime_error("not implemented"); }
    };
    


    
  } // ::ospray::amr
} // ::ospray


