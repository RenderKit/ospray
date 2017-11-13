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

using namespace ospcommon;
namespace ospray {

TransferFunction::TransferFunction(std::shared_ptr<sg::TransferFunction> tfn) :
  transferFcn(tfn),
  widget([&]()
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
	 })
{}

TransferFunction::TransferFunction(const TransferFunction &t) :
  transferFcn(t.transferFcn),
  widget([&]()
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
	 })
{}

TransferFunction::~TransferFunction()
{}

TransferFunction& TransferFunction::operator=(const TransferFunction &t)
{
  if (this == &t)
    return *this;
  transferFcn = t.transferFcn;
  return *this;
}

void TransferFunction::drawUi()
{
  widget.drawUi();
}

void TransferFunction::render()
{
  widget.render();
}

void TransferFunction::load(const ospcommon::FileName &fileName)
{
  widget.load(fileName.str());
}

void TransferFunction::save(const ospcommon::FileName &fileName) const
{
  widget.save(fileName.str());
}

}// ::ospray

