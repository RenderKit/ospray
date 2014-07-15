/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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
