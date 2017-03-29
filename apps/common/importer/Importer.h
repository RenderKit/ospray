// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "ospcommon/box.h"
// ospray
#include "ospray/common/OSPCommon.h"
// std
#include <vector>

#ifdef _WIN32
#  ifdef ospray_importer_EXPORTS
#    define OSPIMPORTER_INTERFACE __declspec(dllexport)
#  else
#    define OSPIMPORTER_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPIMPORTER_INTERFACE
#endif

#define OSPRAY_APPS_IMPORTER_ENABLE_PRINTS 0

namespace ospray {
  namespace importer {
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

    //! Convenient wrapper that will do the template dispatch for you based on the voxelType passed
    inline void extendVoxelRange(ospcommon::vec2f &voxelRange
        , const OSPDataType voxelType
        , const unsigned char *voxels
        , const size_t numVoxels
        )
    {
      switch (voxelType) {
        case OSP_UCHAR:
          extendVoxelRange(voxelRange, (unsigned char*)voxels, numVoxels);
          break;
        case OSP_SHORT:
          extendVoxelRange(voxelRange, (short*)voxels, numVoxels);
          break;
        case OSP_USHORT:
          extendVoxelRange(voxelRange, (unsigned short*)voxels, numVoxels);
          break;
        case OSP_FLOAT:
          extendVoxelRange(voxelRange, (float*)voxels, numVoxels);
          break;
        case OSP_DOUBLE:
          extendVoxelRange(voxelRange, (double*)voxels, numVoxels);
          break;
        default:
          std::cerr << "ERROR: unsupported voxel type " << stringForType(voxelType) << std::endl;
          exit(1);
      }
    }

    /*! handle for any sort of geometry object. the loader will create
        the geometry, create all the required data arrays, etc, and
        set the appropriate bounding box field - but will NOT commit
        it, yet */
    struct Geometry {
      box3f bounds;
      OSPGeometry handle {nullptr};
    };

    /*! abstraction for a volume object whose values are already
        set. the importer will create the volume, will set issue all
        the required ospSetRegion()'s on it, but will NOT commit
        it. */
    struct Volume {
      OSPVolume handle {nullptr};
      /* range of voxel values in the given volume. TODO: should be
         moved to a "ospray::range1f" type, not a implicit min/max
         stored in the x/y coordinates of a vec2f. */
      vec2f voxelRange {1e8, -1e8};
      //! desired sampling rate for this volume
      float samplingRate {1.f};
      //! world space bounding box of this volume
      box3f bounds;
      
      size_t fileOffset {0};
      vec3f gridOrigin {0};
      vec3f gridSpacing {1};
      vec3i subVolumeOffsets {0};
      vec3i subVolumeDimensions {-1};
      vec3i subVolumeSteps {-1};
      vec3i dimensions {-1};
      vec3f scaleFactor{1};

      std::string voxelType;
    };
    
    struct Group {
      std::vector<Geometry *> geometry;
      std::vector<Volume *>   volume;
    };

    OSPIMPORTER_INTERFACE Group *import(const std::string &fileName,
                  Group *existingGroupToAddTo=nullptr);
  } // ::ospray::importer

  //! Print an error message.
  inline void emitMessage(const std::string &kind, const std::string &message) 
  { std::cerr << "#osp:vv:importer " + kind + ": " + message + "." << std::endl; }
  
  //! Error checking.
  inline void exitOnCondition(bool condition, const std::string &message) 
  { if (!condition) return;  emitMessage("ERROR", message);  exit(1); }
  
  //! Warning condition.
  inline void warnOnCondition(bool condition, const std::string &message) 
  { if (!condition) return;  emitMessage("WARNING", message); }
  
#if 0
  //! Get the absolute file path.
  static std::string getFullFilePath(const std::string &filename)
  { 
#ifdef _WIN32
    //getfullpathname
    throw std::runtime_error("no realpath() under windows");
#else
    char *fullpath = realpath(filename.c_str(), nullptr);
    return(fullpath != nullptr ? fullpath : filename);
#endif
  }
#endif

} // ::ospray
