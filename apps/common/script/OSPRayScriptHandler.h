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

#pragma once

#include <thread>
#include <mutex>

#include <ospray/ospray_cpp/Camera.h>
#include <ospray/ospray_cpp/Data.h>
#include <ospray/ospray_cpp/FrameBuffer.h>
#include <ospray/ospray_cpp/Geometry.h>
#include <ospray/ospray_cpp/Light.h>
#include <ospray/ospray_cpp/Material.h>
#include <ospray/ospray_cpp/Model.h>
#include <ospray/ospray_cpp/PixelOp.h>
#include <ospray/ospray_cpp/Renderer.h>
#include <ospray/ospray_cpp/Texture2D.h>
#include <ospray/ospray_cpp/TransferFunction.h>
#include <ospray/ospray_cpp/Volume.h>

// ChaiScript
#include "chaiscript/chaiscript.hpp"

#ifdef _WIN32
  #ifdef ospray_script_EXPORTS
    #define OSPSCRIPT_INTERFACE __declspec(dllexport)
  #else
    #define OSPSCRIPT_INTERFACE __declspec(dllimport)
  #endif
#else
  #define OSPSCRIPT_INTERFACE
#endif

namespace ospray {

// Protect some of the script module functionality under an additional namespace
namespace script {
  using RegisterModuleFn = void (*)(chaiscript::ChaiScript&);
  using GetHelpFn = void (*)();

  class OSPSCRIPT_INTERFACE Module {
    RegisterModuleFn registerMod;
    GetHelpFn getHelp;

  public:
    Module(RegisterModuleFn registerMod, GetHelpFn getHelp);
    // Register the types, functions and objects exported by this module.
    void registerModule(chaiscript::ChaiScript &engine);
    // Print the modules help string via getHelp if a help callback was registered.
    void help() const;
  };

  //! \brief Setup a modules scripting registration callbacks to be called when a script
  //!        thread begins or requests help.
  //!
  //! registerModule Register types, functions and global variables exported by the module
  //! getHelp Print the modules help string to cout, detailing functions/types/objects. getHelp
  //          can be null if no help text will be provided.
  OSPSCRIPT_INTERFACE void register_module(RegisterModuleFn registerModule, GetHelpFn getHelp);
}

class OSPSCRIPT_INTERFACE OSPRayScriptHandler
{
public:

  OSPRayScriptHandler(OSPModel model, OSPRenderer renderer, OSPCamera camera);
  virtual ~OSPRayScriptHandler();

  void runScriptFromFile(const std::string &file);

  void start();
  void stop();

  bool running();

  //! \brief Mutex that can be used to protect against scripts trampling
  //!        scene data that is actively being used for rendering. Acquiring
  //!        this mutex will block scripts from running until it's released.
  std::mutex scriptMutex;

protected:

  //! \brief ChaiScript engine, only accessable if the interactive thread isn't
  //!        running.
  //!
  //! \note Allow anyone who extends (inherits from) OSPRayScriptHandler to
  //!       have access to the engine to let them add custom functions or types.
  chaiscript::ChaiScript &scriptEngine();

  //! \note Child classes should append this string with any additional help
  //!       text that is desired when 'help' is invoked in the script engine.
  std::string helpText;

  //! \brief This scripts unique lock managing locking/unlocking of the scriptMutex
  std::unique_lock<std::mutex> lock;

private:

  void consoleLoop();

  void runChaiLine(const std::string &line);
  void runChaiFile(const std::string &file);

  void registerScriptObjects();
  void registerScriptTypes();
  void registerScriptFunctions();

  // Data //

  cpp::Model    model;
  cpp::Renderer renderer;
  cpp::Camera   camera;

  chaiscript::ChaiScript chai;

  bool scriptRunning;

  //! \brief background thread to handle the scripting commands from the console
  std::thread thread;
};

}// namespace ospray
