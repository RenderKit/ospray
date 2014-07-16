/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#pragma once

#include "ospray/common/managed.h"

namespace ispc
{
    struct _TransferFunction;
};

namespace ospray
{
    //! \brief A transfer function is an abstraction that maps a value to
    //!  a color and opacity for rendering.
    //!
    //!  The actual mapping is unknown to this class, and is implemented
    //!  in subclasses. A type string specifies a particular concrete
    //!  implementation to createTransferFunction(). The type string must
    //!  be registered with in OSPRay proper, or in a loaded module using
    //!  OSP_REGISTER_TRANSFER_FUNCTION.
    //!
    class TransferFunction : public ManagedObject {
    public:

        //! Constructor.
        TransferFunction() { };

        //! Destructor.
        virtual ~TransferFunction() { };

        //! A string description of this class.
        virtual std::string toString() const { return "ospray::TransferFunction"; }

        //! Allocate storage and populate the transfer function.
        virtual void commit() = 0;

        //! Create the equivalent ISPC transfer function.
        virtual ispc::_TransferFunction * createIE() = 0;

        //! Create a transfer function of the given type.
        static TransferFunction * createTransferFunction(const char * identifier);

        //! Look up the color corresponding to the provided value.
        virtual vec3f lookupColor(float value) = 0;

        //! Look up the opacity corresponding to the provided value.
        virtual float lookupAlpha(float value) = 0;
    };

    //! \brief Register an internal transfer function class name
    //!  (ospray::<ClassName>) under the externally accessible name
    //!  "ExternalName".
    //! 
    //! \internal This mapping works by defining an extern "C" function with
    //!  a given predefined name that creates a new instance of the transfer
    //!  function. By having this symbol in the shared library, OSPRay can always
    //!  get a handle in the createTransferFunction function to create an
    //!  instance of a transfer function.
    //!
    #define OSP_REGISTER_TRANSFER_FUNCTION(InternalClassName, ExternalName) \
        extern "C" TransferFunction * ospray_create_transfer_function__##ExternalName() { return(new InternalClassName); }

} // namespace ospray
