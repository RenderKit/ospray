/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "VolumeViewer.h"
#include <iostream>
#include <QtGui>

int main(int argc, char * argv[])
{
    // initialize OSPRay
    ospInit(&argc, (const char **)argv);

    // initialize Qt
    QApplication * app = new QApplication(argc, argv);

    // parse commandline arguments
    // we expect: <filename> <dim1> <dim2> <dim3> <type> <format>
    if(argc < 7)
    {
        std::cerr << "usage: " << argv[0] << " <filename> <dim1> <dim2> <dim3> <format> <layout> [options]" << std::endl;
        std::cerr << "options:" << std::endl;
        std::cerr << "  -dt <dt>        : use ray cast sample step size 'dt'" << std::endl;

        return 1;
    }

    std::string filename = argv[1];

    osp::vec3i dimensions;
    dimensions.x = atoi(argv[2]);
    dimensions.y = atoi(argv[3]);
    dimensions.z = atoi(argv[4]);

    std::string format = argv[5];
    std::string layout = argv[6];

    std::cout << "got filename = " << filename << ", dimensions = (" << dimensions.x << ", " << dimensions.y << ", " << dimensions.z << "), format = " << format << ", layout = " << layout << std::endl;

    // parse optional command line arguments

    // viewer will auto-set dt for dt == 0
    float dt = 0.f;

    for(int i=7; i<argc; i++)
    {
        std::string arg = argv[i];

        if(arg == "-dt")
        {
            if(i+1 >= argc)
            {
                throw std::runtime_error("missing <dt> argument");
            }

            dt = atof(argv[++i]);

            std::cout << "got dt = " << dt << std::endl;
        }
        else
        {
            throw std::runtime_error("unknown parameter " + arg);
        }
    }

    // create volume viewer window
    VolumeViewer * volumeViewer = new VolumeViewer();

    // load the volume from the commandline arguments
    volumeViewer->loadVolume(filename, dimensions, format, layout, dt);

    // enter Qt event loop
    app->exec();

    // cleanup
    delete volumeViewer;

    return 0;
}
