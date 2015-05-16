// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "device/bluetooth/bluetooth_adapter_android.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class BluetoothAdapterAndroidTest : public testing::Test {
 protected:
  void InitWithPermission() {
    adapter_ = BluetoothAdapterAndroid::CreateAdapter().get();
  }

  void InitWithoutPermission() {
    adapter_ =
        BluetoothAdapterAndroid::CreateAdapterWithoutPermissionForTesting()
            .get();
  }

  // Generic callbacks
  void Callback() { LOG(INFO) << "Callback"; }

  base::Closure GetCallback() {
    return base::Bind(&BluetoothAdapterAndroidTest::Callback,
                      base::Unretained(this));
  }

  void ErrorCallback() { LOG(INFO) << "ErrorCallback"; }

  base::Closure GetErrorCallback() {
    return base::Bind(&BluetoothAdapterAndroidTest::ErrorCallback,
                      base::Unretained(this));
  }

  scoped_refptr<BluetoothAdapterAndroid> adapter_;
};

TEST_F(BluetoothAdapterAndroidTest, Construct) {
  InitWithPermission();
  ASSERT_TRUE(adapter_.get());
  EXPECT_TRUE(adapter_->HasBluetoothCapability());
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

  // TODO JUST LOCAL TESTING
  // TODO JUST LOCAL TESTING
  // TODO JUST LOCAL TESTING
  // TODO JUST LOCAL TESTING
  // TODO JUST LOCAL TESTING
  // TODO JUST LOCAL TESTING
  adapter_->AddDiscoverySession(NULL, GetCallback(), GetErrorCallback());
  base::MessageLoop message_loop;
  message_loop.Run();
}

TEST_F(BluetoothAdapterAndroidTest, ConstructNoPermision) {
  InitWithoutPermission();
  ASSERT_TRUE(adapter_.get());
  EXPECT_FALSE(adapter_->HasBluetoothCapability());
  EXPECT_EQ(adapter_->GetAddress().length(), 0u);
  EXPECT_EQ(adapter_->GetName().length(), 0u);
  EXPECT_FALSE(adapter_->IsPresent());
  EXPECT_FALSE(adapter_->IsPowered());
  EXPECT_FALSE(adapter_->IsDiscoverable());
  EXPECT_FALSE(adapter_->IsDiscovering());
}

}  // namespace device
