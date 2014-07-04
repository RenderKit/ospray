#pragma once

#include <ospray/ospray.h>
#include <string>
#include <vector>
#include <QtGui>

class ColorMap
{
public:

    ColorMap(std::string name, std::vector<osp::vec3f> colors);

    std::string getName();
    std::vector<osp::vec3f> getColors();

    QImage getImage();

protected:

    std::string name_;
    std::vector<osp::vec3f> colors_;
};
