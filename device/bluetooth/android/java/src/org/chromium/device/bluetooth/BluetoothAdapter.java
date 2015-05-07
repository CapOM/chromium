// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.content.Context;
import android.content.pm.PackageManager;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.Log;

/**
 * Exposes android.bluetooth.BluetoothAdapter as necessary for C++
 * device::BluetoothAdapterAndroid.
 */
@JNINamespace("device")
final class BluetoothAdapter {
    private static final String TAG = Log.makeTag("Bluetooth");

    private final boolean mHasBluetoothPermission;
    private android.bluetooth.BluetoothAdapter mAdapter;

    @CalledByNative
    private static BluetoothAdapter create(Context context) {
        return new BluetoothAdapter(context);
    }

    @CalledByNative
    private static BluetoothAdapter createWithoutPermissionForTesting(Context context) {
        return new BluetoothAdapter();
    }

    // Constructs a BluetoothAdapter.
    private BluetoothAdapter(Context context) {
        mHasBluetoothPermission =
                context.checkCallingOrSelfPermission(android.Manifest.permission.BLUETOOTH)
                        == PackageManager.PERMISSION_GRANTED
                && context.checkCallingOrSelfPermission(android.Manifest.permission.BLUETOOTH_ADMIN)
                        == PackageManager.PERMISSION_GRANTED;
        if (!mHasBluetoothPermission) {
            Log.w(TAG,
                    "Can not use Bluetooth API, requires BLUETOOTH and BLUETOOTH_ADMIN permission.");
            return;
        }

        mAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
        if (mAdapter == null) {
            Log.w(TAG, "No adapter found.");
        }
    }

    // Constructs a BluetoothAdapter for testing, with no permission.
    private BluetoothAdapter() {
        Log.i(TAG, "Testing BluetoothAdapter created.");
        mHasBluetoothPermission = false;
    }

    @CalledByNative
    private boolean hasBluetoothPermission() {
        return mHasBluetoothPermission;
    }

    // ---------------------------------------------------------------------------------------------
    // BluetoothAdapterAndroid.h interface:

    @CalledByNative
    private String getAddress() {
        if (isPresent()) {
            return mAdapter.getAddress();
        } else {
            return "";
        }
    }

    @CalledByNative
    private String getName() {
        if (isPresent()) {
            return mAdapter.getName();
        } else {
            return "";
        }
    }

    @CalledByNative
    private boolean isPresent() {
        return mAdapter != null;
    }

    @CalledByNative
    private boolean isPowered() {
        return isPresent() && mAdapter.isEnabled();
    }

    @CalledByNative
    private boolean isDiscoverable() {
        return isPresent()
                && mAdapter.getScanMode()
                == android.bluetooth.BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE;
    }

    @CalledByNative
    private boolean isDiscovering() {
        return isPresent() && mAdapter.isDiscovering();
    }
}
