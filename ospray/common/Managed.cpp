// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "OSPCommon_ispc.h"

namespace ospray {

  /*! \brief destructor */
  ManagedObject::~ManagedObject() 
  {
    // it is OK to potentially delete nullptr, nothing bad happens ==> no need to check
    ispc::delete_uniform(ispcEquivalent);
    ispcEquivalent = nullptr;
  }

  /*! \brief commit the object's outstanding changes (i.e. changed parameters etc) */
  void ManagedObject::commit() 
  {
  }

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
  {
    UNUSED(object);
  }

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
    s = new std::string;
    *s = strdup(str);
    type    = OSP_STRING;
  }

  void ManagedObject::Param::set(void *ptr)
  {
    Assert2(this,"trying to set null parameter");
    clear();
    (void*&)this->ptr = ptr;
    type = OSP_VOID_PTR;
  }

  void ManagedObject::Param::clear()
  {
    Assert2(this,"trying to clear null parameter");
    if (type == OSP_OBJECT && ptr)
      ptr->refDec();
    if (type == OSP_STRING && s)
      delete s;
    type = OSP_OBJECT;
    ptr  = nullptr;
  }

  ManagedObject::Param::Param(const char *_name)
    : ptr(nullptr), type(OSP_FLOAT), name(strdup(_name))
  {
    Assert(_name);
  }

  void *ManagedObject::getVoidPtr(const char *name, void *valIfNotFound)
  {
    Param *param = findParam(name);                                     
    if (!param) return valIfNotFound;                                   
    if (param->type != OSP_VOID_PTR) return valIfNotFound;                
    return (void*)param->ptr;                                            
  }

  ManagedObject::Param *ManagedObject::findParam(const char *name,
                                                 bool addIfNotExist)
  {
    auto foundParam =
        std::find_if(paramList.begin(), paramList.end(),
          [&](const std::shared_ptr<Param> &p) {
            return p->name == name;
          });

    if (foundParam != paramList.end())
      return foundParam->get();
    else if (addIfNotExist) {
      paramList.push_back(std::make_shared<Param>(name));
      return paramList[paramList.size()-1].get();
    }
    else
      return nullptr;
  }

#define define_getparam(T,ABB,TARGETTYPE,FIELD)                     \
  T ManagedObject::getParam##ABB(const char *name, T valIfNotFound) \
  {                                                                 \
    Param *param = findParam(name);                                 \
    if (!param) return valIfNotFound;                               \
    if (param->type != TARGETTYPE) return valIfNotFound;            \
    return (T&)param->FIELD;                                        \
  }
  
  define_getparam(ManagedObject *, Object, OSP_OBJECT, ptr)
  define_getparam(int32,  1i, OSP_INT,     u_int)
  define_getparam(vec3i,  3i, OSP_INT3,    u_vec3i)
  define_getparam(vec3f,  3f, OSP_FLOAT3,  u_vec3f)
  define_getparam(vec3fa, 3f, OSP_FLOAT3A, u_vec3fa)
  define_getparam(vec4f,  4f, OSP_FLOAT4,  u_vec4f)
  define_getparam(vec2f,  2f, OSP_FLOAT2,  u_vec2f)
  define_getparam(float,  1f, OSP_FLOAT,   u_float)
  define_getparam(float,  f,  OSP_FLOAT,   u_float)

#undef define_getparam

  const char *ManagedObject::getParamString(const char *name,
                                            const char *valIfNotFound)
  {
    Param *param = findParam(name);
    if (!param) return valIfNotFound;
    if (param->type != OSP_STRING) return valIfNotFound;
    return param->s->c_str();
  }

  void ManagedObject::removeParam(const char *name)
  {
    auto foundParam =
        std::find_if(paramList.begin(), paramList.end(),
          [&](const std::shared_ptr<Param> &p) {
            return p->name == name;
          });

    if (foundParam != paramList.end()) {
      paramList.erase(foundParam);
    }
  }

  void ManagedObject::notifyListenersThatObjectGotChanged() 
  {
    for (auto *object : objectsListeningForChanges)
      object->dependencyGotChanged(this);
  }

  void ManagedObject::emitMessage(const std::string &kind,
                                  const std::string &message) const
  {
    postStatusMsg() << "  " << toString()
                   << "  " << kind << ": " << message + '.';
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
