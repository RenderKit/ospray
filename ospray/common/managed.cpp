/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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
  void ManagedObject::Param::set(void *ptr)
  {
    Assert2(this,"trying to set null parameter");
    clear();
    (void*&)this->ptr = ptr;
    type     = OSP_VOID_PTR;
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
    : name(NULL), type(OSP_FLOAT), ptr(NULL) 
  {
    Assert(name);
    f[0] = 0;
    f[1] = 0;
    f[2] = 0;
    f[3] = 0;
    if (name) this->name = strdup(name);
  };

  void *ManagedObject::getVoidPtr(const char *name, void * valIfNotFound) 
  {
    Param *param = findParam(name);                                     
    if (!param) return valIfNotFound;                                   
    if (param->type != OSP_VOID_PTR) return valIfNotFound;                
    return (void*)param->ptr;                                            
  }

  ManagedObject::Param *ManagedObject::findParam(const char *name, bool addIfNotExist)
  {
    for (size_t i=0 ; i < paramList.size() ; i++) {
      if (!strcmp(paramList[i]->name,name)) return paramList[i];
    }
    if (!addIfNotExist) return NULL;
    paramList.push_back(new Param(name));
    return paramList[paramList.size()-1];
  }

#define define_getparam(T,ABB,TARGETTYPE,FIELD)                       \
  T ManagedObject::getParam##ABB(const char *name, T valIfNotFound) { \
  Param *param = findParam(name);                                     \
  if (!param) return valIfNotFound;                                   \
  if (param->type != TARGETTYPE) return valIfNotFound;                \
  return (T&)param->FIELD;                                            \
  }
  
  define_getparam(ManagedObject *,Object,OSP_OBJECT,ptr);
  define_getparam(int32, 1i,OSP_INT,i);
  define_getparam(vec3i, 3i,OSP_INT3,i);
  define_getparam(vec3f, 3f,OSP_FLOAT3,f);
  define_getparam(vec3fa,3f,OSP_FLOAT3,f);
  define_getparam(float, 1f,OSP_FLOAT, f);
  define_getparam(float, f, OSP_FLOAT, f);
  define_getparam(const char *, String, OSP_STRING, ptr);

#undef define_getparam
  
  void   ManagedObject::setParam(const char *name, ManagedObject *data)
  {
    ManagedObject::Param *p = findParam(name,true);
    p->set(data);
  }

}
