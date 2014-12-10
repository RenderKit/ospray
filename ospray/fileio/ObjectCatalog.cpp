// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "ospray/fileio/ObjectCatalog.h"

namespace ospray {

  ObjectCatalog::ObjectCatalog(const char *name, 
                               ospray::ManagedObject *object) 
  {
    //! Initialization.
    this->name = strdup(name);  this->object = (OSPObject) object;  this->value = NULL;

    //! The subtype of this catalog is the subtype of the ManagedObject.
    this->type = object->managedObjectType;

    //! Add a catalog entry for each OSPRay object parameter.
    std::vector<OSPObjectCatalog> entries;  for (size_t i=0 ; i < object->paramList.size() ; i++) append(entries, object->paramList[i]);

    //! Copy the object entries into the catalog.
    this->entries = new OSPObjectCatalog[entries.size() + 1]();  memcpy(this->entries, &entries[0], entries.size() * sizeof(OSPObjectCatalog));

  }

  void ObjectCatalog::append(std::vector<OSPObjectCatalog> &entries, 
                             ospray::ManagedObject::Param *attribute) 
  {
    //! Attribute pointer could be NULL.
    if (attribute == NULL) return;

    //! The attribute is itself an OSPRay object.
    else if (attribute->type == OSP_OBJECT) entries.push_back(new ObjectCatalog(attribute->name, attribute->ptr));

    //! The attribute is a C-style string.
    else if (attribute->type == OSP_STRING) entries.push_back(new ObjectCatalog(attribute->name, OSP_STRING, attribute->s));

    //! The attribute is a void pointer.
    else if (attribute->type == OSP_VOID_PTR) entries.push_back(new ObjectCatalog(attribute->name, OSP_VOID_PTR, attribute->ptr));

    //! The attribute is a scalar or vector value.
    else entries.push_back(new ObjectCatalog(attribute->name, attribute->type, &attribute->f[0]));

  }

} // ::ospray

