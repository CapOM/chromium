// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_ANDROID_H_
#define DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_ANDROID_H_

#include "device/bluetooth/test/bluetooth_test.h"

namespace device {

typedef BluetoothTestAndroid BluetoothTest;

// Android implementation of BluetoothTestBase.
class BluetoothTestAndroid: public BluetoothTestBase {
 public:
  bool TestFixtureIsEnabledOnThisPlatform() override;
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_ANDROID_H_
