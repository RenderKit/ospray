// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

  ManagedObject::Param::Param(const char *_name)
    : name(_name)
  {
    Assert(_name);
  }

  ManagedObject::Param::~Param()
  {
    if (data.is<ManagedObject*>())
      data.get<ManagedObject*>()->refDec();
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

#define define_getparam(T,ABB)                                      \
  T ManagedObject::getParam##ABB(const char *name, T valIfNotFound) \
  {                                                                 \
    Param *param = findParam(name);                                 \
    if (!param) return valIfNotFound;                               \
    if (!param->data.is<T>()) return valIfNotFound;                 \
    return param->data.get<T>();                                    \
  }

  define_getparam(ManagedObject *, Object)
  define_getparam(std::string,     String)
  define_getparam(void*,           VoidPtr)

  define_getparam(int32,  1i)
  define_getparam(vec3i,  3i)
  define_getparam(vec3f,  3f)
  define_getparam(vec3fa, 3f)
  define_getparam(vec4f,  4f)
  define_getparam(vec2f,  2f)
  define_getparam(float,  1f)
  define_getparam(float,  f)

#undef define_getparam

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
