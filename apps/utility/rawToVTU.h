// Intel copyright here

// pragmas?

#include "ospcommon/array3D/Array3D.h"
#include "ospcommon/box.h"
#include "ospcommon/FileName.h"
#include "ospcommon/tasking/parallel_for.h"

#include <mutex>

// ALOK: TODO
// remove these if possible
using namespace ospcommon::array3D;
using namespace ospcommon;

// ALOK: replace with user argument if possible
#define USE_THRESHOLD 1 // used in ospray::vtu::makeVTU()

namespace ospray {
    namespace vtu {
        // ALOK: TODO
        // are these necessary or are they duplicates of
        // other structs?
        enum CellType {
            Tetra = 0x0a,
            Hexa  = 0x0c,
            Wedge = 0x0d,
            unknown = 0xff
        };

        struct PointsData {
            void resize(uint64_t count) {
                samples.resize(count);
                coords.resize(count);
            }

            std::vector<float> samples;
            range1f samplesRange;
            std::vector<vec3f> coords;
            box3f coordsRange;
        };

        struct CellsData {
            void resize(uint64_t count, unsigned int vertsPerCell) {
                connectivity.resize(vertsPerCell * count);
                offsets.resize(count);
                types.resize(count);
            }

            std::vector<uint64_t> connectivity;
            std::vector<uint64_t> offsets;
            std::vector<uint8_t> types;
        };

        void makeVTU(const std::shared_ptr<Array3D<float>> &in,
                     const std::string primType,
                     const range1f &threshold,
                     PointsData &points,
                     CellsData &cells);

        void outputVTU(const std::string &fileName,
                       const PointsData &points,
                       const CellsData &cells);

    } // namespace vtu
} // namespace ospray
