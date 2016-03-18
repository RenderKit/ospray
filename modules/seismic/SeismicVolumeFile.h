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

#include "ospray/common/OSPCommon.h"
#include <cdds.h>
#include <string>
#include "apps/volumeViewer/loaders/VolumeFile.h"

//! \brief A concrete implementation of the VolumeFile class
//!  for reading voxel data stored in seismic file formats on disk.
//!
class SeismicVolumeFile : public VolumeFile {
public:

  //! Constructor.
  SeismicVolumeFile(const std::string &filename) : filename(filename), verbose(true) {}

  //! Destructor.
  virtual ~SeismicVolumeFile() {};

  //! Import the volume data.
  virtual OSPVolume importVolume(OSPVolume volume);

  //! A string description of this class.
  virtual std::string toString() const { return("ospray_module_seismic::SeismicVolumeFile"); }

private:

  //! Path to the file containing the volume data.
  std::string filename;

  //! Verbose logging.
  bool verbose;

  //! Seismic data attributes
  BIN_TAG inputBinTag;
  int traceHeaderSize;
  ospray::vec3i dimensions;           //<! Dimensions of the volume.
  ospray::vec3f deltas;               //!< Voxel spacing along each dimension.

  //! Use a subvolume of the full volume.
  bool useSubvolume;
  ospray::vec3i subvolumeOffsets;     //!< Subvolume offset from full volume origin.
  ospray::vec3i subvolumeDimensions;  //!< Dimensions of subvolume, not considering any subsampling.
  ospray::vec3i subvolumeSteps;       //!< Step size for generation of subvolume in each dimension; values > 1 allow for subsampling.

  //! The dimensions of the volume to be imported, considering any subvolume parameters.
  ospray::vec3i volumeDimensions;

  //! The grid spacing of the volume to be imported.
  ospray::vec3f gridSpacing;

  //! Open the seismic data file and populate attributes.
  bool openSeismicDataFile(OSPVolume volume);

  //! Scan the seismic data file to determine the volume dimensions.
  bool scanSeismicDataFileForDimensions(OSPVolume volume);

  //! Import the voxel data from the file into the volume.
  bool importVoxelData(OSPVolume volume);

};
