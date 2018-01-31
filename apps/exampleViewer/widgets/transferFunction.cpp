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

#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
#include <imgui.h>
#include <imconfig.h>

#include "ospcommon/math.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/TransferFunction.h"
#include "common/sg/common/Data.h"

#include "transferFunction.h"
#include <widgets/TransferFunctionWidget.h>

using namespace ospcommon;
namespace ospray {

TransferFunction::TransferFunction(std::shared_ptr<sg::TransferFunction> tfn) : transferFcn(tfn) 
{
    widget = new tfn::tfn_widget::TransferFunctionWidget(
	[&]()
	{
	    return this->transferFcn->child("numSamples").valueAs<int>();
	},
	[&](const std::vector<float>& c, const std::vector<float>& a)
	{
	    int sampleNum = this->transferFcn->child("numSamples").valueAs<int>();
	    auto colors =
	    ospray::sg::createNode("colors", "DataVector3f")
	    ->nodeAs<ospray::sg::DataVector3f>();
	    auto alpha  =
	    ospray::sg::createNode("alpha", "DataVector2f")
	    ->nodeAs<ospray::sg::DataVector2f>();
	    colors->v.resize(sampleNum);
	    alpha->v.resize(sampleNum);
	    std::copy(c.data(), c.data() + c.size(),
		      reinterpret_cast<float*>(colors->v.data()));
	    std::copy(a.data(), a.data() + a.size(),
		      reinterpret_cast<float*>(alpha->v.data()));
	    this->transferFcn->add(colors);
	    this->transferFcn->add(alpha);
	    colors->markAsModified();
	});
}
    
TransferFunction::TransferFunction(const TransferFunction &t) : transferFcn(t.transferFcn)
{
    widget = new tfn::tfn_widget::TransferFunctionWidget(
	[&]()
	{
	    return this->transferFcn->child("numSamples").valueAs<int>();
	},
	[&](const std::vector<float>& c, const std::vector<float>& a)
	{
	    int sampleNum = this->transferFcn->child("numSamples").valueAs<int>();
	    auto colors =
	    ospray::sg::createNode("colors", "DataVector3f")
	    ->nodeAs<ospray::sg::DataVector3f>();
	    auto alpha  =
	    ospray::sg::createNode("alpha", "DataVector2f")
	    ->nodeAs<ospray::sg::DataVector2f>();
	    colors->v.resize(sampleNum);
	    alpha->v.resize(sampleNum);
	    std::copy(c.data(), c.data() + c.size(),
		      reinterpret_cast<float*>(colors->v.data()));
	    std::copy(a.data(), a.data() + a.size(),
		      reinterpret_cast<float*>(alpha->v.data()));
	    this->transferFcn->add(colors);
	    this->transferFcn->add(alpha);
	    colors->markAsModified();
	});
}

TransferFunction::~TransferFunction()
{
    delete (tfn::tfn_widget::TransferFunctionWidget*)widget;
}

TransferFunction& TransferFunction::operator=(const TransferFunction &t)
{
  if (this == &t)
    return *this;
  transferFcn = t.transferFcn;
  widget = new tfn::tfn_widget::TransferFunctionWidget(
      [&]()
      {
	  return this->transferFcn->child("numSamples").valueAs<int>();
      },
      [&](const std::vector<float>& c, const std::vector<float>& a)
      {
	  int sampleNum = this->transferFcn->child("numSamples").valueAs<int>();
	  auto colors =
	  ospray::sg::createNode("colors", "DataVector3f")
	  ->nodeAs<ospray::sg::DataVector3f>();
	  auto alpha  =
	  ospray::sg::createNode("alpha", "DataVector2f")
	  ->nodeAs<ospray::sg::DataVector2f>();
	  colors->v.resize(sampleNum);
	  alpha->v.resize(sampleNum);
	  std::copy(c.data(), c.data() + c.size(),
		    reinterpret_cast<float*>(colors->v.data()));
	  std::copy(a.data(), a.data() + a.size(),
		    reinterpret_cast<float*>(alpha->v.data()));
	  this->transferFcn->add(colors);
	  this->transferFcn->add(alpha);
	  colors->markAsModified();
      });

  return *this;
}

bool TransferFunction::drawUI()
{
  return static_cast<tfn::tfn_widget::TransferFunctionWidget*>(widget)->drawUI();
}

void TransferFunction::render()
{
  static_cast<tfn::tfn_widget::TransferFunctionWidget*>(widget)->render();
}

void TransferFunction::load(const ospcommon::FileName &fileName)
{
  static_cast<tfn::tfn_widget::TransferFunctionWidget*>(widget)->load(fileName.str());
}

void TransferFunction::save(const ospcommon::FileName &fileName) const
{
  static_cast<tfn::tfn_widget::TransferFunctionWidget*>(widget)->save(fileName.str());
}

}// ::ospray

