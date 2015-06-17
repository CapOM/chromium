// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/bluetooth_test_android.h"

namespace device {

class BluetoothAdapterAndroidTest : public BluetoothTestAndroid {
 public:
  void InitWithDefaultAdapter() {
    adapter_ = adapter_android_ = 
        BluetoothAdapterAndroid::Create(
            BluetoothAdapterWrapper::CreateWithDefaultAdapter().obj()).get();
  }

  void InitWithoutDefaultAdapter() {
    adapter_ = adapter_android_ = BluetoothAdapterAndroid::Create(NULL).get();
  }
};

TEST_F(BluetoothAdapterAndroidTest, ConstructDefaultAdapter) {
  InitWithDefaultAdapter();
  ASSERT_TRUE(adapter_.get());
  if (!adapter_->IsPresent()) {
    LOG(WARNING) << "Bluetooth adapter not present; skipping unit test.";
    return;
  }
  EXPECT_GT(adapter_->GetAddress().length(), 0u);
  EXPECT_GT(adapter_->GetName().length(), 0u);
  EXPECT_TRUE(adapter_->IsPresent());
  // Don't know on test machines if adapter will be powered or not, but
  // the call should be safe to make and consistent.
  EXPECT_EQ(adapter_->IsPowered(), adapter_->IsPowered());
  EXPECT_FALSE(adapter_->IsDiscoverable());
  EXPECT_FALSE(adapter_->IsDiscovering());
}

}  // namespace device
