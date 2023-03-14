#include "MicrofacetAlbedoTables.h"
#include "common/ISPCRTBuffers.h"
#include "render/bsdfs/MicrofacetAlbedoTablesShared.h"
#ifndef OSPRAY_TARGET_SYCL
#include "render/bsdfs/MicrofacetAlbedoTables_ispc.h"
#else
namespace ispc {
void precomputeMicrofacetAlbedoTables(void *_tables);
}
#endif

namespace ospray {

MicrofacetAlbedoTables::MicrofacetAlbedoTables(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext())
{
  albedo_dir = make_buffer_shared_unique<float>(device.getIspcrtContext(),
      MICROFACET_ALBEDO_TABLE_SIZE * MICROFACET_ALBEDO_TABLE_SIZE);
  albedo_avg = make_buffer_shared_unique<float>(
      device.getIspcrtContext(), MICROFACET_ALBEDO_TABLE_SIZE);

  dielectricAlbedo_dir =
      make_buffer_shared_unique<float>(device.getIspcrtContext(),
          MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE);
  dielectricAlbedo_avg =
      make_buffer_shared_unique<float>(device.getIspcrtContext(),
          MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE);

  dielectricRcpEtaAlbedo_dir =
      make_buffer_shared_unique<float>(device.getIspcrtContext(),
          MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE);
  dielectricRcpEtaAlbedo_avg =
      make_buffer_shared_unique<float>(device.getIspcrtContext(),
          MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE);

  dielectricReflectionAlbedo_dir =
      make_buffer_shared_unique<float>(device.getIspcrtContext(),
          MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE);
  dielectricReflectionAlbedo_avg =
      make_buffer_shared_unique<float>(device.getIspcrtContext(),
          MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
              * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE);

  sheenAlbedo_dir = make_buffer_shared_unique<float>(device.getIspcrtContext(),
      MICROFACET_SHEEN_ALBEDO_TABLE_SIZE * MICROFACET_SHEEN_ALBEDO_TABLE_SIZE);

  getSh()->albedo_dir = albedo_dir->data();
  getSh()->albedo_avg = albedo_avg->data();

  getSh()->dielectricAlbedo_dir = dielectricAlbedo_dir->data();
  getSh()->dielectricAlbedo_avg = dielectricAlbedo_avg->data();

  getSh()->dielectricRcpEtaAlbedo_dir = dielectricRcpEtaAlbedo_dir->data();
  getSh()->dielectricRcpEtaAlbedo_avg = dielectricRcpEtaAlbedo_avg->data();

  getSh()->dielectricReflectionAlbedo_dir =
      dielectricReflectionAlbedo_dir->data();
  getSh()->dielectricReflectionAlbedo_avg =
      dielectricReflectionAlbedo_avg->data();

  getSh()->sheenAlbedo_dir = sheenAlbedo_dir->data();

  // TODO: Could be a kernel dispatch for SYCL, right now it's run serially on
  // the host
  ispc::precomputeMicrofacetAlbedoTables(getSh());
}

std::string MicrofacetAlbedoTables::toString() const
{
  return "ospray::MicrofacetAlbedoTables";
}

} // namespace ospray
