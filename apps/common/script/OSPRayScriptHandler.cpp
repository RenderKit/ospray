// ======================================================================== //
// Copyright 2017 Intel Corporation                                         //
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

#include "ospray/common/OSPCommon.h"
#include "ospray/common/Util.h"
#include "OSPRayScriptHandler.h"
#include "ospcommon/vec.h"
#include "ospcommon/box.h"
#include "chaiscript/chaiscript_stdlib.hpp"
#include "chaiscript/utility/utility.hpp"
#include "tfn_lib/tfn_lib.h"

using std::runtime_error;

using std::cerr;
using std::cin;
using std::cout;
using std::endl;

#include <string>
using std::string;
using std::stringstream;

#include <functional>

#ifdef USE_SYSTEM_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#else

static char *mystrdup (const char *s) {
  size_t len = strlen(s); // Space for length plus nul
  char *d = static_cast<char*>(malloc (len+1));
  if (d == nullptr) return nullptr;          // No memory
#if 0//MSVC
  strcpy_s(d, len+1, s);                        // Copy the characters
#else
  strncpy(d,s,len);                        // Copy the characters
#endif
  d[len] = '\0';
  return d;                            // Return the new string
}

static char* readline(const char* p)
{
  string retval;
  cout << p ;
  getline(std::cin, retval);
  return cin.eof() ? nullptr : mystrdup(retval.c_str());
}


static void add_history(const char*){}
static void using_history(){}
#endif

// Static helper functions ////////////////////////////////////////////////////

static std::string get_next_command() {
  std::string retval("quit");
  char *input_raw = readline("% ");
  if (input_raw) {
    add_history(input_raw);

    std::string val(input_raw);
    size_t pos = val.find_first_not_of("\t \n");
    if (pos != std::string::npos) {
      val.erase(0, pos);
    }
    pos = val.find_last_not_of("\t \n");
    if (pos != std::string::npos) {
      val.erase(pos+1, std::string::npos);
    }

    retval = val;

    ::free(input_raw);
  }
  return retval;
}

// Scripting callback functions ///////////////////////////////////////////////

namespace chaiospray {

void ospLoadModule(const string &name)
{
  ::ospLoadModule(name.c_str());
}

void ospSetString(OSPObject _object, const string &id, const string &s)
{
  ::ospSetString(_object, id.c_str(), s.c_str());
}

void ospSetObject(OSPObject _object, const string &id, OSPObject object)
{
  ::ospSetObject(_object, id.c_str(), object);
}

void ospSet1f(OSPObject _object, const string &id, float x)
{
  ::ospSet1f(_object, id.c_str(), x);
}

void ospSet1i(OSPObject _object, const string &id, int x)
{
  ::ospSet1i(_object, id.c_str(), x);
}

void ospSet2f(OSPObject _object, const string &id, float x, float y)
{
  ::ospSet2f(_object, id.c_str(), x, y);
}

void ospSet2i(OSPObject _object, const string &id, int x, int y)
{
  ::ospSet2i(_object, id.c_str(), x, y);
}

void ospSet3f(OSPObject _object, const string &id, float x, float y, float z)
{
  ::ospSet3f(_object, id.c_str(), x, y, z);
}

void ospSet3i(OSPObject _object, const string &id, int x, int y, int z)
{
  ::ospSet3i(_object, id.c_str(), x, y, z);
}

void ospSetVoidPtr(OSPObject _object, const string &id, void *v)
{
  ::ospSetVoidPtr(_object, id.c_str(), v);
}

void ospRemoveParam(OSPObject _object, const string &id)
{
  ::ospRemoveParam(_object, id.c_str());
}

void ospCommit(OSPObject object)
{
  ::ospCommit(object);
}

ospray::cpp::TransferFunction loadTransferFunction(const std::string &fname) {
  using namespace ospcommon;
  tfn::TransferFunction fcn(fname);
  ospray::cpp::TransferFunction transferFunction("piecewise_linear");
  auto colorsData = ospray::cpp::Data(fcn.rgbValues.size(), OSP_FLOAT3,
                                      fcn.rgbValues.data());
  transferFunction.set("colors", colorsData);

  const float tf_scale = fcn.opacityScaling;
  // Sample the opacity values, taking 256 samples to match the volume viewer
  // the volume viewer does the sampling a bit differently so we match that
  // instead of what's done in createDefault
  std::vector<float> opacityValues;
  const int N_OPACITIES = 256;
  size_t lo = 0;
  size_t hi = 1;
  for (int i = 0; i < N_OPACITIES; ++i) {
    const float x = float(i) / float(N_OPACITIES - 1);
    float opacity = 0;
    if (i == 0) {
      opacity = fcn.opacityValues[0].y;
    } else if (i == N_OPACITIES - 1) {
      opacity = fcn.opacityValues.back().y;
    } else {
      // If we're over this val, find the next segment
      if (x > fcn.opacityValues[lo].x) {
        for (size_t j = lo; j < fcn.opacityValues.size() - 1; ++j) {
          if (x <= fcn.opacityValues[j + 1].x) {
            lo = j;
            hi = j + 1;
            break;
          }
        }
      }
      const float delta = x - fcn.opacityValues[lo].x;
      const float interval = fcn.opacityValues[hi].x - fcn.opacityValues[lo].x;
      if (delta == 0 || interval == 0) {
        opacity = fcn.opacityValues[lo].y;
      } else {
        opacity = fcn.opacityValues[lo].y + delta / interval
          * (fcn.opacityValues[hi].y - fcn.opacityValues[lo].y);
      }
    }
    opacityValues.push_back(tf_scale * opacity);
  }

  auto opacityValuesData = ospray::cpp::Data(opacityValues.size(),
                                             OSP_FLOAT,
                                             opacityValues.data());
  transferFunction.set("opacities", opacityValuesData);
  transferFunction.set("valueRange", vec2f(fcn.dataValueMin, fcn.dataValueMax));
  transferFunction.commit();
  return transferFunction;
}

// Get an string environment variable
std::string getEnvString(const std::string &var) {
  return ospray::getEnvVar<std::string>(var).second;
}

}

