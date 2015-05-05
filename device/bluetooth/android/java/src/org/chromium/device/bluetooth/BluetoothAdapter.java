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
    private boolean hasBluetoothPermission() {
        return mHasBluetoothPermission;
    }

    @CalledByNative
    private String getName() {
        if (mAdapter != null) {
            return mAdapter.getName();
        } else {
            return "";
        }
    }

    private BluetoothAdapter(Context context) {
        mHasBluetoothPermission =
                context.checkCallingOrSelfPermission(android.Manifest.permission.BLUETOOTH)
                == PackageManager.PERMISSION_GRANTED;
        if (!mHasBluetoothPermission) {
            Log.w(TAG, "Can not use Bluetooth API, requires BLUETOOTH permission.");
            return;
        }

        mAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
        if (mAdapter == null) {
            Log.w(TAG, "No adapter found.");
        }
    }
}
