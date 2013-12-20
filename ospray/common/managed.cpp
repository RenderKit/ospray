#include "managed.h"
#include "data.h"

namespace ospray {
  void ManagedObject::Param::set(ManagedObject *object)
  {
    clear();
    if (object) object->refInc();
    ptr  = object;
    type = OSP_OBJECT;
  }
  void ManagedObject::Param::clear()
  {
    if (type == OSP_OBJECT && ptr)
      ptr->refDec();
    type = OSP_OBJECT;
    ptr = NULL;
  }
  ManagedObject::Param::Param(const char *name)  
    : name(NULL), type(OSP_OBJECT), ptr(NULL) 
  {
    Assert(name);
    if (name) this->name = strdup(name);
  };

  ManagedObject::Param *ManagedObject::findParam(const char *name, bool addIfNotExist)
  {
    for (unsigned i=0;i<paramList.size();i++) {
      if (!strcmp(paramList[i]->name,name)) return paramList[i];
    }
    if (!addIfNotExist) return NULL;
    paramList.push_back(new Param(name));
    return paramList[paramList.size()-1];
  }
  
  void   ManagedObject::setParam(const char *name, Data *data)
  {
    ManagedObject::Param *p = findParam(name,true);
    p->set(data);
  }

}
