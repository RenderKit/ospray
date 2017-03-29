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

#include "miniSG.h"
#include "stb_image.h"

#include "ospray/common/OSPCommon.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef USE_IMAGEMAGICK
#define MAGICKCORE_QUANTUM_DEPTH 16
#define MAGICKCORE_HDRI_ENABLE 1
# include <Magick++.h>
using namespace Magick;
#  ifndef MaxRGB
#    define MaxRGB QuantumRange
#  endif
#endif

namespace ospray {
  namespace miniSG {

    Texture2D::Texture2D()
      : channels(0)
      , depth(0)
      , width(0)
      , height(0)
      , data(nullptr)
    {}

    Material::Material()
    {
      // setParam( "Kd", vec3f(.7f) );
      // setParam( "Ks", vec3f(0.f) );
      // setParam( "Ka", vec3f(0.f) );
    }

    Texture2D *loadTexture(const std::string &_path,
                           const std::string &fileNameBase,
                           const bool prefereLinear)
    {
      std::string path = _path;
      FileName fileName = path+"/"+fileNameBase;

      static std::map<std::string,Texture2D*> textureCache;
      if (textureCache.find(fileName.str()) != textureCache.end())
        return textureCache[fileName.str()];

      Texture2D *tex = nullptr;
      const std::string ext = fileName.ext();
      if (ext == "ppm") {
        try { 
            #ifdef USE_IMAGEMAGICK
            if (1)
            {
                Magick::Image image(fileName.str().c_str());
                tex = new Texture2D;
                tex->width    = image.columns();
                tex->height   = image.rows();
                tex->channels = image.matte() ? 4 : 3;
                const bool hdr = image.depth() > 8;
                tex->depth    = hdr ? 4 : 1;
                tex->prefereLinear = prefereLinear;
                float rcpMaxRGB = 1.0f/float(MaxRGB);
                const Magick::PixelPacket* pixels = image.getConstPixels(0,0,tex->width,tex->height);
                if (!pixels) {
                  std::cerr << "#osp:minisg: failed to load texture '"+fileName.str()+"'" << std::endl;
                  delete tex;
                  tex = nullptr;
                } else {
                  tex->data = new unsigned char[tex->width*tex->height*tex->channels*tex->depth];
                  // convert pixels and flip image (because OSPRay's textures have the origin at the lower left corner)
                  for (size_t y=0; y<tex->height; y++) {
                    for (size_t x=0; x<tex->width; x++) {
                      const Magick::PixelPacket &pixel = pixels[y*tex->width+x];
                      if (hdr) {
                        float *dst = &((float*)tex->data)[(x+(tex->height-1-y)*tex->width)*tex->channels];
                        *dst++ = pixel.red * rcpMaxRGB;
                        *dst++ = pixel.green * rcpMaxRGB;
                        *dst++ = pixel.blue * rcpMaxRGB;
                        if (tex->channels == 4)
                          *dst++ = pixel.opacity * rcpMaxRGB;
                      } else {
                        unsigned char *dst = &((unsigned char*)tex->data)[(x+(tex->height-1-y)*tex->width)*tex->channels];
                        *dst++ = pixel.red;
                        *dst++ = pixel.green;
                        *dst++ = pixel.blue;
                        if (tex->channels == 4)
                          *dst++ = pixel.opacity;
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

          tex = new Texture2D;
          tex->width    = width;
          tex->height   = height;
          tex->channels = 3;
          tex->depth    = 1;
          tex->prefereLinear = prefereLinear;
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

          tex = new Texture2D;
          tex->width    = width;
          tex->height   = height;
          tex->channels = numChannels;
          tex->depth    = sizeof(float);
          tex->prefereLinear = prefereLinear;
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
        tex = new Texture2D;
        tex->width    = image.columns();
        tex->height   = image.rows();
        tex->channels = image.matte() ? 4 : 3;
        const bool hdr = image.depth() > 8;
        tex->depth    = hdr ? 4 : 1;
        tex->prefereLinear = prefereLinear;
        float rcpMaxRGB = 1.0f/float(MaxRGB);
        const Magick::PixelPacket* pixels = image.getConstPixels(0,0,tex->width,tex->height);
        if (!pixels) {
          std::cerr << "#osp:minisg: failed to load texture '"+fileName.str()+"'" << std::endl;
          delete tex;
          tex = nullptr;
        } else {
          tex->data = new unsigned char[tex->width*tex->height*tex->channels*tex->depth];
          // convert pixels and flip image (because OSPRay's textures have the origin at the lower left corner)
          for (size_t y=0; y<tex->height; y++) {
            for (size_t x=0; x<tex->width; x++) {
              const Magick::PixelPacket &pixel = pixels[y*tex->width+x];
              if (hdr) {
                float *dst = &((float*)tex->data)[(x+(tex->height-1-y)*tex->width)*tex->channels];
                *dst++ = pixel.red * rcpMaxRGB;
                *dst++ = pixel.green * rcpMaxRGB;
                *dst++ = pixel.blue * rcpMaxRGB;
                if (tex->channels == 4)
                  *dst++ = pixel.opacity * rcpMaxRGB;
              } else {
                unsigned char *dst = &((unsigned char*)tex->data)[(x+(tex->height-1-y)*tex->width)*tex->channels];
                *dst++ = pixel.red;
                *dst++ = pixel.green;
                *dst++ = pixel.blue;
                if (tex->channels == 4)
                  *dst++ = pixel.opacity;
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
        if (n < 3)  //TODO: it seems that grayscale bump maps with > 8 bit crash
        {
           if (ospray::logLevel())
            std::cout << "WARNING: ignoring texture with < 3 channels.  Turn on USE_IMAGEMAGICK to fully utilize this scenes textures\n";
           return tex;
        }
        tex = new Texture2D;
        tex->width    = width;
        tex->height   = height;
        tex->channels = n;
        tex->depth    = hdr ? 4 : 1;
        tex->prefereLinear = prefereLinear;
        unsigned char MaxRGB = 1;
        float rcpMaxRGB = 1.0f/float(MaxRGB);
        if (!pixels) {
          std::cerr << "#osp:minisg: failed to load texture '"+fileName.str()+"'" << std::endl;
          delete tex;
          tex = nullptr;
        } else {
          tex->data = new unsigned char[tex->width*tex->height*tex->channels*tex->depth];
          // convert pixels and flip image (because OSPRay's textures have the origin at the lower left corner)
          for (size_t y=0; y<tex->height; y++) {
            for (size_t x=0; x<tex->width; x++) {
              const unsigned char *pixel = &pixels[(y*tex->width+x)*tex->channels];
              if (hdr) {
                printf("loading hdr image\n");
                float *dst = &((float*)tex->data)[(x+(tex->height-1-y)*tex->width)*tex->channels];
                *dst++ = pixel[0] * rcpMaxRGB;
                *dst++ = pixel[1] * rcpMaxRGB;
                *dst++ = pixel[2] * rcpMaxRGB;
                if (tex->channels == 4)
                  *dst++ = pixel[3] * rcpMaxRGB;
              } else {
                unsigned char *dst = &((unsigned char*)tex->data)[((tex->height-1-y)*tex->width+x)*tex->channels];
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
      return tex;
    }


    float Material::getParam(const char *name, float defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::FLOAT && "Param type mismatch" );
        return it->second->f[0];
      }

      return defaultVal;
    }

    vec2f Material::getParam(const char *name, vec2f defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::FLOAT_2 && "Param type mismatch" );
        return vec2f(it->second->f[0], it->second->f[1]);
      }

      return defaultVal;
    }

    vec3f Material::getParam(const char *name, vec3f defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::FLOAT_3 && "Param type mismatch" );
        return vec3f( it->second->f[0], it->second->f[1], it->second->f[2] );
      }

      return defaultVal;
    }

    vec4f Material::getParam(const char *name, vec4f defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::FLOAT_4 && "Param type mismatch" );
        return vec4f( it->second->f[0], it->second->f[1], it->second->f[2], it->second->f[3] );
      }

      return defaultVal;
    }

    int32_t Material::getParam(const char *name, int32_t defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::INT && "Param type mismatch" );
        return it->second->i[0];
      }

      return defaultVal;
    }

    vec2i Material::getParam(const char *name, vec2i defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::INT_2 && "Param type mismatch" );
        return vec2i(it->second->i[0], it->second->i[1]);
      }

      return defaultVal;
    }

    vec3i Material::getParam(const char *name, vec3i defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::INT_3 && "Param type mismatch" );
        return vec3i(it->second->i[0], it->second->i[1], it->second->i[2]);
      }

      return defaultVal;
    }

    vec4i Material::getParam(const char *name, vec4i defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::INT_4 && "Param type mismatch" );
        return vec4i(it->second->i[0], it->second->i[1], it->second->i[2], it->second->i[3]);
      }

