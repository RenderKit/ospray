#pragma once

// ospray
#include "ospray/common/Data.h"
#include "ospray/transferfunction/TransferFunction.h"
#include "LinearTransferFunction_ispc.h"
// std
#include <vector>

namespace ospray {

  //! \brief A concrete implementation of the TransferFunction class for
  //!  piecewise linear transfer functions.
  //!
  class LinearTransferFunction : public TransferFunction {
  public:

    //! Constructor.
    LinearTransferFunction() {}

    //! Destructor.
    virtual ~LinearTransferFunction() { if (ispcEquivalent != NULL) ispc::LinearTransferFunction_destroy(ispcEquivalent); }

    //! Allocate storage and populate the transfer function.
    virtual void commit();

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::LinearTransferFunction"); }

  protected:

    //! Data array that stores the color map.
    Ref<Data> colorValues;

    //! Data array that stores the opacity map.
    Ref<Data> opacityValues;

    //! Create the equivalent ISPC transfer function.
    virtual void createEquivalentISPC();

  };

} // ::ospray

