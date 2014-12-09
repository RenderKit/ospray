#pragma once

#include "ospray/volume/StructuredVolume.h"

namespace ospray {

  //! \brief A concrete implementation of the StructuredVolume class
  //!  with 62-bit addressing in which the voxel data is laid out in
  //!  memory in multiple pages each in brick order.
  //!
  class BlockBrickedVolume : public StructuredVolume {
  public:

    //! Constructor.
    BlockBrickedVolume() {};

    //! Destructor.
    virtual ~BlockBrickedVolume() {};

    //! Create the equivalent ISPC volume container.
    virtual void createEquivalentISPC();

    //! Copy voxels into the volume at the given index.
    virtual void setRegion(const void *source, const vec3i &index, const vec3i &count);

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::BlockBrickedVolume<" + voxelType + ">"); }

  protected:

    //! Complete volume initialization.
    virtual void finish();

    //! Update select parameters after the volume has been allocated and filled.
    virtual void updateEditableParameters();

    //! Range test a vector value against [b, c).
    inline bool inRange(const vec3i &a, const vec3i &b, const vec3i &c)
    { return(a.x >= b.x && a.y >= b.y && a.z >= b.z && a.x < c.x && a.y < c.y && a.z < c.z); }

  };

} // ::ospray

