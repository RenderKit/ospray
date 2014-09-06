//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include <map>
#include "ospray/common/library.h"
#include "ospray/volume/StructuredVolumeFile.h"

namespace ospray {

    StructuredVolumeFile *StructuredVolumeFile::open(const std::string &filename) {

        //! Function pointer type for creating a concrete instance of a subtype of this class.
        typedef StructuredVolumeFile *(*creationFunctionPointer)(const std::string &filename);

        //! Function pointers corresponding to each subtype.
        static std::map<std::string, creationFunctionPointer> symbolRegistry;

        //! The subtype string is the file extension.
        std::string type = filename.substr(filename.find_last_of(".") + 1);

        //! Return a concrete instance of the requested subtype if the creation function is already known.
        if (symbolRegistry.count(type) > 0 && symbolRegistry[type] != NULL) return((*symbolRegistry[type])(filename));

        //! Otherwise construct the name of the creation function to look for.
        std::string creationFunctionName = "ospray_create_structured_volume_file_" + std::string(type);

        //! Look for the named function.
        symbolRegistry[type] = (creationFunctionPointer) getSymbol(creationFunctionName);

        //! The named function may not be found if the requested subtype is not known.
        if (symbolRegistry[type] == NULL && ospray::logLevel >= 1) std::cout << "OSPRay::StructuredVolumeFile error: unrecognized subtype '" << type << "'" << std::endl;

        //! Return a concrete instance of the requested subtype.
        return(symbolRegistry[type] ? (*symbolRegistry[type])(filename) : NULL);

    }

} // namespace ospray

