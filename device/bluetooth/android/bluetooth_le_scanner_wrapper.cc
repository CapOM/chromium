// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/android/bluetooth_le_scanner_wrapper.h"

#include "base/android/jni_android.h"
#include "jni/BluetoothLeScannerWrapper_jni.h"

using base::android::AttachCurrentThread;

namespace device {

// static
ScopedJavaLocalRef<jobject>
BluetoothLeScannerWrapper::CreateWithDefaultLeScanner() {
  return Java_BluetoothLeScannerWrapper_createWithDefaultLeScanner(
      AttachCurrentThread());
}

// static
bool BluetoothLeScannerWrapper::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);  // From BluetoothLeScannerWrapper_jni.h
}

}  // namespace device
