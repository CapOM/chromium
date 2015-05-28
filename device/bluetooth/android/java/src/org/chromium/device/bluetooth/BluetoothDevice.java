// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.annotation.TargetApi;
import android.os.Build;


import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.Log;
import org.chromium.device.bluetooth.BluetoothAdapter;

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

    private long mNativeBluetoothDeviceAndroid;
    private final android.bluetooth.BluetoothDevice mDevice;

    /**
     * Constructs a BluetoothDevice wrapping device, and associated
     * device::BluetoothDeviceAndroid. 
     *  
     * Calls adapter.onDeviceAdded to ensure objects are owned.
     */
    public BluetoothDevice(android.bluetooth.BluetoothDevice device, BluetoothAdapter adapter) {
        mDevice = device;
        mNativeBluetoothDeviceAndroid = nativeInit();
        adapter.onDeviceAdded(this);
    }

    @CalledByNative
    private long getNativePointer() {
        return mNativeBluetoothDeviceAndroid;
    }

    // ---------------------------------------------------------------------------------------------
    // bluetooth_device_android.cc C++ methods declared for access from java:

    // Binds to Init().
    private native long nativeInit();
}
