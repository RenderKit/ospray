/*! \file viewer.cpp Implements a *sample* viewer for the haroon plugin */

#undef NDEBUG

#include "ospray/rv_module.h"
// STL
#include <vector>
// viewer widget
#include "../../apps/util/glut3D/glut3D.h"
// ospray
#include "ospray/render/util.h"

#define TOK "\t ()\n\r"

#define USE_GLUI 1
#ifdef USE_GLUI
#include <GL/glui.h>
#endif

namespace ospray {
  namespace rv_viewer {
    using namespace ospray::glut3D;
    using std::cout;
    using std::endl;

    // default layer names for this application.
    const char *layerName[] = { "p+", "pl", "n+", "p", "tcn", "gcn", "m0", "m1", "m2", "m3", 
                                "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "tm1", 
                                "mp1", "mp2", "vcn", "vg", "vt", "v0", "v1", "v2", "v3", 
                                "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "tv1", 
                                "vp12", NULL };

    const float layerZ[][2] = { { -40,919 }, { 1320,-1 }, { -40,-1 }, { 0,39 }, { 0,39 }, 
                                { 0,39 }, { 440,479 }, { 880,919 }, { 1320,1359 }, 
                                { 1760,1799 }, { 2200,2239 }, { 2640,2679 }, { 3080,3119 },
                                { 3780,3879 }, { 4480,4579 }, { 9080,9679 }, { 13680,14279 }, 
                                { 18280,21279 }, { 41280,44279 }, { 64280,67279 }, 
                                { 87280,90279 }, { 40,439 }, { 40,439 }, { 40,439 },
                                { 480,879 }, { 920,1319 }, { 1360,1759 }, { 1800,2199 }, 
                                { 2240,2639 }, { 2680,3079 }, { 3120,3779 }, { 3880,4479 }, 
                                { 4580,9079 }, { 9680,13679 }, { 14280,18279 }, 
                                { 21280,41279 }, { 44280,64279 }, { 67280,87279 }, {-1,-1} };


    // ID of the glui window, in case we need to GLUI_select_window
    int windowID = -1;

    //! num layers defined (global variable to allow glui to operate on array of its size)
    size_t numLayers;
    //! array of flags defining which layers are enabled
    int *layerEnable;
    //! shade mode when rendering
    int shadeMode = ospray::rv::RENDER_BY_LAYER;

    //! vector of resistors we're visualizing
    std::vector<ospray::rv::Resistor> resistorVec;
    //! list of attrib names
    std::vector<std::string> attributeName;
    //! list of attribute values
    std::vector<float> attributeVec;
    //! value and color ranges of all attributes
    std::vector<ospray::rv::ColorRange> attributeRange;
    //! the resistor model we use (we only use a single one)
    ospray::rv::id_t modelID;

    //! define the layers to the rv modules
    void specifyLayers() 
    {
      // count number of layers
      numLayers = 0;
      for (const char **p = layerName; *p; ++p) numLayers++;
      ospray::rv::Layer *layer = new ospray::rv::Layer[numLayers];
      layerEnable = new int[numLayers];
      for (int i=0;i<numLayers;i++) {
        layer[i].min_z = layerZ[i][0];
        layer[i].max_z = layerZ[i][1];
        layer[i].color = ospray::makeRandomColor(i+1234567);
      }
      ospray::rv::setLayers(layer,numLayers);
      for (int i=0;i<numLayers;i++) {
        layerEnable[i] = 1;
        ospray::rv::setLayerVisibility(i,layerEnable[i]);
      }
    }
    
    //! define the nets to be used 
    void specifyNets()
    {
      // (iw): i don't really know what the nets are for, so i'll just
      // specify a single one
      ospray::rv::Net net;
      net.color = ospray::makeRandomColor(123);
      ospray::rv::setNets(&net,1);
      cout << "#osp:rv_viewer: nets specified" << endl;
    }

    //! define the nets to be used 
    void specifyResistors()
    {
      modelID = ospray::rv::newResistorModel(resistorVec.size(),
                                             &resistorVec[0],
                                             &attributeVec[0]);
      cout << "#osp:rv_viewer: resistors specified" << endl;
    }

