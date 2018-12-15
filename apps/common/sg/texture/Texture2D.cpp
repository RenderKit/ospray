// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#define STB_IMAGE_IMPLEMENTATION
#include "../3rdParty/stb_image.h"

#ifdef USE_OPENIMAGEIO
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING
#endif

#include "Texture2D.h"

#include "ospray/common/OSPCommon.h"

namespace ospray {
  namespace sg {

    // static helper functions ////////////////////////////////////////////////

    OSPTextureFormat
    osprayTextureFormat(int depth, int channels, bool preferLinear = false)
    {
      if (depth == 1) {
        if( channels == 1 ) return OSP_TEXTURE_R8;
        if( channels == 3 )
          return preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
        if( channels == 4 )
          return preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
      } else if (depth == 4) {
        if( channels == 1 ) return OSP_TEXTURE_R32F;
        if( channels == 3 ) return OSP_TEXTURE_RGB32F;
        if( channels == 4 ) return OSP_TEXTURE_RGBA32F;
      }

      return OSP_TEXTURE_FORMAT_INVALID;
    }

    // Texture2D definitions //////////////////////////////////////////////////

    Texture2D::Texture2D() : Texture("texture2d") {}

    void Texture2D::preCommit(RenderContext &ctx)
    {
      if (committed)
        return;

      Texture::preCommit(ctx);

      auto ospTexture = valueAs<OSPTexture>();

      auto type = osprayTextureFormat(depth, channels, preferLinear);

      if (data == nullptr)
        data = texelData.get() != nullptr ? texelData->base() : nullptr;
      else {
        const auto dataSize = size.x * size.y * channels * depth;
        texelData =
          std::make_shared<sg::DataArray1uc>((unsigned char*)data, dataSize);
      }

      if (data == nullptr)
        throw std::runtime_error("committed a Texture2D node with null data!");

      ospSet1i(ospTexture, "type", (int)type);
      ospSet1i(ospTexture, "flags", nearestFilter ? OSP_TEXTURE_FILTER_NEAREST : 0);
      ospSet2i(ospTexture, "size", size.x, size.y);
      ospSetObject(ospTexture, "data", texelData->getOSP());
    }

    void Texture2D::postCommit(RenderContext &)
    {
      if (committed)
        return;
      auto ospTexture = valueAs<OSPTexture>();
      ospCommit(ospTexture);
      committed = true;
    }

    std::string Texture2D::toString() const
    {
      return "ospray::sg::Texture2D";
    }

