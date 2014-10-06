//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#pragma once

#include <string.h>
#include <vector>
#include "ospray/include/ospray/ospray.h"
#include "ospray/common/managed.h"

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

} // namespace ospray

