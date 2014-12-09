#include "apps/common/fileio/OSPObjectFile.h"
#include "apps/common/fileio/RawVolumeFile.h"

namespace ospray {

  //! Loader for XML object files.
  OSP_REGISTER_OBJECT_FILE(OSPObjectFile, osp);

  //! Loader for RAW volume files.
  OSP_REGISTER_VOLUME_FILE(RawVolumeFile, raw);

} // ::ospray

