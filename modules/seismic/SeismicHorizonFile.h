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

#include <cdds.h>
#include <string>
#include "modules/loaders/TriangleMeshFile.h"

//! \brief A concrete implementation of the TriangleMeshFile class
//!  for reading horizon data stored in seismic file formats on disk.
//!
class SeismicHorizonFile : public TriangleMeshFile {
public:

  //! Constructor.
  SeismicHorizonFile(const std::string &filename) : filename(filename), scale(osp::vec3f(1.f)), verbose(true) {}

  //! Destructor.
  virtual ~SeismicHorizonFile() {};

  //! Import the horizon data.
  virtual OSPTriangleMesh importTriangleMesh(OSPTriangleMesh triangleMesh);

  //! A string description of this class.
  virtual std::string toString() const { return("ospray_module_seismic::SeismicHorizonFile"); }

private:

  //! Path to the file containing the horizon data.
  std::string filename;

  //! Scaling for vertex coordinates.
  osp::vec3f scale;

  //! Verbose logging.
  bool verbose;

  //! Seismic data attributes
  BIN_TAG inputBinTag;
  int traceHeaderSize;
  osp::vec3i dimensions;           //<! Dimensions of the horizon volume.
  osp::vec3f deltas;               //!< Voxel spacing along each dimension.

  //! Open the seismic data file and populate attributes.
  bool openSeismicDataFile(OSPTriangleMesh triangleMesh);

  //! Import the horizon data from the file.
  bool importHorizonData(OSPTriangleMesh triangleMesh);

};
