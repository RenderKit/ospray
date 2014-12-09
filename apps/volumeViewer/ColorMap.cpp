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
