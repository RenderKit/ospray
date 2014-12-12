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

#pragma once

// ospray
#include "ospray/include/ospray/ospray.h"
#include "ospray/common/Managed.h"
// stl
#include <string.h>
#include <vector>

namespace ospray {

  //! \brief The ObjectCatalog class stores the user specified parameters
  //!  associated with one or multiple OSPRay objects, and is intended to
  //!  allow introspection of these parameters at the application level.
  //!
  //!  Note that the parameters associated with an OSPRay object can be a
  //!  mix of data values and pointers to other OSPRay objects (each with
  //!  an associated ObjectCatalog).  
  //!
  class ObjectCatalog : public osp::ObjectCatalog, public ospray::ManagedObject {
  public:

    //! Constructor for an object type entry.
    ObjectCatalog(const char *name, ospray::ManagedObject *object);

    //! Constructor for an empty catalog.
    ObjectCatalog()
    { this->name = NULL;  this->object = NULL;  this->type = OSP_OBJECT_CATALOG;  this->value = NULL;  this->entries = new OSPObjectCatalog[1](); }

    //! Destructor.
    virtual ~ObjectCatalog()
    { for (size_t i=0 ; entries[i] ; i++) delete entries[i];  if (name != NULL) free(name); }

    //! Commit state for the enclosed OSPRay objects.
    virtual void commit()
    { for (size_t i=0 ; entries[i] ; i++) ((ObjectCatalog *) entries[i])->commit();  if (object) ospCommit(object); }

  protected:

    //! Constructor for a value type entry.
    ObjectCatalog(const char *name, OSPDataType type, const void *value)
    { this->name = strdup(name);  this->object = NULL;  this->type = type;  this->value = value;  this->entries = new OSPObjectCatalog[1](); }

    //! Add a catalog entry for an OSPRay object parameter.
    void append(std::vector<OSPObjectCatalog> &entries, ospray::ManagedObject::Param *attribute);

  };

} // ::ospray

