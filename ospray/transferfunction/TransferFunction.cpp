//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "TransferFunction.h"
#include "ospray/common/library.h"
#include <map>

namespace ospray
{
    typedef TransferFunction *(*creatorFct)();
    std::map<std::string, creatorFct> transferFunctionRegistry;

    TransferFunction * TransferFunction::createTransferFunction(const char * identifier)
    {
        char type[strlen(identifier)+2];
        strcpy(type,identifier);

        for(char *s = type; *s; ++s)
            if(*s == '-')
                *s = '_';

        std::map<std::string, TransferFunction *(*)()>::iterator it = transferFunctionRegistry.find(type);

        if(it != transferFunctionRegistry.end())
            return it->second ? (it->second)() : NULL;

        if(ospray::logLevel >= 2) 
            std::cout << "#ospray: trying to look up transfer function type '" << type << "' for the first time" << std::endl;

        std::string creatorName = "ospray_create_transfer_function__"+std::string(type);
        creatorFct creator = (creatorFct)getSymbol(creatorName);
        transferFunctionRegistry[type] = creator;

        if(creator == NULL)
        {
            if(ospray::logLevel >= 1)
                std::cout << "#ospray: could not find transfer function type '" << type << "'" << std::endl;

            return NULL;
        }

        return (*creator)();
  }

} // namespace ospray
