#include "managed.h"
#include "data.h"

namespace ospray {
  void ManagedObject::Param::set(ManagedObject *object)
  {
    Assert2(this,"trying to set null parameter");
    clear();
    if (object) object->refInc();
    ptr  = object;
    type = OSP_OBJECT;
  }
  void ManagedObject::Param::set(const char *str)
  {
    Assert2(this,"trying to set null parameter");
    clear();
    this->s  = strdup(str);
    type     = OSP_STRING;
  }
  void ManagedObject::Param::clear()
  {
    Assert2(this,"trying to clear null parameter");
    if (type == OSP_OBJECT && ptr)
      ptr->refDec();
    if (type == OSP_STRING && ptr)
      free(ptr);
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

  /*! @{ */
  /*! \brief find the named parameter, and return its object value if
    available; else return 'default' value 
    
    \detailed The returned managed object will *not* automatically
    have its refcount increased; it is up to the callee to
    properly do that (typically by assigning to a proper 'ref'
    instance */
  
#define define_getparam(T,ABB,TARGETTYPE,FIELD)                       \
  T ManagedObject::getParam##ABB(const char *name, T valIfNotFound) { \
  Param *param = findParam(name);                                     \
  if (!param) return valIfNotFound;                                   \
  if (param->type != TARGETTYPE) return valIfNotFound;                \
  return (T&)param->FIELD;                                            \
  }
  
  define_getparam(ManagedObject *,,OSP_OBJECT,ptr);
  define_getparam(vec3f, 3f,OSP_vec3f,f);
  define_getparam(vec3fa,3f,OSP_vec3f,f);
  define_getparam(float, f, OSP_FLOAT, f);
  define_getparam(const char *, String, OSP_STRING, ptr);

#undef define_getparam

  /*! @} */

  
  void   ManagedObject::setParam(const char *name, ManagedObject *data)
  {
    ManagedObject::Param *p = findParam(name,true);
    p->set(data);
  }

}
