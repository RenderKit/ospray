// Intel copyright here

#include "rawToAMR.h"

int main(int ac, char **av)
{
    // ALOK: TODO
    // better argument handling and usage description
    if (ac != 11) {
        std::cout << "usage: ./raw2amr in.raw <float|byte|double> Nx Ny Nz "
            "numLevels brickSize refineFactor threshold outfilebase"
            << std::endl;
        exit(0);
    }

    // inFileName - input filename, e.g. /mnt/ssd/turbulence/vel256.raw
    const char *inFileName   = av[1];
    // format - one of float, byte/uchar/uint8, or double/float64
    // though it seems only float is acceptable as an AMR volume in the rest
    // of OSPRay
    const std::string format = av[2];
    // inDims - dimensions of the volume in input dataset as a space-
    // separated triplet
    const vec3i inDims(atoi(av[3]), atoi(av[4]), atoi(av[5]));
    // numLevels - how many refinement levels to do (????)
    int numLevels           = atoi(av[6]);
    // blockSize - brick size
    // of the smallest brick (????)
    int blockSize                  = atoi(av[7]);
    // refinementLevel - refinement factor, or the scalar multiple for brick size 
    // e.g. 2 means each refinement level is twice as small in each dimension
    // (????)
    int refinementLevel                  = atoi(av[8]);
    // threshold - some sort of data threshold, potentially when to create
    // a new refinement level (????)
    float threshold         = atof(av[9]);
    // outFileBase - path + basename for output files
    // this program creates three files with different extensions: .osp,
    // .data, and .info
    // so you would provide, e.g., /mnt/ssd/turbulence/vel256_amr
    std::string outFileBase = av[10];

    // ALOK: ultimately storing the data as float regardless of input type
    // (????)
    std::shared_ptr<Array3D<float>> in;
    if (format == "float") {
        in = mmapRAW<float>(inFileName, inDims);
    } else if (format == "byte" || format == "uchar" || format == "uint8") {
        in = std::make_shared<Array3DAccessor<unsigned char, float>>(
                mmapRAW<unsigned char>(inFileName, inDims));
    } else if (format == "double" || format == "float64") {
        in = std::make_shared<Array3DAccessor<double, float>>(
                mmapRAW<double>(inFileName, inDims));
    } else {
        throw std::runtime_error("unknown input voxel format");
    }

    ospray::amr::makeAMR(in, numLevels, blockSize, refinementLevel, threshold, outFileBase);

    return 0;
}
