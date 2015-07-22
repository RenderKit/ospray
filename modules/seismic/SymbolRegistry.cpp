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

#include "modules/loaders/OSPObjectFile.h"
#include "SeismicHorizonFile.h"
#include "SeismicVolumeFile.h"

// Loaders for seismic horizon files for supported self-describing formats.
OSP_REGISTER_TRIANGLEMESH_FILE(SeismicHorizonFile, dds);

// Loader for seismic volume files for supported self-describing formats.
OSP_REGISTER_VOLUME_FILE(SeismicVolumeFile, dds);
OSP_REGISTER_VOLUME_FILE(SeismicVolumeFile, H);
OSP_REGISTER_VOLUME_FILE(SeismicVolumeFile, sgy);
OSP_REGISTER_VOLUME_FILE(SeismicVolumeFile, segy);

//! Module initialization function
extern "C" void ospray_init_module_seismic()
{
  std::cout << "loaded module 'seismic'." << std::endl;
}
