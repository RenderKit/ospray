#include "ospray/volume/BlockBrickedVolume.h"

namespace ospray {

  //! A volume type with 64-bit addressing and multi-level bricked storage order.
  OSP_REGISTER_VOLUME(BlockBrickedVolume, block_bricked_volume);

  //! A volume type with 32-bit addressing and brick storage order.
  //  OSP_REGISTER_VOLUME(BrickedVolume, bricked_volume);

  //! A volume type with 32-bit addressing and XYZ storage order.
  //  OSP_REGISTER_VOLUME(MonolithicVolume, monolithic_volume);

} // ::ospray

