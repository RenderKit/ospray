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

#include "ColorMap.h"

ColorMap::ColorMap(std::string name, std::vector<ospcommon::vec3f> colors)
{
  this->name = name;
  this->colors = colors;
}

std::string ColorMap::getName() const
{
  return name;
}

std::vector<ospcommon::vec3f> ColorMap::getColors() const
{
  return colors;
}

QImage ColorMap::getImage()
{
  int numRows = 1;

  QImage image(int(colors.size()), numRows, QImage::Format_ARGB32_Premultiplied);

  for(unsigned int i=0; i<colors.size(); i++)
    {
      for(unsigned int j=0; j<numRows; j++)
        {
          image.setPixel(QPoint(i, j), QColor::fromRgbF(colors[i].x, colors[i].y, colors[i].z).rgb());
        }
    }

  return image;
}
