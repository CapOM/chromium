// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/bluetooth_test_android.h"

#include "base/logging.h"
#include "device/bluetooth/android/wrappers.h"
#include "device/bluetooth/bluetooth_adapter_android.h"
#include "jni/Fakes_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

namespace device {

BluetoothTestAndroid::BluetoothTestAndroid() {
}

BluetoothTestAndroid::~BluetoothTestAndroid() {
}

void BluetoothTestAndroid::SetUp() {
  // Register in SetUp so that ASSERT can be used.
  ASSERT_TRUE(RegisterNativesImpl(AttachCurrentThread()));
}

void BluetoothTestAndroid::InitWithDefaultAdapter() {
  adapter_ = adapter_android_ =
      BluetoothAdapterAndroid::Create(
          BluetoothAdapterWrapper_CreateWithDefaultAdapter().obj()).get();
}

void BluetoothTestAndroid::InitWithoutDefaultAdapter() {
  adapter_ = adapter_android_ = BluetoothAdapterAndroid::Create(NULL).get();
}

void BluetoothTestAndroid::InitWithFakeAdapter() {
  adapter_ = adapter_android_ =
      BluetoothAdapterAndroid::Create(
          Java_FakeBluetoothAdapter_create(AttachCurrentThread()).obj()).get();
}

void BluetoothTestAndroid::DiscoverANewDevice() {
  Java_FakeBluetoothAdapter_discoverANewDevice(AttachCurrentThread(),
                                               j_fake_bluetooth_adapter_.obj());
}

}  // namespace device
