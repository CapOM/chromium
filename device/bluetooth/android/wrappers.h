// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WRAPPER_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WRAPPER_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "device/bluetooth/bluetooth_export.h"

using base::android::ScopedJavaLocalRef;

namespace device {

// Bindings into Java methods in org.chromium.device.bluetooth.Wrappers classes:

// Register C++ methods exposed to Java using JNI.
bool WrappersRegisterJNI(JNIEnv* env);

// Calls Java: BluetoothAdapterWrapper.createWithDefaultAdapter().
DEVICE_BLUETOOTH_EXPORT ScopedJavaLocalRef<jobject>
BluetoothAdapterWrapper_CreateWithDefaultAdapter();

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_WRAPPER_H_
