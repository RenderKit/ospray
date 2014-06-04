#pragma once

#include "ospray/common/managed.h"

namespace ispc {
  struct _Volume;
};

// #define LOW_LEVEL_KERNELS 1

namespace ospray {
  /*! \brief a volume _sampler_ is the abstraction for the actul thing
      that returns actual samples

      This is an *abstraction* - the actual data layout or source of
      those samples isn't specified: it can be a strucured volume, and
      unstructured one, RBFs, whatever. Begin this general, we do not
      even specify the 'dimensions' of the volume, as this may simply
      not make sense for something like an RBF.
  */
  struct Volume : public ManagedObject
  {
    Volume() {};

    virtual std::string toString() const
    { return "ospray::Volume(abstract base class)"; }

    /*! create an ISPC-equivalent for this type of class */
    virtual ispc::_Volume *createIE() { return NULL; };
    // ispc::_Volume *getIE() { return ispcEquivalent; };

    //! tri-linear interpolation at given sample location
    virtual float lerpf(const vec3fa &samplePos) = 0;
    //! gradient at given sample location
    virtual vec3f gradf(const vec3fa &samplePos) = 0;

    /*! \brief creates an abstract volume class of given type 

      The respective volume type must be a registered volume type
      in either ospray proper or any already loaded module. For
      volume types specified in special modules, make sure to call
      ospLoadModule first. */
    static Volume *createVolume(const char *identifier);

    // ispc::_Volume *ispcEquivalent;
  };

  template<typename T>
  struct StructuredVolume : public Volume
  {
    typedef T ScalarType;
    StructuredVolume(const vec3i &size=vec3i(-1), const T *data=NULL) 
      : size(size), data(data) 
    {}
    virtual std::string toString() const
    { return "ospray::StructuredVolumeSampler<internalformat>"; }
    virtual float lerpf(const vec3fa &samplePos)=0;
    virtual vec3f gradf(const vec3fa &samplePos)=0;
    virtual void commit(); 
    
    virtual void allocate() = 0;
    virtual void setRegion(const vec3i &where,const vec3i &size,const T *data)=0;
    //! helper function load files from RAW format (assuming same scalar type
    void loadRAW(const vec3i &size, const char *fileName);

    /*! resample form another volume - mostly for testing */
    virtual void resampleFrom(Volume *source);

    vec3i       size;
    vec3f       f_size; /*! translation from [(0,0,0)-(1,1,1)] to cell
                          coordinate space */
    vec3f       f_clampSize; /*! clamp to the largest valid float
                               coordinate still inside the voluem
                               (i.e., f_size-ulp */
    const T    *data;
  };



  // //! base abstraction class for anything 'volume'
  // /*! this is separate from the sampler because a complex volume class
  //     like an AMR volume may consist of multiple different samplers */
  // struct Volume : public ManagedObject {
  //   VolumeSampler *sampler;

  //   Volume(VolumeSampler *sampler = NULL) : sampler(sampler) {};

  //   virtual std::string toString() const { return "ospray::VolumeObject()"; }

  //   static Volume *createVolume(const char *identifier);
  // };

  /*! a special class of volume object that contains a structured
      volume sampler

      Structured volumes right now have special load/store functions
  */
  // template<typename StructuredSampler>
  // struct StructuredVolume : public Volume 
  // {
  //   StructuredSampler *structured;
  //   StructuredVolume() : structured(NULL) {};
  //   virtual ~StructuredVolume() {};
  //   virtual void commit(); 
  // };

  /*! \brief registers a internal ospray::<ClassName> volume under
    the externally accessible name "external_name" 
    
    \internal This currently works by defining a extern "C" function
    with a given predefined name that creates a new instance of this
    volume. By having this symbol in the shared lib ospray can
    lateron always get a handle to this fct and create an instance
    of this volume.
  */
#define OSP_REGISTER_VOLUME(InternalClassName,external_name)          \
  extern "C" Volume *ospray_create_volume__##external_name()          \
  {                                                                   \
    return new InternalClassName;                                     \
  }                                                                   \
  
}

