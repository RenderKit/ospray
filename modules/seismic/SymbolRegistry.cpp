#include "apps/common/fileio/OSPObjectFile.h"
#include "SeismicVolumeFile.h"

namespace ospray {

  //! Loader for seismic volume files for supported self-describing formats.
  OSP_REGISTER_VOLUME_FILE(SeismicVolumeFile, dds);
  OSP_REGISTER_VOLUME_FILE(SeismicVolumeFile, sgy);
  OSP_REGISTER_VOLUME_FILE(SeismicVolumeFile, segy);

} // namespace ospray
