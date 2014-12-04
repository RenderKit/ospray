/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include <string>
#include <vector>
#include <ospray/ospray.h>

class PLYGeometryFile {

public:

    PLYGeometryFile(const std::string &filename);

    //! Get an OSPTriangleMesh representing the geometry.
    OSPTriangleMesh getOSPTriangleMesh();

protected:

    //! PLY geometry filename.
    std::string filename;

    //! Vertices.
    std::vector<osp::vec3fa> vertices;

    //! Vertex colors.
    std::vector<osp::vec3fa> vertexColors;

    //! Triangle definitions.
    std::vector<osp::vec3i>  triangles;

    //! Parse the file, determining the vertices, vertex colors, and triangles.
    bool parse();

};
