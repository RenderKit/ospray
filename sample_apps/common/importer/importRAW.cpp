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

// own
#include "Importer.h"
// ospcommon
#include "common/FileName.h"
// ospray api
#include "ospray/ospray.h"

namespace ospray {
  namespace importer {

    /*! helper function to help build voxel ranges during parsing */
    template<typename T>
    inline void extendVoxelRange(ospcommon::vec2f &voxelRange,
                                 const T *voxel,
                                 size_t num)
    {
      for (size_t i=0;i<num;i++) {
        voxelRange.x = std::min(voxelRange.x,(float)voxel[i]);
        voxelRange.y = std::max(voxelRange.y,(float)voxel[i]);
      }
    }

    void importVolumeRAW(const FileName &fileName, Volume *volume)
    {
      std::string filename = fileName.str();
      // Look for the volume data file at the given path.
      FILE *file = NULL;
      FileName fn = fileName;
      bool gzipped = fn.ext() == "gz";
      if (gzipped) {
        std::string cmd = "/usr/bin/gunzip -c "+filename;
        file = popen(cmd.c_str(),"r");
      } else {
        file = fopen(filename.c_str(),"rb");
      }
      //FILE *file = fopen(filename.c_str(), "rb");
      exitOnCondition(!file, "unable to open file '" + filename + "'");

      // Offset into the volume data file if any.
      if (volume->fileOffset > 0)
        fseek(file, volume->fileOffset, SEEK_SET);
      
      // Volume dimensions.
      ospcommon::vec3i volumeDimensions = volume->dimensions;  
      assert(volumeDimensions != vec3i(0) && volumeDimensions != vec3i(-1));
      
      // Voxel type string.
      const char *voxelType = volume->voxelType.c_str();  
      // Voxel size in bytes.
      size_t voxelSize;

      if (strcmp(voxelType, "uchar") == 0)
        voxelSize = sizeof(unsigned char);
      else if (strcmp(voxelType, "float") == 0)
        voxelSize = sizeof(float);
      else if (strcmp(voxelType, "double") == 0)
        voxelSize = sizeof(double);
      else
        exitOnCondition(true, "unsupported voxel type");
      ospSetString(volume->handle,"voxelType",voxelType);

      // Check if a subvolume of the volume has been specified.
      // Subvolume params: subvolumeOffsets, subvolumeDimensions,
      // subvolumeSteps.
      // The subvolume defaults to full dimensions (allowing for just
      // subsampling, for example).
      ospcommon::vec3i subvolumeOffsets = volume->subVolumeOffsets;
      exitOnCondition(reduce_min(subvolumeOffsets) < 0 ||
                      reduce_max(subvolumeOffsets - volumeDimensions) >= 0,
                      "invalid subvolume offsets");
      
      ospcommon::vec3i subvolumeDimensions =
          volumeDimensions - subvolumeOffsets;
      if (volume->subVolumeDimensions != vec3i(-1))
        subvolumeDimensions = volume->subVolumeDimensions;

      exitOnCondition(reduce_min(subvolumeDimensions) < 1 ||
                      reduce_max(subvolumeDimensions -
                                 (volumeDimensions - subvolumeOffsets)) > 0,
                      "invalid subvolume dimension(s) specified");
      
      ospcommon::vec3i subvolumeSteps = ospcommon::vec3i(1);  
      if (volume->subVolumeSteps != vec3i(-1))
        subvolumeSteps = volume->subVolumeSteps;
      exitOnCondition(reduce_min(subvolumeSteps) < 1 ||
                      reduce_max(subvolumeSteps -
                                 (volumeDimensions - subvolumeOffsets)) >= 0,
                      "invalid subvolume steps");
      
      bool useSubvolume = false;
      
      // The dimensions of the volume to be imported; this will be changed if a
      // subvolume is specified.
      ospcommon::vec3i importVolumeDimensions = volumeDimensions;
      
      if (reduce_max(subvolumeOffsets) > 0 ||
          subvolumeDimensions != volumeDimensions ||
          reduce_max(subvolumeSteps) > 1) {

        useSubvolume = true;

        // The dimensions of the volume to be imported, considering the
        // subvolume specified.
        int xdim = subvolumeDimensions.x / subvolumeSteps.x +
          (subvolumeDimensions.x % subvolumeSteps.x != 0);
        int ydim = subvolumeDimensions.y / subvolumeSteps.y +
          (subvolumeDimensions.y % subvolumeSteps.y != 0);
        int zdim = subvolumeDimensions.z / subvolumeSteps.z +
          (subvolumeDimensions.z % subvolumeSteps.z != 0);
        importVolumeDimensions = ospcommon::vec3i(xdim, ydim, zdim);

        // Range check.
        exitOnCondition(reduce_min(importVolumeDimensions) <= 0,
                        "invalid import volume dimensions");

        // Update the provided dimensions of the volume for the
        // subvolume specified.
        ospSet3i(volume->handle, "dimensions",
                 importVolumeDimensions.x,
                 importVolumeDimensions.y,
                 importVolumeDimensions.z);
      }
      else {
        ospSet3i(volume->handle, "dimensions",
                 volumeDimensions.x,
                 volumeDimensions.y,
                 volumeDimensions.z);
      }

      if (!useSubvolume) {

        int numSlicesPerSetRegion = 4;

        // Voxel count.
        size_t voxelCount = volumeDimensions.x * volumeDimensions.y;

        // Allocate memory for a single slice through the volume.
        unsigned char *voxelData =
          new unsigned char[numSlicesPerSetRegion * voxelCount * voxelSize];

        // We copy data into the volume by the slice in case memory is limited.
        for (int z = 0 ;
             z < volumeDimensions.z ;
             z += numSlicesPerSetRegion) {

          // Copy voxel data into the buffer.
          size_t slicesToRead = std::min(numSlicesPerSetRegion,
                                         volumeDimensions.z - z);

          size_t voxelsRead = fread(voxelData,
                                    voxelSize,
                                    slicesToRead * voxelCount,
                                    file);

          // The end of the file may have been reached unexpectedly.
          exitOnCondition(voxelsRead != slicesToRead*voxelCount,
                          "end of volume file reached before read completed");

          if (strcmp(voxelType, "uchar") == 0) {
            extendVoxelRange(volume->voxelRange,
                             (unsigned char *)voxelData,
                             volumeDimensions.x*volumeDimensions.y);
          }
          else if (strcmp(voxelType, "float") == 0) {
            extendVoxelRange(volume->voxelRange,
                             (float *)voxelData,
                             volumeDimensions.x*volumeDimensions.y);
          }
          else if (strcmp(voxelType, "double") == 0) {
            extendVoxelRange(volume->voxelRange,
                             (double *)voxelData,
                             volumeDimensions.x*volumeDimensions.y);
          }
          else {
            exitOnCondition(true, "unsupported voxel type");
          }

          ospcommon::vec3i region_lo(0, 0, z);
          ospcommon::vec3i region_sz(volumeDimensions.x,
                                     volumeDimensions.y,
                                     slicesToRead);
          // Copy the voxels into the volume.
          ospSetRegion(volume->handle,
                       voxelData,
                       osp::vec3i{region_lo.x, region_lo.y, region_lo.z},
                       osp::vec3i{region_sz.x, region_sz.y, region_sz.z});
        }

        ospSet2f(volume->handle,"voxelRange",
                 volume->voxelRange.x,
                 volume->voxelRange.y);

        // Clean up.
        delete [] voxelData;

      } else {

        throw std::runtime_error("subvolumes not yet implemented for "
                                 "RAW files ...");

      }

      volume->bounds = ospcommon::empty;
      volume->bounds.extend(volume->gridOrigin);
      volume->bounds.extend(volume->gridOrigin
                            + vec3f(volume->dimensions) * volume->gridSpacing);
      
      if (gzipped)
        pclose(file);
      else
        fclose(file);
      // Return the volume.
      
    }
    
  } // ::ospray::vv
} // ::ospray
