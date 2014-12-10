// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

ColorMap::ColorMap(std::string name, std::vector<osp::vec3f> colors)
{
  name_ = name;
  colors_ = colors;
}

std::string ColorMap::getName()
{
  return name_;
}

std::vector<osp::vec3f> ColorMap::getColors()
{
  return colors_;
}

QImage ColorMap::getImage()
{
  int numRows = 1;

  QImage image(int(colors_.size()), numRows, QImage::Format_ARGB32_Premultiplied);

  for(unsigned int i=0; i<colors_.size(); i++)
    {
      for(unsigned int j=0; j<numRows; j++)
        {
          image.setPixel(QPoint(i, j), QColor::fromRgbF(colors_[i].x, colors_[i].y, colors_[i].z).rgb());
        }
    }

  return image;
}
