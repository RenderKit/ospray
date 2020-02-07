// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class TransferFunction
    : public ManagedObject<OSPTransferFunction, OSP_TRANSFER_FUNCTION>
{
 public:
  TransferFunction(const std::string &type);
  TransferFunction(const TransferFunction &copy);
  TransferFunction(OSPTransferFunction existing = nullptr);
};

static_assert(sizeof(TransferFunction) == sizeof(OSPTransferFunction),
    "cpp::TransferFunction can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline TransferFunction::TransferFunction(const std::string &type)
{
  ospObject = ospNewTransferFunction(type.c_str());
}

inline TransferFunction::TransferFunction(const TransferFunction &copy)
    : ManagedObject<OSPTransferFunction, OSP_TRANSFER_FUNCTION>(copy.handle())
{
  ospRetain(copy.handle());
}

inline TransferFunction::TransferFunction(OSPTransferFunction existing)
    : ManagedObject<OSPTransferFunction, OSP_TRANSFER_FUNCTION>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::TransferFunction, OSP_TRANSFER_FUNCTION);

} // namespace ospray