    box3f resistorBounds(size_t resID)
    {
      box3f bounds;
      bounds.lower.x = resistorVec[resID].coordinate.lower.x;
      bounds.lower.y = resistorVec[resID].coordinate.lower.y;
      bounds.upper.x = resistorVec[resID].coordinate.upper.x;
      bounds.upper.y = resistorVec[resID].coordinate.upper.y;
      bounds.lower.z = layerZ[resistorVec[resID].layerID][0];
      bounds.upper.z = layerZ[resistorVec[resID].layerID][1];
      return bounds;
    }

    struct ResistorViewer : public Glut3DWidget {
      /*! construct volume from file name and dimensions \see volview_notes_on_volume_interface */
      ResistorViewer(const std::string &modelFileName) 
        : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE), 
#if USE_GLUI
          glui(NULL),
#endif
          initialized(false)
      {
        parseModelFile(modelFileName);
        box3f world = embree::empty;
        for (int i=0;i<resistorVec.size();i++)
          world.extend(resistorBounds(i));
        cout << "#osp:rv_viewer: world bounds is " << world << endl;
        setWorldBounds(world);
        initColorRanges();
      };
      virtual void reshape(const ospray::vec2i &newSize) 
      {
        if (!initialized) {
          initialize();
          initialized = true;
        }
        Glut3DWidget::reshape(newSize);
        ospray::rv::setFrameBufferSize(newSize);

        // if (fb) ospFreeFrameBuffer(fb);
        // fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
        // ospSetf(camera,"aspect",viewPort.aspect);
        // ospCommit(camera);
        // setTitle("Resistor Viewer (sample, for testing only)");
      }

      void initialize();

      virtual void display() 
      {
        if (!initialized) return;

        if (viewPort.modified) {
          ospray::rv::setCamera(viewPort.from,viewPort.at,viewPort.up,
                                viewPort.aspect, 60.f);
          viewPort.modified = false;
        }

        PING;

        // ospRenderFrame(fb,renderer);
    
        ucharFB = (unsigned int *)ospray::rv::mapFB();
        frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
        Glut3DWidget::display();
        ospray::rv::unmapFB();
        // forceRedraw();
      }

      void initColorRanges();
      void parseModelFile(const std::string &modelFileName);

      //! whether we have already initailized and created the GUI
      bool initialized;
#if USE_GLUI
      GLUI *glui;
#endif
    };


    void ResistorViewer::initColorRanges()
    {
      const size_t numAttribs = attributeName.size();
      attributeRange.resize(numAttribs);
      for (int i=0;i<numAttribs;i++) {
        attributeRange[i].lo_value = attributeVec[i];
        attributeRange[i].hi_value = attributeVec[i];
        attributeRange[i].lo_color = vec3f(0.f,1.f,0.f);
        attributeRange[i].hi_color = vec3f(1.f,0.f,0.f);
      }
      for (int a=0;a<attributeVec.size();a+=numAttribs) 
        for (int i=0;i<numAttribs;i++) {
          attributeRange[i].lo_value = std::min(attributeRange[i].lo_value,
                                                attributeVec[a+i]);
          attributeRange[i].hi_value = std::max(attributeRange[i].hi_value,
                                                attributeVec[a+i]);
        }
      cout << "#osp:rv: attribute ranges initialized" << endl;
    }

#ifdef USE_GLUI
    int whichAttribToVisualize = 0;
    void glui_attribute_changed(int which)
    {
      // which = whichAttribToVisualize;
      // ivl::haroon_setAttribs(rivl::g_haroon->attrib[which].min,
      //                        rivl::g_haroon->attrib[which].max,
      //                        rivl::g_haroon->attrib[which].d_data);
      // viewer::mainWindow.accumID = 0;
      if (shadeMode == ospray::rv::RENDER_BY_ATTRIBUTE)
        ospray::rv::shadeByAttribute(whichAttribToVisualize,
                                     attributeRange[whichAttribToVisualize]);
      glutSetWindow(windowID);  
      glutPostRedisplay();
    }
    void allOnOffCB(int what)
    {
      for (int i=0;i<numLayers;i++) {
        layerEnable[i] = what;
        ospray::rv::setLayerVisibility(i,layerEnable[i]);
      }
      glutSetWindow(windowID);  
      glutPostRedisplay();
    }
    void glui_shadeMode_changed(int ID)
    {
      switch(ID) {
      case 0: {
        ospray::rv::shadeByLayer();
      } break;
      case 1: {
        ospray::rv::shadeByNet();
      } break;
      case 2: {
        ospray::rv::shadeByAttribute(whichAttribToVisualize,
                                     attributeRange[whichAttribToVisualize]);
      } break;
      }
      glutSetWindow(windowID);  
      glutPostRedisplay();
    }
    void glui_layer_toggled(int which)
    {
      ospray::rv::setLayerVisibility(which,layerEnable[which]);
    }

#endif