      return defaultVal;
    }


    uint32_t Material::getParam(const char *name, uint32_t defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::UINT && "Param type mismatch" );
        return it->second->ui[0];
      }

      return defaultVal;
    }

    vec2ui Material::getParam(const char *name, vec2ui defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::UINT_2 && "Param type mismatch" );
        return vec2ui(it->second->ui[0], it->second->ui[1]);
      }

      return defaultVal;
    }

    vec3ui Material::getParam(const char *name, vec3ui defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::UINT_3 && "Param type mismatch" );
        return vec3ui(it->second->ui[0], it->second->ui[1], it->second->ui[2]);
      }

      return defaultVal;
    }

    vec4ui Material::getParam(const char *name, vec4ui defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::UINT_4 && "Param type mismatch" );
        return vec4ui(it->second->ui[0], it->second->ui[1], it->second->ui[2], it->second->ui[3]);
      }

      return defaultVal;
    }

    const char *Material::getParam(const char *name, const char *defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::STRING && "Param type mismatch" );
        return it->second->s;
      }

      return defaultVal;
    }

    void *Material::getParam(const char *name, void *defaultVal)
    {
      ParamMap::iterator it = params.find(name);
      if (it != params.end()) {
        assert( it->second->type == Param::TEXTURE /*|| other 'data' types*/&& "Param type mismatch" );
        return it->second->ptr;
      }

      return defaultVal;
    }

    void error(const std::string &err)
    {
      throw std::runtime_error("ospray::miniSG fatal error : "+err);
    }

    bool operator==(const Instance &a, const Instance &b)
    { return a.meshID == b.meshID && a.xfm == b.xfm; }

    bool operator!=(const Instance &a, const Instance &b)
    { return !(a==b); }


    /*! computes and returns the world-space bounding box of given mesh */
    box3f Mesh::getBBox()
    {
      if (bounds.empty()) {
        for (size_t i = 0; i < position.size(); i++)
          bounds.extend(position[i]);
      }
      return bounds;
    }

    /*! computes and returns the world-space bounding box of the entire model */
    box3f Model::getBBox()
    {
      // this does not yet properly support instancing with transforms!
      box3f bBox = ospcommon::empty;
      if (!instance.empty()) {
        std::vector<box3f> meshBounds;
        for (size_t i = 0; i < mesh.size(); i++)
          meshBounds.push_back(mesh[i]->getBBox());
        for (size_t i = 0;i < instance.size(); i++) {
          box3f b_i = meshBounds[instance[i].meshID];
          vec3f corner;
          for (int iz=0;iz<2;iz++) {
            corner.z = iz?b_i.upper.z:b_i.lower.z;
            for (int iy=0;iy<2;iy++) {
              corner.y = iy?b_i.upper.y:b_i.lower.y;
              for (int ix=0;ix<2;ix++) {
                corner.x = ix?b_i.upper.x:b_i.lower.x;
                bBox.extend(xfmPoint(instance[i].xfm,corner));
              }
            }
          }
        }
      } else {
        for (size_t i = 0; i < mesh.size(); i++)
          bBox.extend(mesh[i]->getBBox());
      }
      return bBox;
    }

    size_t Model::numUniqueTriangles() const
    {
      size_t sum = 0;
      for (size_t i = 0; i < mesh.size(); i++)
        sum += mesh[i]->triangle.size();
      return sum;
    }

    OSPTexture2D createTexture2D(Texture2D *msgTex)
    {
      if(msgTex == NULL) {
        static int numWarnings = 0;
        if (++numWarnings < 10)
          std::cout << "WARNING: material does not have Textures (only warning for the first 10 times)!" << std::endl;
        return NULL;
      }

      static std::map<Texture2D*, OSPTexture2D> alreadyCreatedTextures;
      if (alreadyCreatedTextures.find(msgTex) != alreadyCreatedTextures.end())
        return alreadyCreatedTextures[msgTex];

      //TODO: We need to come up with a better way to handle different possible pixel layouts
      OSPTextureFormat type = OSP_TEXTURE_R8;

      if (msgTex->depth == 1) {
        if( msgTex->channels == 1 ) type = OSP_TEXTURE_R8;
        if( msgTex->channels == 3 )
          type = msgTex->prefereLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
        if( msgTex->channels == 4 )
          type = msgTex->prefereLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
      } else if (msgTex->depth == 4) {
        if( msgTex->channels == 1 ) type = OSP_TEXTURE_R32F;
        if( msgTex->channels == 3 ) type = OSP_TEXTURE_RGB32F;
        if( msgTex->channels == 4 ) type = OSP_TEXTURE_RGBA32F;
      }

      vec2i texSize(msgTex->width, msgTex->height);
      OSPTexture2D ospTex = ospNewTexture2D( (osp::vec2i&)texSize,
                                             type,
                                             msgTex->data,
                                             0);

      alreadyCreatedTextures[msgTex] = ospTex;

      ospCommit(ospTex);
      //g_tex = ospTex; // remember last texture for debugging

      return ospTex;
    }

  } // ::ospray::minisg
} // ::ospray

