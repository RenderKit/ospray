// Intel copyright here

// pragmas?

#include "ospcommon/array3D/Array3D.h"
#include "ospcommon/box.h"
#include "ospcommon/FileName.h"
#include "ospcommon/tasking/parallel_for.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

// ALOK: TODO
// remove these if possible
using namespace ospcommon::array3D;
using namespace ospcommon;

namespace ospray {
    // ALOK: TODO
    // move these out of the namespace if possible
    // (here because both amr and vtu had these)
    static size_t numWritten = 0;
    static size_t numRemoved = 0;

    namespace amr {
        // ALOK: TODO
        // remove this and use osp_amr_brick_info instead
        struct BrickDesc
        {
            box3i box;
            int level;
            float dt;
        };

        // ALOK: TODO
        // move these out of the namespace if possible
        static std::mutex fileMutex;
        //static FILE *infoOut = nullptr;
        //static FILE *dataOut = nullptr;

        void makeAMR(const std::shared_ptr<Array3D<float>> in,
                     const int numLevels,
                     const int blockSize,
                     const int refinementLevel,
                     const float threshold,
                     const FileName outFileBase);
    } // namespace amr
} // namespace ospray