    void ResistorViewer::initialize()
    {
#if USE_GLUI
      char text[10000];
      glui = GLUI_Master.create_glui( "OSPRay Resistor Viewer UI" );
      ospray::rv_viewer::windowID = windowID;

      // -------------------------------------------------------
      GLUI_Panel *infoPanel = glui->add_panel("Model Info");
      // glui->add_statictext_to_panel(infoPanel,"Hello world...");

      sprintf(text,"Number of attributes: %li",attributeName.size());
      glui->add_statictext_to_panel(infoPanel,text);

      const long numPrims = resistorVec.size();
      if (numPrims >= 1000000000) 
        sprintf(text,"Number of elements: %.3fG",numPrims/1000000000.f);
      else if (numPrims >= 1000000) 
        sprintf(text,"Number of elements: %.3fM",numPrims/1000000.f);
      else if (numPrims >= 1000) 
        sprintf(text,"Number of elements: %.3fK",numPrims/1000.f);
      else
        sprintf(text,"Number of elements: %li",numPrims);
      glui->add_statictext_to_panel(infoPanel,text);

      // -------------------------------------------------------
      GLUI_Panel *attribPanel = glui->add_panel("Attributes to visualize");
      GLUI_RadioGroup *rg = glui->add_radiogroup_to_panel
        (attribPanel,&whichAttribToVisualize,0,GLUI_CB(glui_attribute_changed));
      for (int i=0;i<attributeName.size();i++) {
        sprintf(text,"%s [min=%f:max=%f]",
                attributeName[i].c_str(),
                attributeRange[i].lo_value,
                attributeRange[i].hi_value);
        glui->add_radiobutton_to_group(rg,text);
      }

      // -------------------------------------------------------
      // glui->add_column();
      GLUI_Panel *layerPanel = glui->add_panel("Model Layers");

      GLUI_Panel *allOnOffPanel = glui->add_panel_to_panel(layerPanel,"all on/off");
      glui->add_button_to_panel(allOnOffPanel,"all OFF",0,GLUI_CB(allOnOffCB));
      glui->add_separator_to_panel(allOnOffPanel);
      glui->add_button_to_panel(allOnOffPanel,"all ON",1,GLUI_CB(allOnOffCB));

      for (int i=0;i<numLayers;i++) {
        glui->add_checkbox_to_panel(layerPanel,layerName[i],
                                    &layerEnable[i],
                                    i,GLUI_CB(glui_layer_toggled));
      }
#if 1


      // -------------------------------------------------------
      GLUI_Panel *shadeModePanel = glui->add_panel("Shade Mode");
      GLUI_RadioGroup *shadeModeRG
        = glui->add_radiogroup_to_panel(shadeModePanel,&shadeMode,-1,
                                        GLUI_CB(glui_shadeMode_changed));
      glui->add_radiobutton_to_group(shadeModeRG,"by layer");
      glui->add_radiobutton_to_group(shadeModeRG,"by net");
      glui->add_radiobutton_to_group(shadeModeRG,"by attribute");
#endif
#endif
      ospray::rv::shadeByLayer();
    }

