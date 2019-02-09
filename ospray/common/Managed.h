// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

// ospcommon
#include "ospcommon/utility/Any.h"
#include "ospcommon/utility/ParameterizedObject.h"
// ospray
#include "ospray/OSPDataType.h"
#include "common/OSPCommon.h"
#include "common/ObjectHandle.h"
// stl
#include <vector>
#include <set>

namespace ospray {

  /*! forward-def so param can use a pointer to data */
  struct Data;

  /*! \brief defines a basic object whose lifetime is managed by ospray

    One of the core concepts of ospray is that all logical
    objects in ospray -- renderers, geometries, models, camera, data
    buffers, volumes, etc -- are all derived from the same basic
    class that provides a certain kind of common, shared
    infrastructure:

    <dl>

    <dt>Reference counting</dt><dd> All ospray objects are reference
    counted, and will automatically live as long as any other object
    (and/or the application) still need it. In particular, even
    calling ospRelease on an object does *not* immediately destroy that
    object if other objects still use it.  </dd>

    <dt>C/ISPC correspondence</dt><dd>Most (though possibly not all!)
    objects in ospray will have components that operate in the C++
    side (typically if they require C++ features like templates or
    inhertiance, or for distribution/MPI issues), and others that live
    on the ISPC side of things (typically lower-level kernels that are
    performance-critical and require vectorization. Thanks to some
    limiations of ISPC this interplay is implemented by a one-on-one
    correspondence where a C++ side ospray object also has an ISPC side
    struct (with function pointers rather than virtual functions) that
    it maintains and updates as required (with "big" things like data
    arrays etc being shared by common pointer). To do this any ospray
    object has an (optional) pointer to a possibly exisitng ISPC
    counterpart (its "ISPC equivalent", or IE). For objects that do not
    have an IE, this value is nullptr; typically, the IE will also have a
    pointer back to its "C equivalent" (CE); and typically, it will be
    the C side that is doing creation, destruction, etc. Since (due to
    some internal ISPC issues) many of the ISPC types we use cannot be
    exported to the C side this IE pointer will be 'void *', always
    understanding that there is typically a non-trivial ISPC struct on
    the other side, and always assuming that the C side class only
    calls functions that know what type this pointer is.</dd>

    <dt>parametrization</dt><dd>The third core concept common to _all_
    ospray objects is that they contain a list of named and typed
    "parameters" of certain predefined types, including a type that
    allows to point to other ospray objects (data buffers are simply
    one instance of such an ospray object). OSPRay itself does not
    know what kind of parameters any specific object may or may not
    need; it simply attaches them to the object, and that object has
    to know by itself what to do with it.</dd>

    <dt>committing and modifying objects</dt><dd>Parameters attached
    to an object are supposed to be only used by an object only after
    an explicit 'ospCommit' has been called on said object. At that
    stage, the object is expected to "parse" the parameters and do
    with those values whatever it needs to be doing with them. E.g., a
    triangle mesh will assume data buffers with names "position" and
    "index" to be attached to them, and upon 'commit' will look for
    these buffers, and copy the (reference-counted) pointers to those
    buffers for use during rendering; the parameters themselves will
    never be accessed other than during 'commit'.</dd>

    <dt>type abstraction</dt><dd>One particular goal of the
    parameterization is that virtually all functionality in ospray is
    abstracted. In particular the API differs between things that are
    fundamentally different in functionality -- such as a renderer is
    different from a camera or a geometry -- but does not specify at
    all what exact variations of these different concepts are
    there. For example, for ospray a camera is simply a camera:
    whether its a perspective or a depth-of-field camera is specified
    only in the 'type' parameter (a string) to 'ospNewCamera'; and
    whatever parameters that camera may need to properly operate have
    to be specified via parameters, without the API even knowing what
    those parameters that might be.</dd>

    </dl>

   */
  struct OSPRAY_SDK_INTERFACE ManagedObject
    : public memory::RefCount,
      public utility::ParameterizedObject
  {
    using OSP_PTR = ManagedObject*;

    ManagedObject() = default;

    /*! \brief destructor, frees the ISPC-side allocated memory pointed to by
     * ispcEquivalent, thus derived classes do not need to and must not delete
     * their ispcEquivalent. This also means that derived classes most often do
     * not need an own destructor. */
    virtual ~ManagedObject() override;

    /*! \brief commit the object's outstanding changes (such as changed
     *         parameters etc) */
    virtual void commit();

    //! \brief common function to help printf-debugging
    /*! \detailed Every derived class should override this! */
    virtual std::string toString() const;

    /*! return the ISPC equivalent of this class */
    void *getIE() const;

    /*! \brief find the named parameter, and return its object value if
        available; else return 'default' value

        The returned managed object will *not* automatically
        have its refcount increased; it is up to the callee to
        properly do that (typically by assigning to a proper 'ref'
        instance */
    ManagedObject *getParamObject(const char *name,
                                  ManagedObject *valIfNotFound = nullptr);

    Data *getParamData(const char *name, Data *valIfNotFound = nullptr);

    vec4f  getParam4f(const char *name, vec4f  valIfNotFound);
    vec3fa getParam3f(const char *name, vec3fa valIfNotFound);
    vec3f  getParam3f(const char *name, vec3f  valIfNotFound);
    vec3i  getParam3i(const char *name, vec3i  valIfNotFound);
    vec2f  getParam2f(const char *name, vec2f  valIfNotFound);
    int32  getParam1i(const char *name, int32  valIfNotFound);
    float  getParam1f(const char *name, float  valIfNotFound);
    float  getParamf (const char *name, float  valIfNotFound);

    void *getParamVoidPtr(const char *name, void * valIfNotFound);
    std::string getParamString(const char *name,
                               std::string valIfNotFound = "");

    // ------------------------------------------------------------------
    // functions to allow objects (called a 'listener') to track
    // changes in one or more other objects (called a
    // 'dependencies'). Listeners can register themselves as listneres
    // with all their dependencies, and get 'notify'd whenever their
    // dependencies got changed/committed.
    // ------------------------------------------------------------------

    /*! \brief gets called whenever any of this node's dependencies got
     *         changed */
    virtual void dependencyGotChanged(ManagedObject *object);

    //! \brief Will notify all listeners that we got changed
    /*! \detailed will call 'dependencyGotChanged' on each of the
        objects in 'objectsListeningForChanges' */
    void notifyListenersThatObjectGotChanged();

    //! \brief register a new listener for given object
    /*! \detailed this object will now get update notifications from us */
    void registerListener(ManagedObject *newListener);

    //! \brief un-register a listener
    /*! \detailed this object will no longer get update notifications from us */
    void unregisterListener(ManagedObject *noLongerListening);

    // Data members //

    //! \brief List of managed objects that want to get notified
    //! whenever this object get changed (ie, committed). */
    /*! \detailed When this object gets committed, it will call
       'depedencyGotChanged' on each of the objects in this list. It
       is up to the respective objects to properly register and
       register themselves as dependencies.  \warning Objects here are
       *NOT* refcounted to avoid cyclical referencing; this means that
       every object that has registered itself somewhere absolutely
       _has_ to properly unregister itself as a listenere before it
       dies */
    std::set<ManagedObject *> objectsListeningForChanges;

    /*! \brief a global ID that can be used for referencing an object remotely*/
    id_t ID {(id_t)-1};

    /*! \brief ISPC-side equivalent of this C++-side class, if available
     *         (nullptr if not) */
    void *ispcEquivalent {nullptr};

    /*! \brief subtype of this ManagedObject */
    OSPDataType managedObjectType {OSP_UNKNOWN};
  };

  // Inlined ManagedObject definitions ////////////////////////////////////////

  inline void* ManagedObject::getIE() const
  {
    return ispcEquivalent;
  }

  inline Data*
  ManagedObject::getParamData(const char *name, Data *valIfNotFound)
  {
    return (Data*)getParamObject(name,(ManagedObject*)valIfNotFound);
  }

} // ::ospray

// Specializations for ISPCDevice /////////////////////////////////////////////

namespace ospcommon {
  namespace utility {

    template <>
    inline void
    ParameterizedObject::Param::set(const ospray::ManagedObject::OSP_PTR &object)
    {
      using OSP_PTR = ospray::ManagedObject::OSP_PTR;

      if (object) object->refInc();

      if (data.is<OSP_PTR>()) {
        auto *existingObj = data.get<OSP_PTR>();
        if (existingObj != nullptr)
          existingObj->refDec();
      }

      data = object;
    }

  } // ::ospcommon::utility
} // ::ospcommon
