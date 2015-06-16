// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_LE_SCANNER_WRAPPER_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_LE_SCANNER_WRAPPER_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "device/bluetooth/bluetooth_export.h"

using base::android::ScopedJavaLocalRef;

namespace device {

// Wraps Java class BluetoothLeScannerWrapper.
class DEVICE_BLUETOOTH_EXPORT BluetoothLeScannerWrapper {
 public:
  // Calls Java: BluetoothLeScannerWrapper.createWithDefaultLeScanner().
  static ScopedJavaLocalRef<jobject> CreateWithDefaultLeScanner();

  // Register C++ methods exposed to Java using JNI.
  static bool RegisterJNI(JNIEnv* env);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_LE_SCANNER_WRAPPER_H_
