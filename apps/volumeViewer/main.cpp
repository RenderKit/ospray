// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include <iostream>
#include <QtGui>
#include <ctype.h>
#include <sstream>
#include "VolumeViewer.h"
#include "TransferFunctionEditor.h"
#include "ospray/include/ospray/ospray.h"

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
    std::cerr << "    -module <moduleName>                 : load the module 'moduleName'"                  << std::endl;
    std::cerr << "    -ply <filename>                      : load PLY geometry from 'filename'"             << std::endl;
    std::cerr << "    -rotate <rate>                       : automatically rotate view according to 'rate'" << std::endl;
    std::cerr << "    -showframerate                       : show the frame rate in the window title bar"   << std::endl;
    std::cerr << "    -fullscreen                          : enter fullscreen mode"                         << std::endl;
    std::cerr << "    -slice <filename>                    : load volume slice from 'filename'"             << std::endl;
    std::cerr << "    -transferfunction <filename>         : load transfer function from 'filename'"        << std::endl;
    std::cerr << "    -viewsize <width>x<height>           : force OSPRay view size to 'width'x'height'"    << std::endl;
    std::cerr << "    -viewup <x> <y> <z>                  : set viewport up vector to ('x', 'y', 'z')"     << std::endl;
    std::cerr << "    -writeframes <filename>              : emit frames to 'filename_xxxxx.ppm'"           << std::endl;
    std::cerr << " "                                                                                        << std::endl;

    return(1);
  }

  //! Parse the OSPRay object file filenames.
  std::vector<std::string> objectFileFilenames;

  for (size_t i=1 ; (i < argc) && (argv[i][0] != '-') ; i++)
    objectFileFilenames.push_back(std::string(argv[i]));

  //! Default values for the optional command line arguments.
  float dt = 0.0f;
  std::vector<std::string> plyFilenames;
  float rotationRate = 0.0f;
  std::vector<std::string> sliceFilenames;
  std::string transferFunctionFilename;
  int benchmarkWarmUpFrames = 0;
  int benchmarkFrames = 0;
  int viewSizeWidth = 0;
  int viewSizeHeight = 0;
  osp::vec3f viewUp(0.f);
  osp::vec3f viewAt(0.f), viewFrom(0.f);
  bool showFrameRate = false;
  bool fullScreen = false;
  std::string writeFramesFilename;

  //! Parse the optional command line arguments.
  for (int i=objectFileFilenames.size() + 1 ; i < argc ; i++) {

    std::string arg = argv[i];

    if (arg == "-dt") {

      if (i + 1 >= argc) throw std::runtime_error("missing <dt> argument");
      dt = atof(argv[++i]);
      std::cout << "got dt = " << dt << std::endl;

    } else if (arg == "-ply") {

      //! Note: we use "-ply" since "-geometry" gets parsed elsewhere...
      if (i + 1 >= argc) throw std::runtime_error("missing <filename> argument");
      plyFilenames.push_back(std::string(argv[++i]));
      std::cout << "got PLY filename = " << plyFilenames.back() << std::endl;

    } else if (arg == "-rotate") {

      if (i + 1 >= argc) throw std::runtime_error("missing <rate> argument");
      rotationRate = atof(argv[++i]);
      std::cout << "got rotationRate = " << rotationRate << std::endl;

    } else if (arg == "-slice") {

      if (i + 1 >= argc) throw std::runtime_error("missing <filename> argument");
      sliceFilenames.push_back(std::string(argv[++i]));
      std::cout << "got slice filename = " << sliceFilenames.back() << std::endl;

    } else if (arg == "-showframerate" || arg == "-fps") {

      showFrameRate = true;
      std::cout << "set show frame rate" << std::endl;

    } else if (arg == "-fullscreen") {

      fullScreen = true;
      std::cout << "go full screen" << std::endl;

    } else if (arg == "-transferfunction") {

      if (i + 1 >= argc) throw std::runtime_error("missing <filename> argument");
      transferFunctionFilename = std::string(argv[++i]);
      std::cout << "got transferFunctionFilename = " << transferFunctionFilename << std::endl;

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

    } else if (arg == "-viewup" || arg == "-vu") {

      if (i + 3 >= argc) throw std::runtime_error("missing <x> <y> <z> arguments");

      viewUp.x = atof(argv[++i]);
      viewUp.y = atof(argv[++i]);
      viewUp.z = atof(argv[++i]);

      std::cout << "got viewup = " << viewUp.x << " " << viewUp.y << " " << viewUp.z << std::endl;

    } else if (arg == "-vp") {

      if (i + 3 >= argc) throw std::runtime_error("missing <x> <y> <z> arguments");

      viewFrom.x = atof(argv[++i]);
      viewFrom.y = atof(argv[++i]);
      viewFrom.z = atof(argv[++i]);

      std::cout << "got view-from (-vp) = " << viewFrom.x << " " << viewFrom.y << " " << viewFrom.z << std::endl;

    } else if (arg == "-vi") {

      if (i + 3 >= argc) throw std::runtime_error("missing <x> <y> <z> arguments");

      viewAt.x = atof(argv[++i]);
      viewAt.y = atof(argv[++i]);
      viewAt.z = atof(argv[++i]);

      std::cout << "got view-at (-vi) = " << viewAt.x << " " << viewAt.y << " " << viewAt.z << std::endl;

    } else if (arg == "-writeframes") {

      if (i + 1 >= argc || argv[i + 1][0] == '-') throw std::runtime_error("the '-writeframes' option requires a filename argument");
      writeFramesFilename = argv[++i];
      std::cout << "got writeFramesFilename = " << writeFramesFilename << std::endl;

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

  //! Create the OSPRay state and viewer window.
  VolumeViewer *volumeViewer = new VolumeViewer(objectFileFilenames, showFrameRate, fullScreen, writeFramesFilename);

  //! Display the first model.
  volumeViewer->setModel(0);

  //! Load PLY geometries from file.
  for(unsigned int i=0; i<plyFilenames.size(); i++)
    volumeViewer->addGeometry(plyFilenames[i]);

  //! Set rotation rate to use in animation mode.
  if(rotationRate != 0.f) {
    volumeViewer->setAutoRotationRate(rotationRate);
    volumeViewer->autoRotate(true);
  }

  //! Load slice(s) from file.
  for(unsigned int i=0; i<sliceFilenames.size(); i++)
    volumeViewer->addSlice(sliceFilenames[i]);

  //! Load transfer function from file.
  if(transferFunctionFilename.empty() != true)
    volumeViewer->getTransferFunctionEditor()->load(transferFunctionFilename);

  //! Set benchmarking parameters.
  volumeViewer->getWindow()->setBenchmarkParameters(benchmarkWarmUpFrames, benchmarkFrames);

  if (viewAt != viewFrom) {
    volumeViewer->getWindow()->getViewport()->at = viewAt;
    volumeViewer->getWindow()->getViewport()->from = viewFrom;
  }

  //! Set the window size if specified.
  if (viewSizeWidth != 0 && viewSizeHeight != 0) volumeViewer->getWindow()->setFixedSize(viewSizeWidth, viewSizeHeight);


  //! Set the view up vector if specified.
  if(viewUp != osp::vec3f(0.f)) {
    volumeViewer->getWindow()->getViewport()->setUp(viewUp);
    volumeViewer->getWindow()->resetAccumulationBuffer();
  }

  //! Enter the Qt event loop.
  app->exec();

  //! Cleanup
  delete volumeViewer;

  return(0);
}
