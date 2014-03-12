#pragma once

// ospray stuff
#include "ospcommon.h"
#include "ospray/include/ospray/ospray.h"
// stl stuff
#include <vector>

namespace ospray {

  /*! forward-def so param can use a pointer to data */
  struct Data;

  /*! \brief defines a basic object whose lifetime is managed by ospray 

    \detailed One of the core concepts of ospray is that all logical
    objects in ospray --- renderers, geometries, models, camera, data
    buffers, volumes, etc --- are all derived from the same absic
    class that provides a certain kind of common, shared
    infrastructure:

    <dl> 

    <dt>Reference counting</dt><dd> All ospray objects are reference
    counted, and will automatically live as long as any other object
    (and/or the application) still need it. In particular, even
    calling ospFree on an object does *not* immediately destroy that
    object if other objects still use it.  </dd>

    <dt>C/ISPC correspondence</dt><dd>Most (though possibly not all!)
    objects in ospray will have components that operate in the C++
    side (typically if they require C++ features like templates or
    inhertiance, of for distribution/MPI issues), and others that live
    on the ISPC side of things (typically lower-level kernels that are
    performance-critical and require vectorization. Thanks to some
    limiations of ISPC this interplay is implemented by a one-on-one
    correspondence where a C++ side ospray object also has a ISPC side
    struct (with function pointers rather than virtual functions) that
    it maintains and updates as required (with "big" things like data
    arrays etc being shared by common pointer). To do this any ospray
    object has a (optional) pointer to a possibly exisitng ISCP
    counterpart (its "ISPC equivalent", or IE). For object that do not
    have an IE, this value is NULL; typically, the IE will also have a
    pointer back to its "C equivalet" (CE); and typically, it will be
    the C side that is doing creation, destruction, etc. Since (due to
    some internal ISPC issues) many of the ISPC types we use cannot be
    exported to the C side this IE pointer will be 'void *', always
    understanding that there is typically a non-trivial ISPC struct on
    the other side, and always assuming that the C side class only
    calls functions that know what type this pointer is. </dd>

    <dt>parametrization</dt><dd>The third core concept common to _all_
    ospray objects is that they contain a list of named and typed
    "parameters" of certain predefined type, including a type that
    allows to point to other ospray objects (data buffers are simply
    one instance of such an ospray object). OSPRay itseld does not
    know what kind of parameters any specific object may or may not
    need; it simply attaches them to the object, and that object has
    to know by itself what to do with it</dd>

    <dt>committing and modifying objects</dt><dd>Parameters attached
    to an object are supposed to be only used by an object only after
    an explicit 'ospCommit' has been called on said object. At that
    stage, the object is expected to "parse" the parameters and do
    with those values whatever it needs to be doing with them. Eg, a
    triangle mesh will assume data buffers with names "position" and
    "index" to be attached to them, and upon 'commit' will look for
    these buffers, and copy the (reference-counted) pointers to those
    buffers for use during rendering; the parameters themselves will
    never be accessed other than during 'commit'.  </dd>

    <dt>type abstraction</dt><dd>One particular goal of the
    parameterization is that virtually all functionality in ospray is
    abstracted. In particular the API differs between things that are
    fundmentally different in functionality---such as a renderer is
    different from a camera or a geometry---but does not specify at
    all what exact variations of these different concepts are
    there. For example, for ospray a camera is simply a camera:
    whether its a perspective or a depth-of-field camera is specified
    only in the 'type' parameter (a string) to 'ospNewCamera'; and
    whatever parameters that camera may need to properly operate have
    to be specified via parameters, without the api even knowing what
    those parameters that might be.  </dd>
    
    </dl>

   */
  struct ManagedObject : public embree::RefCount
  {
    struct Param {
      Param(const char *name);
      ~Param() { clear(); };
      /*! clear parameter to 'invalid type and value'; free/de-refcount data if reqd' */
      void clear();
      /*! set parameter to a 'pointer to object' type, and given pointer */
      void set(ManagedObject *ptr);
      //! set parameter to a 'c-string' type 
      /* \internal this function creates and keeps a *copy* of the passed string! */
      void set(const char *s);
      //! set parameter to a 'c-string' type 
      /* \internal this function does *not* copy whatever data this
         pointer points to (it doesn't have the info to do so), so
         this pointer belongs to the application, and it can not be
         used remotely */
      void set(void *v);
      /*! set parameter to given float value and type */
      void set(const float v) { clear(); type = OSP_FLOAT; (float&)f = v; }
      /*! set parameter to given int value and type */
      void set(const int v) { clear(); type = OSP_INT; (int&)f = v; }
      /*! set parameter to vec3f value and type */
      void set(const vec3f &v) { clear(); type = OSP_vec3f; (vec3f&)f = v; }
      /*! set parameter to vec3i value and type */
      void set(const vec3i &v) { clear(); type = OSP_vec3i; (vec3i&)i = v; }
      /*! storage for the various types this parameter could be */
      union {
        float f[4];
        int32 i[4];
        uint32 ui[4]; 
        int64  l;
        ManagedObject *ptr;
        const char    *s;
      };
      /*! actual type of this parameter */
      OSPDataType type;
      /*! name under which this parameter is registered */
      const char *name;
    };

    /*! constructor */
    ManagedObject() : ID(-1), ispcEquivalent(NULL) {};
    /*! destructor */
    virtual ~ManagedObject() {};

    //! commit the model's (or more exactly, the model's parameters') outstanding changes
    virtual void commit() {}
    
    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::ManagedObject"; }
    /*! return the ISPC equivalent of this class*/
    void *getIE() const { return ispcEquivalent; }

    /*! find a given parameter, or add it if not exists (and so specified) */
    Param *findParam(const char *name, bool addIfNotExist = false);
    /*! set given parameter to given data array */
    void   setParam(const char *name, ManagedObject *data);

    /*! @{
    /*! \brief find the named parameter, and return its object value if
        available; else return 'default' value 
        
        \detailed The returned managed object will *not* automatically
        have its refcount increased; it is up to the callee to
        properly do that (typically by assigning to a proper 'ref'
        instance */
    ManagedObject *getParam(const char *name, ManagedObject *valIfNotFound);
    vec3fa getParam3f(const char *name, const vec3fa valIfNotFound);
    vec3f  getParam3f(const char *name, const vec3f  valIfNotFound);
    vec3i  getParam3i(const char *name, const vec3i  valIfNotFound);
    int32  getParam1i(const char *name, const int32  valIfNotFound);
    float  getParamf (const char *name, const float  valIfNotFound);
    void  *getVoidPtr(const char *name, void *valIfNotFound);
    const char  *getParamString(const char *name, const char *valIfNotFound);
    /*! @} */

    std::vector<Param *> paramList; /*!< list of parameters attached
                                       to this object */
    id_t ID; /*!< a global ID that can be used for referencing an
               object remotely */
    void *ispcEquivalent; /*!< ispc-side eqivalent of this C++-side
                            class, if available (NULL if not) */
  };

}
