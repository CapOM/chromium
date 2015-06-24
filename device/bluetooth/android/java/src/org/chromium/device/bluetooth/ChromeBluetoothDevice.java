// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.annotation.TargetApi;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.os.Build;
import android.os.ParcelUuid;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.Log;

import java.util.List;

/**
 * Exposes android.bluetooth.BluetoothDevice as necessary for C++
 * device::BluetoothDeviceAndroid.
 *
 * Lifetime is controlled by device::BluetoothDeviceAndroid.
 */
@JNINamespace("device")
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
final class ChromeBluetoothDevice {
    private static final String TAG = "cr.Bluetooth";

    private long mNativeBluetoothDeviceAndroid;
    private final Wrappers.BluetoothDeviceWrapper mDevice;
    private List<ParcelUuid> mUuidsFromScan;

    /**
     * Constructs a ChromeBluetoothDevice wrapping device, and associate
     * device::BluetoothDeviceAndroid.
     *
     * Calls adapter.onDeviceAdded to ensure objects are owned.
     */
    public ChromeBluetoothDevice(
            Wrappers.BluetoothDeviceWrapper device, ChromeBluetoothAdapter adapter, List<ParcelUuid> uuidsFromScan) {
        mNativeBluetoothDeviceAndroid = nativeInit();
        mDevice = device;
        mUuidsFromScan = uuidsFromScan;
        Log.v(TAG, "ChromeBluetoothDevice created.");
        adapter.onDeviceAdded(this);
    }

    @CalledByNative
    private long getNativePointer() {
        return mNativeBluetoothDeviceAndroid;
    }

    // ---------------------------------------------------------------------------------------------
    // BluetoothDeviceAndroid methods implemented in java:

    // Implements BluetoothAdapterAndroid::GetBluetoothClass.
    @CalledByNative
    private int getBluetoothClass() {
        return mDevice.getBluetoothClass_getDeviceClass();
    }

    // Implements BluetoothAdapterAndroid::GetAddress.
    @CalledByNative
    private String getAddress() {
        return mDevice.getAddress();
    }

    // Implements BluetoothAdapterAndroid::IsPaired.
    @CalledByNative
    private boolean isPaired() {
        return mDevice.getBondState() == BluetoothDevice.BOND_BONDED;
    }

    // Implements BluetoothAdapterAndroid::GetUUIDs.
    // Returns number of UUIDs found when scanning.
    @CalledByNative
    private int getUuidsCount() {
        if (mUuidsFromScan == null) {
            return 0;
        }
        return mUuidsFromScan.size();
    }

    // Implements BluetoothAdapterAndroid::GetUUIDs.
    // Returns one UUID String from the array of UUIDs.
    @CalledByNative
    private String getUuid(int i) {
        return mUuidsFromScan.get(i).toString();
    }

    // Implements BluetoothAdapterAndroid::GetDeviceName.
    @CalledByNative
    private String getDeviceName() {
        return mDevice.getName();
    }

    // ---------------------------------------------------------------------------------------------
    // bluetooth_device_android.cc C++ methods declared for access from java:

    // Binds to Init().
    private native long nativeInit();
}
