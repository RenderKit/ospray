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

#include "RawVolumeFile.h"
#include "ObjectFile.h"
#include "TinyXML2.h"
// stl
#include <map>

//! \brief A concrete implementation of the ObjectFile class
//!  for loading collections of OSPRay objects stored in XML format.
//!
//!  The loader creates and configures OSPRay object state according
//!  to the XML specification.  Importantly this object state is not
//!  committed, to allow an application to set additional parameters
//!  through the returned ObjectCatalog type.
//!
class OSPObjectFile : public ObjectFile {
public:

  //! Constructor.
  OSPObjectFile(const std::string &filename) : filename(filename) {}

  //! Destructor.
  virtual ~OSPObjectFile() {};

  //! Import a collection of objects.
  virtual OSPObject *importObjects();

  //! A string description of this class.
  virtual std::string toString() const { return("ospray_module_loaders::OSPObjectFile"); }

private:

  //! Path to the file containing the object state.
  std::string filename;

  //! Import a scalar float.
  void importAttributeFloat(const tinyxml2::XMLNode *node, OSPObject parent);

  //! Import a 2-vector float.
  void importAttributeFloat2(const tinyxml2::XMLNode *node, OSPObject parent);

  //! Import a 3-vector float.
  void importAttributeFloat3(const tinyxml2::XMLNode *node, OSPObject parent);

  //! Import a signed scalar integer.
  void importAttributeInteger(const tinyxml2::XMLNode *node, OSPObject parent);

  //! Import a signed 3-vector integer.
  void importAttributeInteger3(const tinyxml2::XMLNode *node, OSPObject parent);

  //! Import a string.
  void importAttributeString(const tinyxml2::XMLNode *node, OSPObject parent);

  //! Import an object.
  OSPObject importObject(const tinyxml2::XMLNode *node);

  //! Import a light object.
  OSPLight importLight(const tinyxml2::XMLNode *root);

  //! Import a triangle mesh object.
  OSPGeometry importTriangleMesh(const tinyxml2::XMLNode *root);

  //! Import a volume object.
  OSPVolume importVolume(const tinyxml2::XMLNode *root);
  
};

