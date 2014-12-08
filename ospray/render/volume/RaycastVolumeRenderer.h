//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#pragma once

#include "ospray/camera/camera.h"
#include "ospray/common/Model.h"
#include "ospray/lights/Light.h"
#include "ospray/render/renderer.h"

namespace ospray {

    //! \brief A concrete implemetation of the Renderer class for rendering
    //!  volumes optionally containing embedded surfaces.
    //!
    class RaycastVolumeRenderer : public Renderer {
    public:

        //! Constructor.
        RaycastVolumeRenderer() {};

        //! Destructor.
       ~RaycastVolumeRenderer() {};

        //! Initialize the renderer state, and create the equivalent ISPC volume renderer object.
        virtual void commit();

        //! Get the equivalent ISPC volume renderer object.
        void *getEquivalentISPC() const { return(getIE()); }

        //! A string description of this class.
        virtual std::string toString() const { return("ospray::RaycastVolumeRenderer"); }

    protected:

        //! Required renderer state.
        Camera *camera;  Model *model;  Model *dynamicModel;

        //! Print an error message.
        void emitMessage(const std::string &kind, const std::string &message) const
            { std::cerr << "  " + toString() + "  " + kind + ": " + message + "." << std::endl; }

        //! Error checking.
        void exitOnCondition(bool condition, const std::string &message) const
            { if (!condition) return;  emitMessage("ERROR", message);  exit(1); }

        //! Gather pointers to the ISPC equivalents from an array of Light objects.
        void **getLightsFromData(const Data *buffer);

    };

} // namespace ospray

