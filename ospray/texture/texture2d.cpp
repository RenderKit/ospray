#include "texture2d.h"
#include "image3c_ispc.h"
#include "image3ca_ispc.h"
#include "image3f_ispc.h"
#include "image3fa_ispc.h"
#include "nearestneighbor_ispc.h"

namespace ospray {

  Texture2D *Texture2D::createTexture(int width, int height, OSPDataType type, void *data, int flags) {
    Texture2D *tx = new Texture2D;
    if (tx) {
      //NOTE: For now, we just copy the data and rely on the viewer to have logic for buffer sharing.
      int bpp = 0;
      //TODO:ISPC side memory for images isn't currently managed, this will leak!
      void *img = NULL;
      switch (type) {
        case OSP_vec4uc:
          bpp = 4;
          img = ispc::Image3ca__new(width, height, (uint32_t*)data, true);
          break;
        case OSP_vec3uc:
          bpp = 3;
          img = ispc::Image3c__new(width, height, (ispc::vec3uc*)data, true);
          break;
        case OSP_vec3f:
          bpp = 3 * sizeof(float);
          img = ispc::Image3f__new(width, height, (ispc::vec3f*)data, true);
          break;
        case OSP_vec3fa:
          bpp = 4 * sizeof(float);
          img = ispc::Image3fa__new(width, height, (ispc::vec3fa*)data, true);
          break;
        default: throw std::runtime_error("Could not determine bytes per pixel in " __FILE__);
      };

      assert(img && "Image pointer must not be null in " __FILE__);
      tx->ispcEquivalent = ispc::NearestNeighbor__new(img);

      const int bytes = width * height * bpp;
      tx->width = width;
      tx->height = height;
      tx->type = type;
      tx->data = new unsigned char[bytes];

      //Do we really need to make this copy?
      if (data) memcpy(tx->data, data, bytes);
    }

    return tx;
  }

}
