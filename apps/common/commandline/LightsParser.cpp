#include "LightsParser.h"

#include <ospray_cpp/Data.h>

#include <vector>

#include <common/FileName.h>
#include <common/miniSG/miniSG.h>

using namespace ospcommon;

OSPTexture2D createTexture2D(ospray::miniSG::Texture2D *msgTex)
  {
    if(msgTex == NULL) {
      static int numWarnings = 0;
      if (++numWarnings < 10)
        std::cout << "WARNING: material does not have Textures (only warning for the first 10 times)!" << std::endl;
      return NULL;
    }

    static std::map<ospray::miniSG::Texture2D*, OSPTexture2D> alreadyCreatedTextures;
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

DefaultLightsParser::DefaultLightsParser(ospray::cpp::Renderer renderer) :
  m_renderer(renderer),
  m_defaultDirLight_direction(.3, -1, -.2)
{
}

bool DefaultLightsParser::parse(int ac, const char **&av)
{
  std::vector<OSPLight> lights;

  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--sun-dir") {
      if (!strcmp(av[i+1],"none")) {
        ++i;
        m_defaultDirLight_direction = vec3f(0.f);
      } else {
        m_defaultDirLight_direction.x = atof(av[++i]);
        m_defaultDirLight_direction.y = atof(av[++i]);
        m_defaultDirLight_direction.z = atof(av[++i]);
      }
    } else if (arg == "--hdri-light") {
      // Syntax for HDRI light is the same as Embree:
        // --hdri-light <intensity> <image file>.(pfm|ppm)
        auto ospHdri = m_renderer.newLight("hdri");
        ospHdri.set("name", "hdri light");//   ospSetString(ospHdri, "name", "hdri_test");
        if (1) {
          ospHdri.set("up", 0.f, 1.f, 0.f);//ospSet3f(ospHdri, "up", 0.f, 1.f, 0.f);
          ospHdri.set("dir", 1.f, 0.f, 0.0f);//ospSet3f(ospHdri, "dir", 1.f, 0.f, 0.0f);
        } else {// up = z
            //ospSet3f(ospHdri, "up", 0.f, 0.f, 1.f);
            //ospSet3f(ospHdri, "dir", 0.f, 1.f, 0.0f);

          ospHdri.set("up", 0.f, 0.f, 1.f);//ospSet3f(ospHdri, "up", 0.f, 1.f, 0.f);
          ospHdri.set("dir", 0.f, 1.f, 0.0f);//ospSet3f(ospHdri, "dir", 1.f, 0.f, 0.0f);
        }
        ospHdri.set( "intensity", atof(av[++i]));
        FileName imageFile(av[++i]);
        ospray::miniSG::Texture2D *lightMap = ospray::miniSG::loadTexture(imageFile.path(), imageFile.base());
        if (lightMap == NULL){
          std::cout << "Failed to load hdri-light texture '" << imageFile << "'" << std::endl;
        } else {
          std::cout << "Successfully loaded hdri-light texture '" << imageFile << "'" << std::endl;
        }
        OSPTexture2D ospLightMap = createTexture2D(lightMap);
        ospHdri.set( "map", ospLightMap);
        ospHdri.commit();
        //ospCommit(ospHdri);
        lights.push_back(ospHdri.handle());
    } else if (arg == "--backplate") {
      /*
        FileName imageFile(av[++i]);
        miniSG::Texture2D *backplate = miniSG::loadTexture(imageFile.path(), imageFile.base());
        if (backplate == NULL){
          std::cout << "Failed to load backplate texture '" << imageFile << "'" << std::endl;
        }
        ospBackplate = createTexture2D(backplate);
        ospSetObject(ospRenderer, "backplate", ospBackplate);
        */
    }
  }


  //TODO: Need to figure out where we're going to read lighting data from
  
  if (m_defaultDirLight_direction != vec3f(0.f)) {
    auto ospLight = m_renderer.newLight("DirectionalLight");
    if (ospLight.handle() == nullptr) {
      throw std::runtime_error("Failed to create a 'DirectionalLight'!");
    }
    ospLight.set("name", "sun");
    ospLight.set("color", 1.f, 1.f, 1.f);
    ospLight.set("direction", m_defaultDirLight_direction);
    ospLight.set("angularDiameter", 0.53f);
    ospLight.commit();
    lights.push_back(ospLight.handle());
  }

  auto lightArray = ospray::cpp::Data(lights.size(), OSP_OBJECT, lights.data());
  //lightArray.commit();
  m_renderer.set("lights", lightArray);

  finalize();

  return true;
}

void DefaultLightsParser::finalize()
{
  
}
