/*
 *  *************************************************************
 *   Copyright 2016 Computational Engineering International, Inc.
 *   All Rights Reserved.
 *
 *        Restricted Rights Legend
 *
 *   Use, duplication, or disclosure of this
 *   software and its documentation by the
 *   Government is subject to restrictions as
 *   set forth in subdivision [(b)(3)(ii)] of
 *   the Rights in Technical Data and Computer
 *   Software clause at 52.227-7013.
 ***************************************************************/
#include "Mapping.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <iostream>
#include <sstream>

EnsightTex1dMapping::EnsightTex1dMapping(const float partcolor[4], 
    int intp, int dispundef, int limitfringe, 
    int len, const float *levels, const float *values, float tmin, float tmax)
{
    d.m_isAlphaTexture1D = false;
    d.m_intp = InterpolationType(intp);
    d.m_dispundef = DisplayUndefinedType(dispundef);
    d.m_limitfringe = LimitFringeType(limitfringe);

    for (int i = 0; i < 4; i++) d.m_partcolor[i] = partcolor[i];
    memset(d.m_levels, 0, sizeof(d.m_levels));
    memset(d.m_values, 0, sizeof(d.m_values));
    const int n = (len > NLEVELS) ? NLEVELS : len;
    for (int i = 0; i < n; i++){
        d.m_levels[i] = levels[i];
        d.m_values[i] = values[i];
    }
    d.m_tmin = tmin;
    d.m_tmax = tmax;
    d.m_len = n;
}

EnsightTex1dMapping::EnsightTex1dMapping(const char *s)
{
    d.m_isAlphaTexture1D = false;
    d.m_len = 0;
    d.m_intp = InterpolationType(0);
    d.m_dispundef = DisplayUndefinedType(0);
    d.m_limitfringe = LimitFringeType(0);

    memset(d.m_partcolor, 0, sizeof(d.m_partcolor));
    memset(d.m_levels, 0, sizeof(d.m_levels));
    memset(d.m_values, 0, sizeof(d.m_values));

    (*this).fromString(s);
}

void EnsightTex1dMapping::fromString(const char *data)
{
    if (data == NULL || data[0] == 0) return;
    //std::cout << data << std::endl;
    std::stringstream ss(data);
    int ver, tmp;
    ss >> ver >> tmp;
    if (ver != 10200){
        fprintf(stderr, "Error: not supported EnsightTex1dMapping version number!\n");
        return;
    }
    for (int i = 0; i < 4; i++){
        ss >> d.m_partcolor[i];
    }
    ss >> tmp; d.m_dispundef = DisplayUndefinedType(tmp);
    ss >> tmp; d.m_limitfringe = LimitFringeType(tmp);
    ss >> tmp; d.m_intp = InterpolationType(tmp);
    ss >> d.m_len;
    memset(d.m_levels, 0, sizeof(d.m_levels));
    memset(d.m_values, 0, sizeof(d.m_values));
    if (d.m_len > 0){
        for (int i = 0; i < d.m_len; i++){
            ss >> d.m_levels[i];
        }
        for (int i = 0; i < d.m_len; i++){
            ss >> d.m_values[i];
        }
        ss >> d.m_tmin;
        ss >> d.m_tmax;
    }
    else{
        d.m_len = 0;
    }
}

float EnsightTex1dMapping::getT(float x, bool *usergba, float rgba[4]) const
{
    return EnsightTex1dMapping_getT(&this->d, x, usergba, rgba);
}