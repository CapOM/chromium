// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.content.Context;
import android.content.pm.PackageManager;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

/**
 * Exposes android.bluetooth.BluetoothAdapter as necessary for C++
 * device::BluetoothAdapterAndroid.
 */
@JNINamespace("device")
class BluetoothAdapter /*final?*/ {
    private static final String TAG = "BluetoothAdapter";

    private final boolean mHasBluetoothPermission;

    @CalledByNative
    private static BluetoothAdapter create(Context context) {
        return new BluetoothAdapter(context);
    }

    @CalledByNative
    private boolean hasBluetoothPermission() {
        return mHasBluetoothPermission;
    }

    private BluetoothAdapter(Context context) {
        mHasBluetoothPermission =
                context.checkCallingOrSelfPermission(android.Manifest.permission.BLUETOOTH)
                == PackageManager.PERMISSION_GRANTED;
        if (!mHasBluetoothPermission) {
            Log.w(TAG, "Failed to use bluetooth API, requires BLUETOOTH permission.");
        }
    }
}