// OSPRayScriptHandler definitions ////////////////////////////////////////////

namespace ospray {

namespace script {
  Module::Module(RegisterModuleFn registerMod, GetHelpFn getHelp)
    : registerMod(registerMod), getHelp(getHelp)
  {}
  void Module::registerModule(chaiscript::ChaiScript &engine) {
    registerMod(engine);
  }
  void Module::help() const {
    if (getHelp) {
      getHelp();
    }
  }

  static std::vector<Module> SCRIPT_MODULES;
  void register_module(RegisterModuleFn registerModule, GetHelpFn getHelp) {
    SCRIPT_MODULES.push_back(Module(registerModule, getHelp));
  }
}

OSPRayScriptHandler::OSPRayScriptHandler(OSPModel    model,
                                         OSPRenderer renderer,
                                         OSPCamera   camera) :
  model(model),
  renderer(renderer),
  camera(camera),
  chai(chaiscript::Std_Lib::library()),
  scriptRunning(false),
  lock(scriptMutex, std::defer_lock_t())
{
  registerScriptTypes();
  registerScriptFunctions();
  registerScriptObjects();

  std::stringstream ss;
  ss << "Commands available:" << endl << endl;
  ss << "exit       --> exit command mode" << endl;
  ss << "done       --> synonomous with 'exit'" << endl;
  ss << "quit       --> synonomous with 'exit'" << endl;
  ss << "run [file] --> execute a script file" << endl << endl;

  ss << "OSPRay viewer objects available:" << endl << endl;
  ss << "c --> Camera"   << endl;
  ss << "m --> Model"    << endl;
  ss << "r --> Renderer" << endl << endl;

  ss << "OSPRay API functions available:" << endl << endl;
  ss << "ospLoadModule(module_name)"                  << endl;
  ss << "ospSetString(object, id, string)"            << endl;
  ss << "ospSetObject(object, id, object)"            << endl;
  ss << "ospSet1f(object, id, float)"                 << endl;
  ss << "ospSet2f(object, id, float, float)"          << endl;
  ss << "ospSet3f(object, id, float, float, float)"   << endl;
  ss << "ospSet1i(object, id, int)"                   << endl;
  ss << "ospSet2i(object, id, int, int)"              << endl;
  ss << "ospSet3i(object, id, int, int, int)"         << endl;
  ss << "ospSetVoidPtr(object, id, ptr)"              << endl;
  ss << "ospRemoveParam(object, id)"                  << endl;
  ss << "ospCommit(object)"                           << endl;
  ss << "TransferFunction loadTransferFunction(file)" << endl;
  ss << "string getEnvString(env_var)"                << endl;
  ss << endl;

  helpText = ss.str();
}

OSPRayScriptHandler::~OSPRayScriptHandler()
{
  stop();
}

void OSPRayScriptHandler::runScriptFromFile(const std::string &file)
{
  if (scriptRunning) {
    throw runtime_error("Cannot execute a script file when"
                        " running interactively!");
  }

  try {
    chai.eval_file(file);
  } catch (const chaiscript::exception::eval_error &e) {
    cerr << "ERROR: script '" << file << "' executed with error(s):" << endl;
    cerr << "    " << e.what() << endl;
  }
}

void OSPRayScriptHandler::consoleLoop()
{
  using_history();

  do {
    std::string input = get_next_command();

    if (input == "done" || input == "exit" || input == "quit") {
      break;
    } else if (input == "help") {
      cout << helpText << endl;
      for (const auto &m : script::SCRIPT_MODULES) {
        m.help();
      }
      continue;
    } else {
      stringstream ss(input);
      string command, arg;
      ss >> command >> arg;
      if (command == "run") {
        runChaiFile(arg);
        continue;
      }
    }

    runChaiLine(input);

  } while (scriptRunning);

  scriptRunning = false;
  cout << "**** EXIT COMMAND MODE *****" << endl;
}

void OSPRayScriptHandler::runChaiLine(const std::string &line)
{
  lock.lock();
  try {
    chai.eval(line);
  } catch (const chaiscript::exception::eval_error &e) {
    cerr << e.what() << endl;
  }
  lock.unlock();
}

void OSPRayScriptHandler::runChaiFile(const std::string &file)
{
  lock.lock();
  try {
    chai.eval_file(file);
  } catch (const std::runtime_error &e) {
    cerr << e.what() << endl;
  }
  lock.unlock();
}

void OSPRayScriptHandler::start()
{
  stop();
  cout << "**** START COMMAND MODE ****" << endl << endl;
  cout << "Type 'help' to see available objects and functions." << endl;
  cout << endl;
  scriptRunning = true;
  thread = std::thread(&OSPRayScriptHandler::consoleLoop, this);
}

void OSPRayScriptHandler::stop()
{
  scriptRunning = false;
  if (thread.joinable()){
    thread.join();
  }
}

bool OSPRayScriptHandler::running()
{
  return scriptRunning;
}

chaiscript::ChaiScript &OSPRayScriptHandler::scriptEngine()
{
  if (scriptRunning)
    throw runtime_error("Cannot modify the script env while running!");

  return chai;
}

void OSPRayScriptHandler::registerScriptObjects()
{
  chai.add_global(chaiscript::var(model),    "m");
  chai.add_global(chaiscript::var(renderer), "r");
  chai.add_global(chaiscript::var(camera),   "c");
  chai.add_global_const(chaiscript::const_var(static_cast<int>(OSP_FB_COLOR)), "OSP_FB_COLOR");
  chai.add_global_const(chaiscript::const_var(static_cast<int>(OSP_FB_ACCUM)), "OSP_FB_ACCUM");
  chai.add_global_const(chaiscript::const_var(static_cast<int>(OSP_FB_DEPTH)), "OSP_FB_DEPTH");
  chai.add_global_const(chaiscript::const_var(static_cast<int>(OSP_FB_VARIANCE)), "OSP_FB_VARIANCE");
  for (auto &m : script::SCRIPT_MODULES) {
    m.registerModule(chai);
  }
}

void OSPRayScriptHandler::registerScriptTypes()
{
  chaiscript::ModulePtr m = chaiscript::ModulePtr(new chaiscript::Module());

  // Class types //

  chaiscript::utility::add_class<ospray::cpp::ManagedObject_T<>>(*m, "ManagedObject",
     {
       chaiscript::constructor<ospray::cpp::ManagedObject_T<>()>(),
       chaiscript::constructor<ospray::cpp::ManagedObject_T<>(OSPObject)>()
     },
     {
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const std::string &) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, int) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, int, int) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, int, int, int) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, float) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, float, float) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, float, float, float) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, double) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, double, double) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, double, double, double) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, void*) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, OSPObject) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const ospcommon::vec2i&) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const ospcommon::vec2f&) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const ospcommon::vec3i&) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const ospcommon::vec3f&) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const ospcommon::vec4f&) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const ospray::cpp::ManagedObject &) const>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &) const>(&ospray::cpp::ManagedObject::remove)), "remove"},
       {chaiscript::fun(&ospray::cpp::ManagedObject::commit), "commit"},
       {chaiscript::fun(&ospray::cpp::ManagedObject::object), "handle"}
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Camera>(*m, "Camera",
     {
       chaiscript::constructor<ospray::cpp::Camera(const std::string &)>(),
       chaiscript::constructor<ospray::cpp::Camera(const ospray::cpp::Camera &)>(),
       chaiscript::constructor<ospray::cpp::Camera(OSPCamera)>()
     },
     {
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Data>(*m, "Data",
     {
       chaiscript::constructor<ospray::cpp::Data(const ospray::cpp::Data &)>(),
       chaiscript::constructor<ospray::cpp::Data(OSPData)>()
     },
     {
     }
     );

  chaiscript::utility::add_class<ospray::cpp::FrameBuffer>(*m, "FrameBuffer",
     {
       chaiscript::constructor<ospray::cpp::FrameBuffer(const ospray::cpp::FrameBuffer &)>(),
       chaiscript::constructor<ospray::cpp::FrameBuffer(OSPFrameBuffer)>()
     },
     {
       {chaiscript::fun(static_cast<void (ospray::cpp::FrameBuffer::*)(ospray::cpp::PixelOp &) const>(&ospray::cpp::FrameBuffer::setPixelOp)), "setPixelOp"},
       {chaiscript::fun(static_cast<void (ospray::cpp::FrameBuffer::*)(OSPPixelOp) const>(&ospray::cpp::FrameBuffer::setPixelOp)), "setPixelOp"}
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Geometry>(*m, "Geometry",
     {
       chaiscript::constructor<ospray::cpp::Geometry(const std::string &)>(),
       chaiscript::constructor<ospray::cpp::Geometry(const ospray::cpp::Geometry &)>(),
       chaiscript::constructor<ospray::cpp::Geometry(OSPGeometry)>()
     },
     {
       {chaiscript::fun(static_cast<void (ospray::cpp::Geometry::*)(ospray::cpp::Material &) const>(&ospray::cpp::Geometry::setMaterial)), "setMaterial"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Geometry::*)(OSPMaterial) const>(&ospray::cpp::Geometry::setMaterial)), "setMaterial"}
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Light>(*m, "Light",
     {
       chaiscript::constructor<ospray::cpp::Light(const ospray::cpp::Light &)>(),
       chaiscript::constructor<ospray::cpp::Light(OSPLight)>()
     },
     {
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Material>(*m, "Material",
     {
       chaiscript::constructor<ospray::cpp::Material(const ospray::cpp::Material &)>(),
       chaiscript::constructor<ospray::cpp::Material(OSPMaterial)>()
     },
     {
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Model>(*m, "Model",
     {
       chaiscript::constructor<ospray::cpp::Model()>(),
       chaiscript::constructor<ospray::cpp::Model(const ospray::cpp::Model &)>(),
       chaiscript::constructor<ospray::cpp::Model(OSPModel)>()
     },
     {
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(ospray::cpp::Geometry &) const>(&ospray::cpp::Model::addGeometry)), "addGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(OSPGeometry) const>(&ospray::cpp::Model::addGeometry)), "addGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(ospray::cpp::Geometry &) const>(&ospray::cpp::Model::removeGeometry)), "removeGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(OSPGeometry) const>(&ospray::cpp::Model::removeGeometry)), "removeGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(ospray::cpp::Volume &) const>(&ospray::cpp::Model::addVolume)), "addVolume"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(OSPVolume) const>(&ospray::cpp::Model::addVolume)), "addVolume"}
     }
     );

  chaiscript::utility::add_class<ospray::cpp::PixelOp>(*m, "PixelOp",
     {
       chaiscript::constructor<ospray::cpp::PixelOp(const std::string &)>(),
       chaiscript::constructor<ospray::cpp::PixelOp(const ospray::cpp::PixelOp &)>(),
       chaiscript::constructor<ospray::cpp::PixelOp(OSPPixelOp)>()
     },
     {
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Renderer>(*m, "Renderer",
     {
       chaiscript::constructor<ospray::cpp::Renderer(const std::string &)>(),
       chaiscript::constructor<ospray::cpp::Renderer(const ospray::cpp::Renderer &)>(),
       chaiscript::constructor<ospray::cpp::Renderer(OSPRenderer)>()
     },
     {
       {chaiscript::fun(&ospray::cpp::Renderer::newMaterial), "newMaterial"},
       {chaiscript::fun(&ospray::cpp::Renderer::newLight), "newLight"}
     }
     );

  chaiscript::utility::add_class<ospray::cpp::TransferFunction>(*m, "TransferFunction",
     {
       chaiscript::constructor<ospray::cpp::TransferFunction(const std::string &)>(),
       chaiscript::constructor<ospray::cpp::TransferFunction(const ospray::cpp::TransferFunction &)>(),
       chaiscript::constructor<ospray::cpp::TransferFunction(OSPTransferFunction)>()
     },
     {
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Texture2D>(*m, "Texture2D",
     {
       chaiscript::constructor<ospray::cpp::Texture2D(const ospray::cpp::Texture2D &)>(),
       chaiscript::constructor<ospray::cpp::Texture2D(OSPTexture2D)>()
     },
     {
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Volume>(*m, "Volume",
     {
       chaiscript::constructor<ospray::cpp::Volume(const std::string &)>(),
       chaiscript::constructor<ospray::cpp::Volume(const ospray::cpp::Volume &)>(),
       chaiscript::constructor<ospray::cpp::Volume(OSPVolume)>()
     },
     {
     }
     );

  using namespace ospcommon;
  chaiscript::utility::add_class<vec2i>(*m, "vec2i",
     {
       chaiscript::constructor<vec2i(int x, int y)>(),
     },
     {
       {chaiscript::fun(static_cast<vec2i (*)(const vec2i &, const vec2i &)>(operator+)), "+"},
       {chaiscript::fun(static_cast<vec2i (*)(const vec2i &, const vec2i &)>(operator*)), "*"},
       {chaiscript::fun(static_cast<vec2i (*)(const vec2i &, const vec2i &)>(operator-)), "-"},
       {chaiscript::fun(static_cast<vec2i& (vec2i::*)(const vec2i &)>(&vec2i::operator=)), "="},
     }
     );

  chaiscript::utility::add_class<vec2f>(*m, "vec2f",
     {
       chaiscript::constructor<vec2f(float x, float y)>(),
     },
     {
       {chaiscript::fun(static_cast<vec2f (*)(const vec2f &, const vec2f &)>(operator+)), "+"},
       {chaiscript::fun(static_cast<vec2f (*)(const vec2f &, const vec2f &)>(operator*)), "*"},
       {chaiscript::fun(static_cast<vec2f (*)(const vec2f &, const vec2f &)>(operator-)), "-"},
       {chaiscript::fun(static_cast<vec2f& (vec2f::*)(const vec2f &)>(&vec2f::operator=)), "="},
     }
     );

  chaiscript::utility::add_class<vec3i>(*m, "vec3i",
     {
       chaiscript::constructor<vec3i(int x, int y, int z)>(),
     },
     {
       {chaiscript::fun(static_cast<vec3i (*)(const vec3i &, const vec3i &)>(operator+)), "+"},
       {chaiscript::fun(static_cast<vec3i (*)(const vec3i &, const vec3i &)>(operator*)), "*"},
       {chaiscript::fun(static_cast<vec3i (*)(const vec3i &, const vec3i &)>(operator-)), "-"},
       {chaiscript::fun(static_cast<vec3i& (vec3i::*)(const vec3i &)>(&vec3i::operator=)), "="},
     }
     );

  chaiscript::utility::add_class<vec3f>(*m, "vec3f",
     {
       chaiscript::constructor<vec3f(float x, float y, float z)>(),
     },
     {
       {chaiscript::fun(static_cast<vec3f (*)(const vec3f &, const vec3f &)>(operator+)), "+"},
       {chaiscript::fun(static_cast<vec3f (*)(const vec3f &, const vec3f &)>(operator*)), "*"},
       {chaiscript::fun(static_cast<vec3f (*)(const vec3f &, const vec3f &)>(operator-)), "-"},
       {chaiscript::fun(static_cast<vec3f& (vec3f::*)(const vec3f &)>(&vec3f::operator=)), "="},
     }
     );

  chaiscript::utility::add_class<vec4f>(*m, "vec4f",
     {
       chaiscript::constructor<vec4f(float x, float y, float z, float w)>(),
     },
     {
       {chaiscript::fun(static_cast<vec4f (*)(const vec4f &, const vec4f &)>(operator+)), "+"},
       {chaiscript::fun(static_cast<vec4f (*)(const vec4f &, const vec4f &)>(operator*)), "*"},
       {chaiscript::fun(static_cast<vec4f (*)(const vec4f &, const vec4f &)>(operator-)), "-"},
       {chaiscript::fun(static_cast<vec4f& (vec4f::*)(const vec4f &)>(&vec4f::operator=)), "="},
     }
     );

  chaiscript::utility::add_class<box3f>(*m, "box3f",
     {
       chaiscript::constructor<box3f()>(),
       chaiscript::constructor<box3f(const vec3f &lower, const vec3f &upper)>(),
     },
     {
     }
     );

  chai.add(m);

  // Inheritance relationships //

  // osp objects
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Camera>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Data>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::FrameBuffer>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Geometry>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Light>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Material>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Model>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::PixelOp>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Renderer>());
  chai.add(chaiscript::base_class<osp::ManagedObject,
                                    osp::TransferFunction>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Texture2D>());
  chai.add(chaiscript::base_class<osp::ManagedObject, osp::Volume>());

  // ospray::cpp objects
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Camera>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Data>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::FrameBuffer>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Geometry>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Light>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Material>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Model>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::PixelOp>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Renderer>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::TransferFunction>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Texture2D>());
  chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Volume>());
}

void OSPRayScriptHandler::registerScriptFunctions()
{
  chai.add(chaiscript::fun(&chaiospray::ospLoadModule),        "ospLoadModule"       );
  chai.add(chaiscript::fun(&chaiospray::ospSetString),         "ospSetString"        );
  chai.add(chaiscript::fun(&chaiospray::ospSetObject),         "ospSetObject"        );
  chai.add(chaiscript::fun(&chaiospray::ospSet1f),             "ospSet1f"            );
  chai.add(chaiscript::fun(&chaiospray::ospSet1i),             "ospSet1i"            );
  chai.add(chaiscript::fun(&chaiospray::ospSet2f),             "ospSet2f"            );
  chai.add(chaiscript::fun(&chaiospray::ospSet2i),             "ospSet2i"            );
  chai.add(chaiscript::fun(&chaiospray::ospSet3f),             "ospSet3f"            );
  chai.add(chaiscript::fun(&chaiospray::ospSet3i),             "ospSet3i"            );
  chai.add(chaiscript::fun(&chaiospray::ospSetVoidPtr),        "ospSetVoidPtr"       );
  chai.add(chaiscript::fun(&chaiospray::ospRemoveParam),       "ospRemoveParam"      );
  chai.add(chaiscript::fun(&chaiospray::ospCommit),            "ospCommit"           );
  chai.add(chaiscript::fun(&chaiospray::loadTransferFunction), "loadTransferFunction");
  chai.add(chaiscript::fun(&chaiospray::getEnvString),         "getEnvString"        );
}

}// namespace ospray
