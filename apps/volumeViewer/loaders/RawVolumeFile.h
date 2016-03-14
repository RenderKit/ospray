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

// own
#include "VolumeFile.h"
// std
#include <string>

//! \brief A concrete implementation of the VolumeFile class for reading
//!  voxel data stored in a file on disk as a single monolithic brick,
//!  where the volume specification is defined elsewhere.
//!
class RawVolumeFile : public VolumeFile {
public:

  //! Constructor.
  RawVolumeFile(const std::string &filename) : filename(filename) {}

  //! Destructor.
  virtual ~RawVolumeFile() {};

  //! Import the volume data.
  virtual OSPVolume importVolume(OSPVolume volume);

  //! A string description of this class.
  virtual std::string toString() const { return("ospray_module_loaders::RawVolumeFile"); }

private:

  //! Path to the file containing the volume data.
  std::string filename;

};

