// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include <common/commandline/CommandLineParser.h>

#include <ospray_cpp/Renderer.h>
#include <ospray_cpp/Light.h>

class LightsParser : public CommandLineParser
{
};

class DefaultLightsParser : public LightsParser
{
public:
  DefaultLightsParser(ospray::cpp::Renderer renderer);
  bool parse(int ac, const char **&av) override;

protected:

  ospray::cpp::Renderer m_renderer;

  /*! when using the OBJ renderer, we create a automatic dirlight with this
   * direction; use ''--sun-dir x y z' to change */
  ospcommon::vec3f m_defaultDirLight_direction;

private:

  void finalize();
};
