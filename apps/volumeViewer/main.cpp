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
        std::cerr << "usage: " << argv[0] << " <filename> <dim1> <dim2> <dim3> <format> <layout>" << std::endl;
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

    // create volume viewer window
    VolumeViewer * volumeViewer = new VolumeViewer();

    // load the volume from the commandline arguments
    volumeViewer->loadVolume(filename, dimensions, format, layout);

    // enter Qt event loop
    app->exec();

    // cleanup
    delete volumeViewer;

    return 0;
}
