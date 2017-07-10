// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#ifdef USE_IMAGEMAGICK
#define MAGICKCORE_QUANTUM_DEPTH 16
#define MAGICKCORE_HDRI_ENABLE 1
# include <Magick++.h>
using namespace Magick;
#  ifndef MaxRGB
#    define MaxRGB QuantumRange
#  endif
#endif

#include "sg/common/Texture2D.h"
#include "ospray/common/OSPCommon.h"

namespace ospray {
  namespace sg {

    std::string Texture2D::toString() const
    {
      return "ospray::viewer::sg::Texture2D";
    }

    //! \brief load texture from given file.
    /*! \detailed if file does not exist, or cannot be loaded for
      some reason, return NULL. Multiple loads from the same file
      will return the *same* texture object */
    std::shared_ptr<Texture2D> Texture2D::load(const FileName &fileNameAbs,
                                               const bool preferLinear)
    {
      FileName fileName = fileNameAbs;
      std::string fileNameBase = fileNameAbs;
      std::string path = fileNameAbs.path();

      /* WARNING: this cache means that every texture ever loaded will
         forever keep at least one refcount - ie, no texture will ever
         auto-die!!! (to fix this we'd have to add a dedicated
         'clearTextureCache', or move this caching fucntionality into
         a CachedTextureLoader objec that each parser creates and
         destroys) */
      static std::map<std::string,std::shared_ptr<Texture2D> > textureCache;
      if (textureCache.find(fileName.str()) != textureCache.end())
        return textureCache[fileName.str()];

      std::shared_ptr<Texture2D> tex = std::static_pointer_cast<Texture2D>(
        createNode(fileName.name(),"Texture2D"));
      const std::string ext = fileName.ext();

      if (ext == "ppm") {
        try {
            #ifdef USE_IMAGEMAGICK
            if (1)
            {
                Magick::Image image(fileName.str().c_str());
                tex->size.x    = image.columns();
                tex->size.y   = image.rows();
                tex->channels = image.matte() ? 4 : 3;
                const bool hdr = image.depth() > 8;
                tex->depth    = hdr ? 4 : 1;
                tex->preferLinear = preferLinear;
                float rcpMaxRGB = 1.0f/float(MaxRGB);
                const Magick::PixelPacket* pixels = image.getConstPixels(0,0,tex->size.x,tex->size.y);
                if (!pixels) {
                  std::cerr << "#osp:minisg: failed to load texture '"+fileName.str()+"'" << std::endl;
                } else {
                  tex->data = new unsigned char[tex->size.x*tex->size.y*tex->channels*tex->depth];
                  // convert pixels and flip image (because OSPRay's textures have the origin at the lower left corner)
                  for (size_t y=0; y<tex->size.y; y++) {
                    for (size_t x=0; x<tex->size.x; x++) {
                      const Magick::PixelPacket &pixel = pixels[y*tex->size.x+x];
                      if (hdr) {
                        float *dst = &((float*)tex->data)[(x+(tex->size.y-1-y)*tex->size.x)*tex->channels];
                        *dst++ = pixel.red * rcpMaxRGB;
                        *dst++ = pixel.green * rcpMaxRGB;
                        *dst++ = pixel.blue * rcpMaxRGB;
                        if (tex->channels == 4)
                          *dst++ = (MaxRGB - pixel.opacity) * rcpMaxRGB;
                      } else {
                        unsigned char *dst = &((unsigned char*)tex->data)[(x+(tex->size.y-1-y)*tex->size.x)*tex->channels];
                        *dst++ = pixel.red;
                        *dst++ = pixel.green;
                        *dst++ = pixel.blue;
                        if (tex->channels == 4)
                          *dst++ = 255 - (unsigned char)pixel.opacity;
                      }
                    }
                  }
                }
            } else
            #endif
        {
          int rc, peekchar;

          // open file
          FILE *file = fopen(fileName.str().c_str(),"rb");
          if (!file)
          {
              if (fileNameBase.size() > 0) {
                if (fileNameBase.substr(0,1) == "/") {// Absolute path.
                  fileName = fileNameBase;
                  path = "";
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

          // tex = new Texture2D;
          tex->size.x   = width;
          tex->size.y   = height;
          tex->channels = 3;
          tex->depth    = 1;
          tex->preferLinear = preferLinear;
          tex->data     = new unsigned char[width*height*3];
          rc = fread(tex->data,width*height*3,1,file);
          // flip in y, because OSPRay's textures have the origin at the lower left corner
          unsigned char *texels = (unsigned char *)tex->data;
          for (int y = 0; y < height/2; y++)
            for (int x = 0; x < width*3; x++)
              std::swap(texels[y*width*3+x], texels[(height-1-y)*width*3+x]);
      }
        } catch(std::runtime_error e) {
          std::cerr << e.what() << std::endl;
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
          tex->data     = new float[width * height * numChannels];
          if (fread(tex->data, sizeof(float), width * height * numChannels, file)
              != width * height * numChannels)
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
        } catch(std::runtime_error e) {
          std::cerr << e.what() << std::endl;
        }
      }
      else {
#ifdef USE_IMAGEMAGICK
        Magick::Image image(fileName.str().c_str());
        tex->size.x    = image.columns();
        tex->size.y   = image.rows();
        tex->channels = image.matte() ? 4 : 3;
        const bool hdr = image.depth() > 8;
        tex->depth    = hdr ? 4 : 1;
        tex->preferLinear = preferLinear;
        float rcpMaxRGB = 1.0f/float(MaxRGB);
        const Magick::PixelPacket* pixels = image.getConstPixels(0,0,tex->size.x,tex->size.y);
        if (!pixels) {
          std::cerr << "#osp:minisg: failed to load texture '"+fileName.str()+"'" << std::endl;
        } else {
          tex->data = new unsigned char[tex->size.x*tex->size.y*tex->channels*tex->depth];
          // convert pixels and flip image (because OSPRay's textures have the origin at the lower left corner)
          for (size_t y=0; y<tex->size.y; y++) {
            for (size_t x=0; x<tex->size.x; x++) {
              const Magick::PixelPacket &pixel = pixels[y*tex->size.x+x];
              if (hdr) {
                float *dst = &((float*)tex->data)[(x+(tex->size.y-1-y)*tex->size.x)*tex->channels];
                *dst++ = pixel.red * rcpMaxRGB;
                *dst++ = pixel.green * rcpMaxRGB;
                *dst++ = pixel.blue * rcpMaxRGB;
                if (tex->channels == 4)
                  *dst++ = (MaxRGB - pixel.opacity) * rcpMaxRGB;
              } else {
                unsigned char *dst = &((unsigned char*)tex->data)[(x+(tex->size.y-1-y)*tex->size.x)*tex->channels];
                *dst++ = pixel.red;
                *dst++ = pixel.green;
                *dst++ = pixel.blue;
                if (tex->channels == 4)
                  *dst++ = 255 - (unsigned char)pixel.opacity;
              }
            }
          }
        }
#else
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
        if (!pixels) {
          std::cerr << "#osp:sg: failed to load texture '"+fileName.str()+"'" << std::endl;
        } else {
          tex->data = new unsigned char[tex->size.x*tex->size.y*tex->channels*tex->depth];
          // convert pixels and flip image (because OSPRay's textures have the origin at the lower left corner)
          for (size_t y=0; y<tex->size.y; y++) {
            for (size_t x=0; x<tex->size.x; x++) {
              if (hdr) {
                const float *pixel = &((float*)pixels)[(y*tex->size.x+x)*tex->channels];
                float *dst = &((float*)tex->data)[(x+(tex->size.y-1-y)*tex->size.x)*tex->channels];
                *dst++ = pixel[0];
                *dst++ = pixel[1];
                *dst++ = pixel[2];
                if (tex->channels == 4)
                  *dst++ = pixel[3];
              } else {
                const unsigned char *pixel = &pixels[(y*tex->size.x+x)*tex->channels];
                unsigned char *dst = &((unsigned char*)tex->data)[((tex->size.y-1-y)*tex->size.x+x)*tex->channels];
                *dst++ = pixel[0];
                *dst++ = pixel[1];
                *dst++ = pixel[2];
                if (tex->channels == 4)
                  *dst++ = pixel[3];
              }
            }
          }
        }
#endif
      }
      textureCache[fileName.str()] = tex;
      std::cout << "loaded texture " << fileName << std::endl;
      return tex;

      // if (ext == "ppm") {
      //   try {
      //     int rc, peekchar;

      //     // open file
      //     FILE *file = fopen(fileName.str().c_str(),"rb");
      //     const int LINESZ=10000;
      //     char lineBuf[LINESZ+1];

      //     if (!file)
      //       throw std::runtime_error("#osp:sg: could not open texture file '"+fileName.str()+"'.");

      //     // read format specifier:
      //     int format=0;
      //     if (fscanf(file,"P%i\n",&format) != 1)
      //       throw std::runtime_error("#osp:sg: could not parse PPM type");
      //     if (format != 6)
      //       throw std::runtime_error("#osp:sg: can currently load only binary P6 subformats for PPM texture files. "
      //                                "Please report this bug at ospray.github.io.");

      //     // skip all comment lines
      //     peekchar = getc(file);
      //     while (peekchar == '#') {
      //       if (!fgets(lineBuf,LINESZ,file))
      //         throw std::runtime_error("could not fgets");
      //       peekchar = getc(file);
      //     } ungetc(peekchar,file);

      //     // read width and height from first non-comment line
      //     int width=-1,height=-1;
      //     rc = fscanf(file,"%i %i\n",&width,&height);
      //     if (rc != 2)
      //       throw std::runtime_error("#osp:sg: could not parse width and height in P6 PPM file '"+fileName.str()+"'. "
      //                                "Please report this bug at ospray.github.io, and include named file to reproduce the error.");

      //     // skip all comment lines
      //     peekchar = getc(file);
      //     while (peekchar == '#') {
      //       if (!fgets(lineBuf,LINESZ,file))
      //         throw std::runtime_error("could not fgets");
      //       peekchar = getc(file);
      //     } ungetc(peekchar,file);

      //     // read maxval
      //     int maxVal=-1;
      //     rc = fscanf(file,"%i",&maxVal);
      //       peekchar = getc(file);

      //     if (rc != 1)
      //       throw std::runtime_error("#osp:sg: could not parse maxval in P6 PPM file '"+fileName.str()+"'. "
      //                                "Please report this bug at ospray.github.io, and include named file to reproduce the error.");
      //     if (maxVal != 255)
      //       throw std::runtime_error("#osp:sg: could not parse P6 PPM file '"+fileName.str()+"': currently supporting only maxVal=255 formats."
      //                                "Please report this bug at ospray.github.io, and include named file to reproduce the error.");

      //     // tex = std::make_shared<Texture2D>();
      //     tex->size      = vec2i(width,height);
      //     tex->texelType = preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
      //     unsigned char *texel     = new unsigned char[width*height*3];
      //     if (!fread(texel,width*height*3,1,file)) {
      //       delete[] texel;
      //       throw std::runtime_error("could not fread");
      //     }
      //     // flip in y, because OSPRay's textures have the origin at the lower left corner
      //     for (int y=0; y < height/2; y++)
      //       for (int x=0; x < width*3; x++)
      //         std::swap(texel[y*width*3+x], texel[(height-1-y)*width*3+x]);

      //     // create data array that now 'owns' the texels
      //     tex->texelData = std::make_shared<DataArray1uc>(texel,width*height*3,true);
      //   } catch(std::runtime_error e) {
      //     std::cerr << e.what() << std::endl;
      //   }
      // }
      // textureCache[fileName.str()] = tex;
      // return tex;
    }

    Texture2D::Texture2D()
      : data(nullptr), texelData(nullptr), ospTexture2D(nullptr)
    {
      setValue((OSPObject)nullptr);
    }

    void Texture2D::preCommit(RenderContext &ctx)
    {
      OSPTextureFormat type = OSP_TEXTURE_R8;

      if (depth == 1) {
        if( channels == 1 ) type = OSP_TEXTURE_R8;
        if( channels == 3 )
          type = preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
        if( channels == 4 )
          type = preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
      } else if (depth == 4) {
        if( channels == 1 ) type = OSP_TEXTURE_R32F;
        if( channels == 3 ) type = OSP_TEXTURE_RGB32F;
        if( channels == 4 ) type = OSP_TEXTURE_RGBA32F;
      }

      void* dat = data;
      if (!dat && texelData)
        dat = texelData->base();
      if (!dat)
      {
        ospTexture2D = nullptr;
        std::cout << "Texture2D: image data null\n";
        return;
      }

      ospTexture2D = ospNewTexture2D( (osp::vec2i&)size,
                                       type,
                                       dat,
                                       0);
      setValue((OSPObject)ospTexture2D);
      ospCommit(ospTexture2D);
    }

    OSP_REGISTER_SG_NODE(Texture2D);


  } // ::ospray::sg
} // ::ospray
