// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.annotation.TargetApi;
import android.os.Build;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.Log;

/**
 * Exposes android.bluetooth.BluetoothDevice as necessary for C++
 * device::BluetoothDeviceAndroid. 
 *  
 * Lifetime is controlled by device::BluetoothDeviceAndroid.
 */
@JNINamespace("device")
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
final class BluetoothDevice {
    private static final String TAG = Log.makeTag("Bluetooth");
}
