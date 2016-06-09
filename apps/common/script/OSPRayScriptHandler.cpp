#include "OSPRayScriptHandler.h"

#include "chaiscript/chaiscript_stdlib.hpp"
#include "chaiscript/utility/utility.hpp"

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
  if ( ! std::cin.eof() ) {
    char *input_raw = readline("% ");
    if ( input_raw ) {
      add_history(input_raw);

      std::string val(input_raw);
      size_t pos = val.find_first_not_of("\t \n");
      if (pos != std::string::npos)
      {
        val.erase(0, pos);
      }
      pos = val.find_last_not_of("\t \n");
      if (pos != std::string::npos)
      {
        val.erase(pos+1, std::string::npos);
      }

      retval = val;

      ::free(input_raw);
    }
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

void ospCommit(OSPObject object)
{
  ::ospCommit(object);
}

}

// OSPRayScriptHandler definitions ////////////////////////////////////////////

namespace ospray {

OSPRayScriptHandler::OSPRayScriptHandler(OSPModel    model,
                                         OSPRenderer renderer,
                                         OSPCamera   camera) :
  m_model(model),
  m_renderer(renderer),
  m_camera(camera),
  m_chai(chaiscript::Std_Lib::library()),
  m_running(false)
{
  registerScriptTypes();
  registerScriptFunctions();

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
  ss << "ospLoadModule(module_name)"                << endl;
  ss << "ospSetString(object, id, string)"          << endl;
  ss << "ospSetObject(object, id, object)"          << endl;
  ss << "ospSet1f(object, id, float)"               << endl;
  ss << "ospSet2f(object, id, float, float)"        << endl;
  ss << "ospSet3f(object, id, float, float, float)" << endl;
  ss << "ospSet1i(object, id, int)"                 << endl;
  ss << "ospSet2i(object, id, int, int)"            << endl;
  ss << "ospSet3i(object, id, int, int, int)"       << endl;
  ss << "ospSetVoidPtr(object, id, ptr)"            << endl;
  ss << "ospCommit(object)"                         << endl;
  ss << endl;

  m_helpText = ss.str();
}

OSPRayScriptHandler::~OSPRayScriptHandler()
{
  stop();
}

void OSPRayScriptHandler::runScriptFromFile(const std::string &file)
{
  if (m_running) {
    throw runtime_error("Cannot execute a script file when"
                        " running interactively!");
  }

  registerScriptObjects();

  try {
    m_chai.eval_file(file);
  } catch (const chaiscript::exception::eval_error &e) {
    cerr << "ERROR: script '" << file << "' executed with error(s):" << endl;
    cerr << "    " << e.what() << endl;
  }
}

void OSPRayScriptHandler::consoleLoop()
{
  registerScriptObjects();

  using_history();

  do {
    std::string input = get_next_command();

    if (input == "done" || input == "exit" || input == "quit") {
      break;
    } else if (input == "help") {
      cout << m_helpText << endl;
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

  } while (m_running);

  cout << "**** EXIT COMMAND MODE *****" << endl;
}

void OSPRayScriptHandler::runChaiLine(const std::string &line)
{
  try {
    m_chai.eval(line);
  } catch (const chaiscript::exception::eval_error &e) {
    cerr << e.what() << endl;
  }
}

void OSPRayScriptHandler::runChaiFile(const std::string &file)
{
  try {
    m_chai.eval_file(file);
  } catch (const std::runtime_error &e) {
    cerr << e.what() << endl;
  }
}

void OSPRayScriptHandler::start()
{
  stop();
  cout << "**** START COMMAND MODE ****" << endl << endl;
  cout << "Type 'help' to see available objects and functions." << endl;
  cout << endl;
  m_running = true;
  m_thread = std::thread(&OSPRayScriptHandler::consoleLoop, this);
}

void OSPRayScriptHandler::stop()
{
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();
}

bool OSPRayScriptHandler::running()
{
  return m_running;
}

chaiscript::ChaiScript &OSPRayScriptHandler::scriptEngine()
{
  if (m_running)
    throw runtime_error("Cannot modify the script env while running!");

  return m_chai;
}

void OSPRayScriptHandler::registerScriptObjects()
{
  m_chai.add(chaiscript::var(m_model),    "m");
  m_chai.add(chaiscript::var(m_renderer), "r");
  m_chai.add(chaiscript::var(m_camera),   "c");
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
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const std::string &)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, int)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, int, int)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, int, int, int)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, float)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, float, float)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, float, float, float)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, double)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, double, double)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, double, double, double)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, void*)>(&ospray::cpp::ManagedObject::set)), "set"},
       {chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, OSPObject)>(&ospray::cpp::ManagedObject::set)), "set"},
       //{chaiscript::fun(static_cast<void (ospray::cpp::ManagedObject::*)(const std::string &, const ospray::cpp::ManagedObject &)>(&ospray::cpp::ManagedObject::set)), "set"},
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
       {chaiscript::fun(static_cast<void (ospray::cpp::FrameBuffer::*)(ospray::cpp::PixelOp &)>(&ospray::cpp::FrameBuffer::setPixelOp)), "setPixelOp"},
       {chaiscript::fun(static_cast<void (ospray::cpp::FrameBuffer::*)(OSPPixelOp)>(&ospray::cpp::FrameBuffer::setPixelOp)), "setPixelOp"}
     }
     );

  chaiscript::utility::add_class<ospray::cpp::Geometry>(*m, "Geometry",
     {
       chaiscript::constructor<ospray::cpp::Geometry(const std::string &)>(),
       chaiscript::constructor<ospray::cpp::Geometry(const ospray::cpp::Geometry &)>(),
       chaiscript::constructor<ospray::cpp::Geometry(OSPGeometry)>()
     },
     {
       {chaiscript::fun(static_cast<void (ospray::cpp::Geometry::*)(ospray::cpp::Material &)>(&ospray::cpp::Geometry::setMaterial)), "setMaterial"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Geometry::*)(OSPMaterial)>(&ospray::cpp::Geometry::setMaterial)), "setMaterial"}
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
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(ospray::cpp::Geometry &)>(&ospray::cpp::Model::addGeometry)), "addGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(OSPGeometry)>(&ospray::cpp::Model::addGeometry)), "addGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(ospray::cpp::Geometry &)>(&ospray::cpp::Model::removeGeometry)), "removeGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(OSPGeometry)>(&ospray::cpp::Model::removeGeometry)), "removeGeometry"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(ospray::cpp::Volume &)>(&ospray::cpp::Model::addVolume)), "addVolume"},
       {chaiscript::fun(static_cast<void (ospray::cpp::Model::*)(OSPVolume)>(&ospray::cpp::Model::addVolume)), "addVolume"}
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

  m_chai.add(m);

  // Inheritance relationships //

  // osp objects
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Camera>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Data>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::FrameBuffer>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Geometry>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Light>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Material>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Model>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::PixelOp>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Renderer>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject,
                                    osp::TransferFunction>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Texture2D>());
  m_chai.add(chaiscript::base_class<osp::ManagedObject, osp::Volume>());

  // ospray::cpp objects
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Camera>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Data>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::FrameBuffer>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Geometry>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Light>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Material>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Model>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::PixelOp>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Renderer>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::TransferFunction>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Texture2D>());
  m_chai.add(chaiscript::base_class<ospray::cpp::ManagedObject,
                                    ospray::cpp::Volume>());
}

void OSPRayScriptHandler::registerScriptFunctions()
{
  m_chai.add(chaiscript::fun(&chaiospray::ospLoadModule), "ospLoadModule");
  m_chai.add(chaiscript::fun(&chaiospray::ospSetString),  "ospSetString" );
  m_chai.add(chaiscript::fun(&chaiospray::ospSetObject),  "ospSetObject" );
  m_chai.add(chaiscript::fun(&chaiospray::ospSet1f),      "ospSet1f"     );
  m_chai.add(chaiscript::fun(&chaiospray::ospSet1i),      "ospSet1i"     );
  m_chai.add(chaiscript::fun(&chaiospray::ospSet2f),      "ospSet2f"     );
  m_chai.add(chaiscript::fun(&chaiospray::ospSet2i),      "ospSet2i"     );
  m_chai.add(chaiscript::fun(&chaiospray::ospSet3f),      "ospSet3f"     );
  m_chai.add(chaiscript::fun(&chaiospray::ospSet3i),      "ospSet3i"     );
  m_chai.add(chaiscript::fun(&chaiospray::ospSetVoidPtr), "ospSetVoidPtr");
  m_chai.add(chaiscript::fun(&chaiospray::ospCommit),     "ospCommit"    );
}

}// namespace ospray