    void ResistorViewer::parseModelFile(const std::string &modelFileName)
    {
      Assert(resistorVec.empty());
      cout << "Parsing resistor model file '" << modelFileName << "'" << endl;

      FILE *in = fopen(modelFileName.c_str(),"r");
      Assert(in);
#define LNSZ 10000
      char line[LNSZ];

      // first, parse the attribute names
      int llxID = -1;
      int llyID = -1;
      int urxID = -1;
      int uryID = -1;
      int layerID = -1;

      while (fgets(line,LNSZ,in) && !feof(in)) {
        char *tok = strtok(line,TOK); // attrib
        if (!tok) continue;
        long attrNum = 0;
        if (!strcmp(tok,"&ATTRIBUTES")) {
          // found attribute line, parse it
          tok = strtok(NULL,TOK); 
          while (tok) {
            // check if it's really an attribute, or a resistor value
            std::string attrName = tok;
            if (attrName == "llx")   { llxID   = attrNum++; tok = strtok(NULL,TOK); continue; }
            if (attrName == "lly")   { llyID   = attrNum++; tok = strtok(NULL,TOK); continue; }
            if (attrName == "urx")   { urxID   = attrNum++; tok = strtok(NULL,TOK); continue; }
            if (attrName == "ury")   { uryID   = attrNum++; tok = strtok(NULL,TOK); continue; }
            if (attrName == "layer") { layerID = attrNum++; tok = strtok(NULL,TOK); continue; }
            
            attributeName.push_back(tok);
            attrNum++;
            tok = strtok(NULL,TOK);
          }
          break;
        }
      }
      if (attributeName.empty())
        throw std::runtime_error("could not parse attributes");
      cout << "#osp:rv_viewer: found " << attributeName.size() << " attributes: ";
      for (int i=0;i<attributeName.size();i++)
        cout << "[" << attributeName[i] << "]";
      cout << endl;

      Assert(llxID >= 0);
      Assert(llyID >= 0);
      Assert(urxID >= 0);
      Assert(uryID >= 0);
      Assert(layerID >= 0);
      
      while (fgets(line,LNSZ,in) && !feof(in)) {
        if (line[0] == '!') continue;
        if (line[0] == '&') continue;
        char *tok = strtok(line,TOK); // attrib
        if (!tok) continue;

        int attrNum = 0;
        ospray::rv::Resistor res;
        bzero(&res,sizeof(res));
        tok = strtok(NULL,TOK);
        for (; tok; tok = strtok(NULL,TOK)) {
          if (attrNum == layerID) {
            res.layerID = -1;
            for (const char **ln = layerName; *ln; ln++)              
              if (!strcasecmp(*ln,tok))
                { res.layerID = ln-layerName; break; }
            if ((int)res.layerID == -1) {
              throw std::runtime_error("could not find layer "+std::string(tok));
            }
            Assert((int)res.layerID != -1);
          }
          else if (attrNum == llxID) res.coordinate.lower.x = atof(tok);
          else if (attrNum == llyID) res.coordinate.lower.y = atof(tok);
          else if (attrNum == urxID) res.coordinate.upper.x = atof(tok);
          else if (attrNum == uryID) res.coordinate.upper.y = atof(tok);
          else attributeVec.push_back(atof(tok));
          ++attrNum;
        }
        resistorVec.push_back(res);
        // check if the right amount of attributes was read...
        Assert(attributeVec.size() == attributeName.size()*resistorVec.size());
      }
      cout << "#osp:rv_viewer: found " << resistorVec.size() << " resistors" << endl;
    }

    int viewerMain(int ac, const char **av)
    {
      ospray::rv::initRV(&ac,av);
      ospray::glut3D::initGLUT(&ac,av);
      specifyLayers();
      specifyNets();
      specifyResistors();

      if (ac != 2)
        throw std::runtime_error("usage: ./ospRV <rvfile.xml>");

      ResistorViewer viewer(av[1]);
      viewer.create("Resistor Viewer (sample, for testing only)");
      ospray::glut3D::runGLUT();
      PING;
      return 0;
    }
  }
}

int main(int ac, const char **av)
{
  return ospray::rv_viewer::viewerMain(ac,av);
}

