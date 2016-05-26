// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// ospray API
#include "ospray/ospray.h"
// ospcommon
#include "common/box.h"
// std
#include <vector>

namespace ospray {
  namespace vv_importer {
    using namespace ospcommon;

    /*! helper function to help build voxel ranges during parsing */
    template<typename T>
    inline void extendVoxelRange(ospcommon::vec2f &voxelRange, const T *voxel, size_t num)
    {
      for (size_t i=0;i<num;i++) {
        voxelRange.x = std::min(voxelRange.x,(float)voxel[i]);
        voxelRange.y = std::max(voxelRange.y,(float)voxel[i]);
      }
    }
    //! Convenient wrapper that will do the template dispatch for you based on the voxelSize passed
    //! voxelSize = 1 -> unsigned char
    //! voxelSize = 2 -> uint16_t
    //! voxelSize = 4 -> float
    //! voxelSize = 8 -> double
    inline void extendVoxelRange(ospcommon::vec2f &voxelRange, const size_t voxelSize, const unsigned char *voxels,
        const size_t numVoxels)
    {
      switch (voxelSize) {
        case sizeof(unsigned char):
          extendVoxelRange(voxelRange, (unsigned char*)voxels, numVoxels);
          break;
        case sizeof(float):
          extendVoxelRange(voxelRange, (float*)voxels, numVoxels);
          break;
        case sizeof(double):
          extendVoxelRange(voxelRange, (double*)voxels, numVoxels);
          break;
        default:
          std::cerr << "ERROR: unsupported voxel type with size " << voxelSize << std::endl;
          exit(1);
      }
    }

    /*! handle for any sort of geometry object. the loader will create
        the geometry, create all the required data arrays, etc, and
        set the appropriate bounding box field - but will NOT commit
        it, yet */
    struct Geometry {
      box3f bounds;
      OSPGeometry handle;
    };

    /*! abstraction for a volume object whose values are already
        set. the importer will create the volume, will set issue all
        the required ospSetRegion()'s on it, but will NOT commit
        it. */
    struct Volume {
      Volume() 
        : handle(NULL),
          voxelRange(vec2f(1e8,-1e8)),
          bounds(empty),
          gridOrigin(0),
          gridSpacing(vec3f(1)),
          subVolumeOffsets(vec3i(0)),
          subVolumeDimensions(vec3i(-1)),
          subVolumeSteps(vec3i(-1)),
          fileOffset(0),
          dimensions(-1),
          voxelType("")
      {}

      OSPVolume handle;
      /* range of voxel values in the given volume. TODO: should be
         moved to a "ospray::range1f" type, not a implicit min/max
         stored in the x/y coordinates of a vec2f. */
      vec2f voxelRange;
      //! desired sampling rate for this volume
      float samplingRate;
      //! world space bounding box of this volume
      box3f bounds;
      
      size_t fileOffset;
      vec3f gridOrigin;
      vec3f gridSpacing;
      vec3i subVolumeOffsets;
      vec3i subVolumeDimensions;
      vec3i subVolumeSteps;
      vec3i dimensions;

      std::string voxelType;
    };
    
    struct Group {
      std::vector<Geometry *> geometry;
      std::vector<Volume *>   volume;
    };

    Group *import(const std::string &fileName, Group *existingGroupToAddTo=NULL);
  }

  //! Print an error message.
  inline void emitMessage(const std::string &kind, const std::string &message) 
  { std::cerr << "#osp:vv:importer " + kind + ": " + message + "." << std::endl; }
  
  //! Error checking.
  inline void exitOnCondition(bool condition, const std::string &message) 
  { if (!condition) return;  emitMessage("ERROR", message);  exit(1); }
  
  //! Warning condition.
  inline void warnOnCondition(bool condition, const std::string &message) 
  { if (!condition) return;  emitMessage("WARNING", message); }
  
  //! Get the absolute file path.
  static std::string getFullFilePath(const std::string &filename)
  { 
#ifdef _WIN32
    //getfullpathname
    throw std::runtime_error("no realpath() under windows");
#else
    char *fullpath = realpath(filename.c_str(), NULL);  return(fullpath != NULL ? fullpath : filename); 
#endif
  }


}
