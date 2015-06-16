// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/bluetooth_test_android.h"

#include "base/logging.h"

namespace device {

bool BluetoothTestBase::TestFixtureIsEnabledOnThisPlatform() {
  LOG(INFO) << "Skipping test. "
               "BluetoothTest fixture not implemented on this platform."
  return false;
}

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_H_
