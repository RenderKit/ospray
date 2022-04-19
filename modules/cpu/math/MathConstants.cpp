#include "math/MathConstants.h"
#include "math/halton.h"

namespace ospray {

MathConstants::MathConstants(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice())
{
  halton_perm3_buf =
      make_buffer_shared_unique<unsigned int>(device.getIspcrtDevice(), 243);
  halton_perm5_buf =
      make_buffer_shared_unique<unsigned int>(device.getIspcrtDevice(), 125);
  halton_perm7_buf =
      make_buffer_shared_unique<unsigned int>(device.getIspcrtDevice(), 343);

  std::memcpy(halton_perm3_buf->begin(),
      halton_perm3,
      halton_perm3_buf->size() * sizeof(unsigned int));

  std::memcpy(halton_perm5_buf->begin(),
      halton_perm5,
      halton_perm5_buf->size() * sizeof(unsigned int));

  std::memcpy(halton_perm7_buf->begin(),
      halton_perm7,
      halton_perm7_buf->size() * sizeof(unsigned int));

  getSh()->halton_perm3 = halton_perm3_buf->data();
  getSh()->halton_perm5 = halton_perm5_buf->data();
  getSh()->halton_perm7 = halton_perm7_buf->data();
}

std::string MathConstants::toString() const
{
  return "ospray::MathConstants";
}

} // namespace ospray
