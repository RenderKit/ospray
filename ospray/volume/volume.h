#pragma once

#include "../common/managed.h"

namespace ispc {
  struct _Volume;
};

namespace ospray {
  //! possible scalar types to be used in volumes

  /* \internal this should probably get merged with what's in the
     public ospray.h */
  typedef enum { UNORM8, FLOAT, DOUBLE, UNSPECIFIED } ScalarType;

  /*! literal names of the scalar types */
  extern const char *scalarTypeName[];

  //! base abstraction class for anything 'volume'
  /*! the C++ interface to the volume class is purely generic. All
    specialization (bricked vs naive, largemem vs smallmem, etc) are
    passed to the single volume class, which internally switches to
    the different ISPC variants that then internally use function
    pointers) 

    \internal eventually this class shoul dbe merged with
    WrapperVolume; see notes there
  */
  struct Volume {
    typedef enum { 
      BRICKED  = 0x1,
      NAIVE    = 0x2,
      LAYOUT   = (BRICKED|NAIVE),
    } FlagValues;
    
    const vec3i      size;
    const ScalarType scalarType;
    
    /*! the size, scalar type and flags specify the *internal*
      layout. */
    Volume(const vec3i &size, ScalarType scalarType) 
      : size(size), scalarType(scalarType)
    {};

    virtual std::string toString() const { return "ospray::Volume"; }
    virtual void setRegion(const vec3i &where,const vec3i &size,const uint8 *data)=0;
    virtual void setRegion(const vec3i &where,const vec3i &size,const float *data)=0;
    virtual int layout() const = 0;
    //! return 'properly' typed ispc equivalent 
    /*! \internal might want to remove and directly use voidptr'ed getIE() */
    ispc::_Volume *inISPC() const { return ispcPtr; }

    //! tri-linear interpolation at given sample location
    virtual float lerpf(const vec3fa &samplePos) = 0;
  protected:
    // void *ispcPtr;
    ispc::_Volume *ispcPtr;
    /*! pointer to the ISPC-side "class" written with
      function pointers etc */
  };

  template<typename T>
  struct NaiveVolume : public Volume {
    NaiveVolume(const vec3i &size, const T *internalData=NULL);

    virtual void setRegion(const vec3i &where,const vec3i &size,const uint8 *data);
    virtual void setRegion(const vec3i &where,const vec3i &size,const float *data);
    virtual std::string toString() const;
    virtual int layout() const { return NAIVE; };

    //! tri-linear interpolation at given sample location
    virtual float lerpf(const vec3fa &samplePos);

    const T *data;
  };

  template<typename T>
  struct BrickedVolume : public Volume {
    BrickedVolume(const vec3i &size, const T *internalData=NULL);

    virtual void setRegion(const vec3i &where,const vec3i &size,const uint8 *data);
    virtual void setRegion(const vec3i &where,const vec3i &size,const float *data);
    virtual std::string toString() const;
    virtual int layout() const { return BRICKED; };

    //! tri-linear interpolation at given sample location
    virtual float lerpf(const vec3fa &samplePos);
  };


  /*  we don't have the volume interface fleshed out, yet, and
      currently create the proper volume type on-the-fly during
      commit, so use this wrpper thingy... ( \see
      volview_notes_on_volume_interface ). eventually this class
      should be the base class for whatever actual volume clssses we
      have above */
  struct WrapperVolume : public ospray::ManagedObject 
  {
    WrapperVolume() : internalRep(NULL) {}
    virtual void commit();
    Volume *internalRep;

    //    static Volume *createVolume(const char *identifier);
  };


  /*! load volume in raw format. (eventually to be moved to special
    importers subdir */
  Volume *loadRaw(const char *fileName, 
                  ScalarType inputType, long sx, long sy, long sz, long flags);

  /*! compute new (float-)volume resampled from given 'src' volume */
  Volume *resampleVolume(Volume *src, const vec3i &newSize, const long flags);

  //! helper - to be (re)moved once no longer needed
  void saveVolume(Volume *volume, const std::string &fileName);
  //! helper - to be (re)moved once no longer needed
  Volume *loadVolume(const std::string &fileName);



  //   /*! \brief registers a internal ospray::<ClassName> volume under
  //       the externally accessible name "external_name" 
      
  //       \internal This currently works by defining a extern "C" function
  //       with a given predefined name that creates a new instance of this
  //       volume. By having this symbol in the shared lib ospray can
  //       lateron always get a handle to this fct and create an instance
  //       of this volume.
  //   */
  // #define OSP_REGISTER_VOLUME(InternalClassName,external_name)        \
  //   extern "C" Volume *ospray_create_volume__##external_name()        \
  //   {                                                                 \
  //     return new InternalClassName;                                   \
  //   }                                                               \


}

