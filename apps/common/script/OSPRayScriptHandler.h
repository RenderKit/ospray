// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include <ospray_cpp/Camera.h>
#include <ospray_cpp/Data.h>
#include <ospray_cpp/FrameBuffer.h>
#include <ospray_cpp/Geometry.h>
#include <ospray_cpp/Light.h>
#include <ospray_cpp/Material.h>
#include <ospray_cpp/Model.h>
#include <ospray_cpp/PixelOp.h>
#include <ospray_cpp/Renderer.h>
#include <ospray_cpp/Texture2D.h>
#include <ospray_cpp/TransferFunction.h>
#include <ospray_cpp/Volume.h>

// ChaiScript
#include "chaiscript/chaiscript.hpp"

namespace ospray {

// Protect some of the script module functionality under an additionaly namespace
namespace script {
  using RegisterTypesFn = void (*)(chaiscript::ChaiScript&);
  using RegisterObjectsFn = void (*)(chaiscript::ChaiScript&);
  using GetHelpFn = void (*)();

  class Module {
    RegisterTypesFn registerTypes;
    // We should only register the module types once per instance of the script engine
    // this kind of assumes that there will be only one script engine running, which
    // I guess is true.
    bool typesRegistered;
    RegisterObjectsFn registerObjects;
    GetHelpFn getHelp;

  public:
    Module(RegisterTypesFn types, RegisterObjectsFn objs, GetHelpFn help);
    // Register the types, functions and objects exported by this module.
    //
    // The object registration will be run each time registerModule is called
    // but the types & functions registration will only be done once.
    void registerModule(chaiscript::ChaiScript &engine);
    void help() const;
  };

  //! \brief Setup a modules scripting registration callbacks to be called when a script
  //!        thread begins.
  //!
  //! registerTypes should register types and functions exported by the module
  //! registerObjects should register global variables exported by the module
  //! getHelp should print the modules help string, detailing functions/types/objects
  //!
  //! Any of the function pointers above can also be nullptr in which case they'll be ignored
  void register_module(RegisterTypesFn registerTypes, RegisterObjectsFn registerObjects,
                       GetHelpFn getHelp);
}

class OSPRayScriptHandler
{
public:

  OSPRayScriptHandler(OSPModel model, OSPRenderer renderer, OSPCamera camera);
  ~OSPRayScriptHandler();

  void runScriptFromFile(const std::string &file);

  void start();
  void stop();

  bool running();

protected:

  //! \brief ChaiScript engine, only accessable if the interactive thread isn't
  //!        running.
  //!
  //! \note Allow anyone who extends (inherits from) OSPRayScriptHandler to
  //!       have access to the engine to let them add custom functions or types.
  chaiscript::ChaiScript &scriptEngine();

  //! \note Child classes should append this string with any additional help
  //!       text that is desired when 'help' is invoked in the script engine.
  std::string m_helpText;

private:

  void consoleLoop();

  void runChaiLine(const std::string &line);
  void runChaiFile(const std::string &file);

  void registerScriptObjects();
  void registerScriptTypes();
  void registerScriptFunctions();

  // Data //

  cpp::Model    m_model;
  cpp::Renderer m_renderer;
  cpp::Camera   m_camera;

  chaiscript::ChaiScript m_chai;

  bool m_running;

  //! \brief background thread to handle the scripting commands from the console
  std::thread m_thread;
};

}// namespace ospray
