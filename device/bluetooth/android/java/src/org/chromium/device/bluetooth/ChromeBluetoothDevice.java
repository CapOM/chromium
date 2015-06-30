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
    private final List<ParcelUuid> mUuidsFromScan;

    /**
     * Constructs a ChromeBluetoothDevice and an associated C++
     * BluetoothDeviceAndroid. Adds new device to adapter.
     *
     * The C++ object lifetime is owned by the C++ BluetoothAdapterAndroid, and the
     * Java objects references are held by C++ objects.
     *
     * @param  device         Wrapper of android.bluetooth.BluetoothDevice.
     * @param  uuidsFromScan  Are cached to report Device's advertised UUIDs.
     * @param  adapter        Informed via onDeviceAdded of new device to maintain
     *                        object ownership.
     */
    public static void CreateAndAddToAdapter(Wrappers.BluetoothDeviceWrapper device,
            List<ParcelUuid> uuidsFromScan, ChromeBluetoothAdapter adapter) {
        ChromeBluetoothDevice chromeBluetoothDevice =
                new ChromeBluetoothDevice(device, uuidsFromScan);
        adapter.onDeviceAdded(chromeBluetoothDevice);
    }

    private ChromeBluetoothDevice(Wrappers.BluetoothDeviceWrapper device, List<ParcelUuid> uuidsFromScan) {
        mNativeBluetoothDeviceAndroid = nativeInit();
        mDevice = device;
        mUuidsFromScan = uuidsFromScan;
        Log.v(TAG, "ChromeBluetoothDevice created.");
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
