// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Managed.h"
#include "common/Util.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE TransferFunction : public ManagedObject
{
  TransferFunction();
  virtual ~TransferFunction() override = default;
  virtual void commit() override;
  virtual std::string toString() const override;

  static TransferFunction *createInstance(const std::string &type);
  template <typename T>
  static void registerType(const char *type);

  range1f valueRange;
  virtual std::vector<range1f> getPositiveOpacityValueRanges() const = 0;
  virtual std::vector<range1i> getPositiveOpacityIndexRanges() const = 0;

 private:
  template <typename BASE_CLASS, typename CHILD_CLASS>
  friend void registerTypeHelper(const char *type);
  static void registerType(const char *type, FactoryFcn<TransferFunction> f);
};

OSPTYPEFOR_SPECIALIZATION(TransferFunction *, OSP_TRANSFER_FUNCTION);

// Inlined defintions /////////////////////////////////////////////////////////

template <typename T>
inline void TransferFunction::registerType(const char *type)
{
  registerTypeHelper<TransferFunction, T>(type);
}

} // namespace ospray
