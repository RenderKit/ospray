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

#include "Managed.h"
#include "Data.h"
#include "OSPCommon_ispc.h"

namespace ospray {

  /*! \brief constructor */
  ManagedObject::ManagedObject()
    : ID(-1), ispcEquivalent(NULL), managedObjectType(OSP_UNKNOWN) 
  {}

  /*! \brief destructor */
  ManagedObject::~ManagedObject() 
  {
    // it is OK to potentially delete NULL, nothing bad happens ==> no need to check
    ispc::delete_uniform(ispcEquivalent);
    ispcEquivalent = NULL;
  }

  /*! \brief commit the object's outstanding changes (such as changed parameters etc) */
  void ManagedObject::commit() 
  {}

  //! \brief common function to help printf-debugging 
  /*! \detailed Every derived class should overrride this! */
  std::string ManagedObject::toString() const 
  {
    return "ospray::ManagedObject"; 
  }

  //! \brief register a new listener for given object
  /*! \detailed this object will now get update notifications from us */
  void ManagedObject::registerListener(ManagedObject *newListener)
  {
    objectsListeningForChanges.insert(newListener); 
  }

  //! \brief un-register a listener
  /*! \detailed this object will no longer get update notifications from us  */
  void ManagedObject::unregisterListener(ManagedObject *noLongerListening)
  {
    objectsListeningForChanges.erase(noLongerListening); 
  }

  /*! \brief gets called whenever any of this node's dependencies got changed */
  void ManagedObject::dependencyGotChanged(ManagedObject *object) 
  {}

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
  }

  void *ManagedObject::getVoidPtr(const char *name, void * valIfNotFound)
  {
    Param *param = findParam(name);                                     
    if (!param) return valIfNotFound;                                   
    if (param->type != OSP_VOID_PTR) return valIfNotFound;                
    return (void*)param->ptr;                                            
  }

  ManagedObject::Param *ManagedObject::findParam(const char *name,
                                                 bool addIfNotExist)
  {
    for (size_t i=0 ; i < paramList.size() ; i++) {
      if (!strcmp(paramList[i]->name,name)) return paramList[i];
    }
    if (!addIfNotExist) return NULL;
    paramList.push_back(new Param(name));
    return paramList[paramList.size()-1];
  }

#define define_getparam(T,ABB,TARGETTYPE,FIELD)                     \
  T ManagedObject::getParam##ABB(const char *name, T valIfNotFound) \
  {                                                                 \
    Param *param = findParam(name);                                 \
    if (!param) return valIfNotFound;                               \
    if (param->type != TARGETTYPE) return valIfNotFound;            \
    return (T&)param->FIELD;                                        \
  }
  
  define_getparam(ManagedObject *, Object, OSP_OBJECT, ptr);
  define_getparam(int32,  1i, OSP_INT,    i);
  define_getparam(vec3i,  3i, OSP_INT3,   i);
  define_getparam(vec3f,  3f, OSP_FLOAT3, f);
  define_getparam(vec3fa, 3f, OSP_FLOAT3, f);
  define_getparam(vec4f,  4f, OSP_FLOAT4, f);
  define_getparam(vec2f,  2f, OSP_FLOAT2, f);
  define_getparam(float,  1f, OSP_FLOAT,  f);
  define_getparam(float,  f,  OSP_FLOAT,  f);
  define_getparam(const char *, String, OSP_STRING, ptr);

#undef define_getparam
  
  void ManagedObject::setParam(const char *name, ManagedObject *data)
  {
    ManagedObject::Param *p = findParam(name,true);
    p->set(data);
  }

  /*!< call 'dependencyGotChanged' on each of the objects in 'objectsListeningForChanges' */
  void ManagedObject::notifyListenersThatObjectGotChanged() 
  {
    for (auto it = objectsListeningForChanges.begin();
         it != objectsListeningForChanges.end(); it++)  {
      ManagedObject *object = *it;
      object->dependencyGotChanged(this);
    }
  }

  void ManagedObject::emitMessage(const std::string &kind,
                           const std::string &message) const
  {
    std::cerr << "  " + toString()
              << "  " + kind + ": " + message + "." << std::endl;
  }

  void ManagedObject::exitOnCondition(bool condition,
                               const std::string &message) const
  {
    if (!condition)
      return;
    emitMessage("ERROR", message);
    exit(1);
  }

  void ManagedObject::warnOnCondition(bool condition,
                               const std::string &message) const
  {
    if (!condition)
      return;

    emitMessage("WARNING", message);
  }


} // ::ospray
