// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/bluetooth_test_android.h"

#include "base/logging.h"
#include "device/bluetooth/android/bluetooth_adapter_wrapper.h"
#include "device/bluetooth/bluetooth_adapter_android.h"
#include "jni/FakeBluetoothAdapter_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

namespace device {

BluetoothTestAndroid::BluetoothTestAndroid() {
}

BluetoothTestAndroid::~BluetoothTestAndroid() {
}

void BluetoothTestAndroid::SetUp() {
  ASSERT_TRUE(RegisterNativesImpl(AttachCurrentThread()));
}

void BluetoothTestAndroid::InitWithDefaultAdapter() {
  adapter_ = adapter_android_ =
      BluetoothAdapterAndroid::Create(
          BluetoothAdapterWrapper::CreateWithDefaultAdapter().obj()).get();
}

void BluetoothTestAndroid::InitWithoutDefaultAdapter() {
  adapter_ = adapter_android_ = BluetoothAdapterAndroid::Create(NULL).get();
}

void BluetoothTestAndroid::InitWithFakeAdapter() {
  j_fake_bluetooth_adapter_.Reset(
      Java_FakeBluetoothAdapter_create(AttachCurrentThread()));

  adapter_ = adapter_android_ =
      BluetoothAdapterAndroid::Create(j_fake_bluetooth_adapter_.obj()).get();
}

}  // namespace device
