//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include <iostream>
#include <QtGui>
#include <ctype.h>
#include <sstream>
#include "VolumeViewer.h"

int main(int argc, char *argv[]) {

    //! Initialize OSPRay.
    ospInit(&argc, (const char **) argv);

    //! Initialize Qt.
    QApplication *app = new QApplication(argc, argv);

    //! Print the command line usage.
    if (argc < 2) {

        std::cerr << " "                                                                                        << std::endl;
        std::cerr << "  USAGE:  " << argv[0] << " <filename> [filename] ... [options]"                          << std::endl;
        std::cerr << " "                                                                                        << std::endl;
        std::cerr << "  Options:"                                                                               << std::endl;
        std::cerr << " "                                                                                        << std::endl;
        std::cerr << "    -benchmark <warm-up frames> <frames> : run benchmark and report overall frame rate"   << std::endl;
        std::cerr << "    -dt <dt>                             : use ray cast sample step size 'dt'"            << std::endl;
        std::cerr << "    -rotate <rate>                       : automatically rotate view according to 'rate'" << std::endl;
        std::cerr << "    -viewsize <width>x<height>           : force OSPRay view size to 'width'x'height'"    << std::endl;
        std::cerr << "    -module <moduleName>                 : load the module 'moduleName'"                  << std::endl;
        std::cerr << " "                                                                                        << std::endl;
        return(1);

    }

    //! Parse the volume file names.
    std::vector<std::string> filenames;  for (size_t i=1 ; (i < argc) && (argv[i][0] != '-') ; i++) filenames.push_back(std::string(argv[i]));

    //! Default values for the optional command line arguments.
    float dt = 0.0f;
    float rotationRate = 0.0f;
    int benchmarkWarmUpFrames = 0;
    int benchmarkFrames = 0;
    int viewSizeWidth = 0;
    int viewSizeHeight = 0;

    //! Parse the optional command line arguments.
    for (int i=filenames.size() + 1 ; i < argc ; i++) {

        std::string arg = argv[i];

        if (arg == "-dt") {

            if (i + 1 >= argc) throw std::runtime_error("missing <dt> argument");
            dt = atof(argv[++i]);
            std::cout << "got dt = " << dt << std::endl;

        } else if (arg == "-rotate") {

            if (i + 1 >= argc) throw std::runtime_error("missing <rate> argument");
            rotationRate = atof(argv[++i]);
            std::cout << "got rotationRate = " << rotationRate << std::endl;

        } else if (arg == "-benchmark") {

            if (i + 2 >= argc) throw std::runtime_error("missing <warm-up frames> <frames> arguments");
            benchmarkWarmUpFrames = atoi(argv[++i]);
            benchmarkFrames = atoi(argv[++i]);
            std::cout << "got benchmarkWarmUpFrames = " << benchmarkWarmUpFrames << ", benchmarkFrames = " << benchmarkFrames << std::endl;

        } else if (arg == "-viewsize") {

            if (i + 1 >= argc) throw std::runtime_error("missing <width>x<height> argument");
            std::string arg2(argv[++i]);
            size_t pos = arg2.find("x");

            if (pos != std::string::npos) {

                arg2.replace(pos, 1, " ");
                std::stringstream ss(arg2);
                ss >> viewSizeWidth >> viewSizeHeight;
                std::cout << "got viewSizeWidth = " << viewSizeWidth << ", viewSizeHeight = " << viewSizeHeight << std::endl;

            } else throw std::runtime_error("improperly formatted <width>x<height> argument");

        } else if (arg == "-module") {

            if (i + 1 >= argc) throw std::runtime_error("missing <moduleName> argument");
            std::string moduleName = argv[++i];
            std::cout << "loading module '" << moduleName << "'." << std::endl;
            error_t error = ospLoadModule(moduleName.c_str());

            if(error != 0) {
                std::ostringstream ss;
                ss << error;
                throw std::runtime_error("could not load module " + moduleName + ", error " + ss.str());
            }

        } else throw std::runtime_error("unknown parameter " + arg);

    }

    //! Create the volume viewer window.
    VolumeViewer *volumeViewer = new VolumeViewer(dt);

    //! Load the volumes from the specified files.
    for (size_t i=0 ; i < filenames.size() ; i++) volumeViewer->initVolumeFromFile(filenames[i]);

    //! Display the first volume.
    volumeViewer->setVolume(0);

    //! Set rotation rate to use in animation mode.
    volumeViewer->getQOSPRayWindow()->setRotationRate(rotationRate);

    //! Set benchmarking parameters.
    volumeViewer->getQOSPRayWindow()->setBenchmarkParameters(benchmarkWarmUpFrames, benchmarkFrames);

    //! Set the window size if specified.
    if (viewSizeWidth != 0 && viewSizeHeight != 0) volumeViewer->getQOSPRayWindow()->setFixedSize(viewSizeWidth, viewSizeHeight);

    //! Enter the Qt event loop.
    app->exec();

    //! Cleanup
    delete volumeViewer;  return(0);

}