    //! \brief load texture from given file.
    /*! \detailed if file does not exist, or cannot be loaded for
      some reason, return NULL. Multiple loads from the same file
      will return the *same* texture object */
    std::shared_ptr<Texture2D> Texture2D::load(const FileName &fileNameAbs,
                                               const bool preferLinear,
                                               const bool nearestFilter)
    {
      FileName fileName = fileNameAbs;
      std::string fileNameBase = fileNameAbs;

      if (textureCache.find(fileName.str()) != textureCache.end())
        return textureCache[fileName.str()];

      std::shared_ptr<Texture2D> tex = std::static_pointer_cast<Texture2D>(
        createNode(fileName.name(),"Texture2D"));

#ifdef USE_OPENIMAGEIO
      ImageInput *in = ImageInput::open(fileName.str().c_str());
      if (!in) {
        std::cerr << "#osp:sg: failed to load texture '"+fileName.str()+"'" << std::endl;
        tex.reset();
      } else {
        const ImageSpec &spec = in->spec();

        tex->size.x = spec.width;
        tex->size.y = spec.height;
        tex->channels = spec.nchannels;
        const bool hdr = spec.format.size() > 1;
        tex->depth = hdr ? 4 : 1;
        tex->preferLinear = preferLinear;
        tex->nearestFilter = nearestFilter;
        const size_t stride = tex->size.x * tex->channels * tex->depth;
        tex->data = alignedMalloc(sizeof(unsigned char) * tex->size.y * stride);

        in->read_image(hdr ? TypeDesc::FLOAT : TypeDesc::UINT8, tex->data);
        in->close();
        ImageInput::destroy(in);

        // flip image (because OSPRay's textures have the origin at the lower left corner)
        unsigned char* data = (unsigned char*)tex->data;
        for (int y = 0; y < tex->size.y / 2; y++) {
          unsigned char *src = &data[y * stride];
          unsigned char *dest = &data[(tex->size.y-1-y) * stride];
          for (size_t x = 0; x < stride; x++)
            std::swap(src[x], dest[x]);
        }
      }
#else
      const std::string ext = fileName.ext();

      if (ext == "ppm") {
        try {
          int rc, peekchar;

          // open file
          FILE *file = fopen(fileName.str().c_str(),"rb");
          if (!file)
          {
              if (fileNameBase.size() > 0) {
                if (fileNameBase.substr(0,1) == "/") {// Absolute path.
                  fileName = fileNameBase;
                }
              }
          }
          const int LINESZ=10000;
          char lineBuf[LINESZ+1];

          if (!file)
            throw std::runtime_error("#osp:miniSG: could not open texture file '"+fileName.str()+"'.");

          // read format specifier:
          int format=0;
          rc = fscanf(file,"P%i\n",&format);
          if (format != 6)
            throw std::runtime_error("#osp:miniSG: can currently load only binary P6 subformats for PPM texture files. "
                                     "Please report this bug at ospray.github.io.");

          // skip all comment lines
          peekchar = getc(file);
          while (peekchar == '#') {
            auto tmp = fgets(lineBuf,LINESZ,file);
            (void)tmp;
            peekchar = getc(file);
          } ungetc(peekchar,file);

          // read width and height from first non-comment line
          int width=-1,height=-1;
          rc = fscanf(file,"%i %i\n",&width,&height);
          if (rc != 2)
            throw std::runtime_error("#osp:miniSG: could not parse width and height in P6 PPM file '"+fileName.str()+"'. "
                                     "Please report this bug at ospray.github.io, and include named file to reproduce the error.");

          // skip all comment lines
          peekchar = getc(file);
          while (peekchar == '#') {
            auto tmp = fgets(lineBuf,LINESZ,file);
            (void)tmp;
            peekchar = getc(file);
          } ungetc(peekchar,file);

          // read maxval
          int maxVal=-1;
          rc = fscanf(file,"%i",&maxVal);
          peekchar = getc(file);

          if (rc != 1)
            throw std::runtime_error("#osp:miniSG: could not parse maxval in P6 PPM file '"+fileName.str()+"'. "
                                     "Please report this bug at ospray.github.io, and include named file to reproduce the error.");
          if (maxVal != 255)
            throw std::runtime_error("#osp:miniSG: could not parse P6 PPM file '"+fileName.str()+"': currently supporting only maxVal=255 formats."
                                     "Please report this bug at ospray.github.io, and include named file to reproduce the error.");
          tex->size.x   = width;
          tex->size.y   = height;
          tex->channels = 3;
          tex->depth    = 1;
          tex->preferLinear = preferLinear;
          tex->nearestFilter = nearestFilter;

          unsigned int dataSize = tex->size.x * tex->size.y * tex->channels * tex->depth;
          tex->data = alignedMalloc(sizeof(unsigned char) * dataSize);
          rc = fread(tex->data,dataSize,1,file);
          // flip in y, because OSPRay's textures have the origin at the lower left corner
          unsigned char *texels = (unsigned char *)tex->data;
          for (int y = 0; y < height/2; y++)
            for (int x = 0; x < width*3; x++) {
              unsigned int a = (y * width * 3) + x;
              unsigned int b = ((height - 1 - y) * width * 3) + x;
              if (a >= dataSize || b >= dataSize) {
                throw std::runtime_error("#osp:minisg: could not parse P6 PPM file '" + fileName.str() + "': buffer overflow.");
              }
              std::swap(texels[a], texels[b]);
            }
        } catch(const std::runtime_error &e) {
          std::cerr << e.what() << std::endl;
          tex.reset();
        }
      } else if (ext == "pfm") {
        try {
          // Note: the PFM file specification does not support comments thus we don't skip any
          // http://netpbm.sourceforge.net/doc/pfm.html
          int rc = 0;
          FILE *file = fopen(fileName.str().c_str(), "rb");
          if (!file) {
            throw std::runtime_error("#osp:miniSG: could not open texture file '"+fileName.str()+"'.");
          }
          // read format specifier:
          // PF: color floating point image
          // Pf: grayscae floating point image
          char format[2] = {0};
          if (fscanf(file, "%c%c\n", &format[0], &format[1]) != 2)
            throw std::runtime_error("could not fscanf");

          if (format[0] != 'P' || (format[1] != 'F' && format[1] != 'f')){
            throw std::runtime_error("#osp:miniSG: invalid pfm texture file, header is not PF or Pf");
          }
          int numChannels = 3;
          if (format[1] == 'f') {
            numChannels = 1;
          }

          // read width and height
          int width = -1;
          int height = -1;
          rc = fscanf(file, "%i %i\n", &width, &height);
          if (rc != 2) {
            throw std::runtime_error("#osp:miniSG: could not parse width and height in PF PFM file '"+fileName.str()+"'. "
                                     "Please report this bug at ospray.github.io, and include named file to reproduce the error.");
          }

          // read scale factor/endiannes
          float scaleEndian = 0.0;
          rc = fscanf(file, "%f\n", &scaleEndian);

          if (rc != 1) {
            throw std::runtime_error("#osp:miniSG: could not parse scale factor/endianness in PF PFM file '"+fileName.str()+"'. "
                                     "Please report this bug at ospray.github.io, and include named file to reproduce the error.");
          }
          if (scaleEndian == 0.0) {
            throw std::runtime_error("#osp:miniSG: scale factor/endianness in PF PFM file can not be 0");
          }
          if (scaleEndian > 0.0) {
            throw std::runtime_error("#osp:miniSG: could not parse PF PFM file '"+fileName.str()+"': currently supporting only little endian formats"
                                     "Please report this bug at ospray.github.io, and include named file to reproduce the error.");
          }
          float scaleFactor = std::abs(scaleEndian);

          tex->size.x    = width;
          tex->size.y   = height;
          tex->channels = numChannels;
          tex->depth    = sizeof(float);
          tex->preferLinear = preferLinear;
          tex->nearestFilter = nearestFilter;
          tex->data     = alignedMalloc(sizeof(float) * width * height * numChannels);
          if (fread(tex->data, sizeof(float), width * height * numChannels, file)
              != size_t(width * height * numChannels))
            throw std::runtime_error("could not fread");
          // flip in y, because OSPRay's textures have the origin at the lower left corner
          float *texels = (float *)tex->data;
          for (int y = 0; y < height / 2; ++y) {
            for (int x = 0; x < width * numChannels; ++x) {
              // Scale the pixels by the scale factor
              texels[y * width * numChannels + x] = texels[y * width * numChannels + x] * scaleFactor;
              texels[(height - 1 - y) * width * numChannels + x] = texels[(height - 1 - y) * width * numChannels + x] * scaleFactor;
              std::swap(texels[y * width * numChannels + x], texels[(height - 1 - y) * width * numChannels + x]);
            }
          }
        } catch(const std::runtime_error &e) {
          std::cerr << e.what() << std::endl;
          tex.reset();
        }
      }
      else {
        int width,height,n;
        const bool hdr = stbi_is_hdr(fileName.str().c_str());
        unsigned char* pixels = nullptr;
        if (hdr)
          pixels = (unsigned char*)stbi_loadf(fileName.str().c_str(), &width, &height, &n, 0);
        else
          pixels = stbi_load(fileName.str().c_str(),&width,&height,&n,0);
        tex->size.x   = width;
        tex->size.y   = height;
        tex->channels = n;
        tex->depth    = hdr ? 4 : 1;
        tex->preferLinear = preferLinear;
        tex->nearestFilter = nearestFilter;
        if (!pixels) {
          std::cerr << "#osp:sg: failed to load texture '"+fileName.str()+"'" << std::endl;
          tex.reset();
        } else {
          tex->data = alignedMalloc(sizeof(unsigned char) * tex->size.x*tex->size.y*tex->channels*tex->depth);
          // convert pixels and flip image (because OSPRay's textures have the origin at the lower left corner)
          for (int y = 0; y < tex->size.y; y++) {
            for (int x = 0; x < tex->size.x; x++) {
              if (hdr) {
                const float *pixel = &((float*)pixels)[(y*tex->size.x+x)*tex->channels];
                float *dst = &((float*)tex->data)[(x+(tex->size.y-1-y)*tex->size.x)*tex->channels];
                for(int i = 0;i<tex->channels;i++)
                  *dst++ = pixel[i];
              } else {
                const unsigned char *pixel = &pixels[(y*tex->size.x+x)*tex->channels];
                unsigned char *dst = &((unsigned char*)tex->data)[((tex->size.y-1-y)*tex->size.x+x)*tex->channels];
                for(int i = 0;i<tex->channels;i++)
                  *dst++ = pixel[i];
              }
            }
          }
        }
      }
#endif

      if (tex.get() != nullptr)
        textureCache[fileName.str()] = tex;

      return tex;
    }

    void Texture2D::clearTextureCache()
    {
      textureCache.clear();
    }

    OSP_REGISTER_SG_NODE(Texture2D);

    std::map<std::string,std::shared_ptr<Texture2D> > Texture2D::textureCache;
  } // ::ospray::sg
} // ::ospray
