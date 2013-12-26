#pragma once

// ospray stuff
#include "ospcommon.h"
#include "ospray/include/ospray/ospray.h"
// stl stuff
#include <vector>

namespace ospray {

  /*! forward-def so param can use a pointer to data */
  struct Data;

  /*! defines a basic object whose lifetime is managed by ospray */
  struct ManagedObject : public embree::RefCount
  {
    struct Param {
      Param(const char *name);
      ~Param() { clear(); };
      void clear();
      void set(ManagedObject *ptr);

      union {
        float f[4];
        int   i[4];
        uint  ui[4]; 
        long  l;
        ManagedObject *ptr;
      };
      OSPDataType type;
      const char *name;
    };

    virtual std::string toString() const { return "ospray::ManagedObject"; }
    ManagedObject() : ID(-1) {};
    virtual ~ManagedObject() {};

    /*! find a given parameter, or add it if not exists (and so specified) */
    Param *findParam(const char *name, bool addIfNotExist = false);
    void   setParam(const char *name, Data *data);

    std::vector<Param *> paramList;
    id_t ID; /*!< a global ID that can be used for referencing an
               object remotely */
  };

}
