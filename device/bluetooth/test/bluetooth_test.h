// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_H_
#define DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_H_

#include "base/memory/ref_counted.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class BluetoothAdapter;

// A test fixture for Bluetooth abstracting platform specifics for creating
// and controlling fake low level objects.
class BluetoothTestBase: public testing::Test {
 public:
  BluetoothTestBase();
  ~BluetoothTestBase() override;

  virtual void InitWithFakeAdapter() = 0;

  scoped_refptr<BluetoothAdapter> adapter_;
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_H_
