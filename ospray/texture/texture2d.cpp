#include "texture2d.h"

namespace ospray {

  Texture2D *Texture2D::createTexture(int width, int height, OSPDataType type, void *data, int flags) {
    Texture2D *tx = new Texture2D;
    if (tx) {
      //NOTE: For now, we just copy the data and rely on the viewer to have logic for buffer sharing.
      int bpp = 0;
      switch (type) {
        case OSP_vec4uc: bpp = 4; break;
        case OSP_vec3uc: bpp = 3; break;
        case OSP_vec3f: bpp = 3 * sizeof(float); break;
        case OSP_vec3fa: bpp = 4 * sizeof(float); break;
        default: throw std::runtime_error("Could not determine bytes per pixel when creating a Texture2D");
      };

      const int bytes = width * height * bpp;
      tx->width = width;
      tx->height = height;
      tx->type = type;
      tx->data = new unsigned char[bytes];

      if (data) memcpy(tx->data, data, bytes);
    }

    return tx;
  }

}
