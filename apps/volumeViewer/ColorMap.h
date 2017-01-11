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

// ospray public
#include <ospray/ospray.h>
// ospcommon
#include "ospcommon/vec.h"
// std
#include <string>
#include <vector>
#include <QtGui>

class ColorMap
{
public:

  ColorMap(std::string name, std::vector<ospcommon::vec3f> colors);

  std::string getName() const;
  std::vector<ospcommon::vec3f> getColors() const;

  QImage getImage();

protected:

  std::string name;
  std::vector<ospcommon::vec3f> colors;
};
