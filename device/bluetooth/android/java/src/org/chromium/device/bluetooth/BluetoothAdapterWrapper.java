// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.Manifest;
import android.annotation.TargetApi;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.le.BluetoothLeScanner;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.Log;

/**
 * Wraps android.bluetooth.BluetoothAdapter, pasing through to a provided
 * object. This indirection enables fake implementations when running tests.
 */
@JNINamespace("device")
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class BluetoothAdapterWrapper {
    private static final String TAG = Log.makeTag("Bluetooth");
    private final BluetoothAdapter mAdapter;

    /***
     * Creates a BluetoothAdapterWrapper using the default android.bluetooth.BluetoothAdapter. May
     * fail if the default adapter is not available or if the application does not have sufficient
     * permissions.
     */
    @CalledByNative
    public static BluetoothAdapterWrapper createWithDefaultAdapter(Context context) {
        final boolean hasMinAPI = Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
        if (!hasMinAPI) {
            Log.i(TAG, "BluetoothAdapterWrapper.create failed: SDK version (%d) too low.",
                    Build.VERSION.SDK_INT);
            return null;
        }

        final boolean hasPermissions =
                context.checkCallingOrSelfPermission(Manifest.permission.BLUETOOTH)
                        == PackageManager.PERMISSION_GRANTED
                && context.checkCallingOrSelfPermission(Manifest.permission.BLUETOOTH_ADMIN)
                        == PackageManager.PERMISSION_GRANTED;
        if (!hasPermissions) {
            Log.w(TAG, "BluetoothAdapterWrapper.create failed: Lacking Bluetooth permissions.");
            return null;
        }

        // Only Low Energy currently supported, see BluetoothAdapterAndroid class note.
        final boolean hasLowEnergyFeature =
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2
                && context.getPackageManager().hasSystemFeature(
                           PackageManager.FEATURE_BLUETOOTH_LE);
        if (!hasLowEnergyFeature) {
            Log.i(TAG, "BluetoothAdapterWrapper.create failed: Low Energy not supported on system.");
            return null;
        }

        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter == null) {
            Log.i(TAG, "BluetoothAdapterWrapper.create failed: Default adapter not found.");
            return null;
        } else {
            Log.i(TAG, "BluetoothAdapterWrapper created with default adapter.");
            return new BluetoothAdapterWrapper(adapter);
        }
    }

    public BluetoothAdapterWrapper(BluetoothAdapter adapter) {
        assert adapter != null;
        mAdapter = adapter;
    }

    public BluetoothLeScanner getBluetoothLeScanner() {
        return mAdapter.getBluetoothLeScanner();
    }

    public boolean isEnabled() {
        return mAdapter.isEnabled();
    }

    public String getAddress() {
        return mAdapter.getAddress();
    }

    public String getName() {
        return mAdapter.getName();
    }

    public int getScanMode() {
        return mAdapter.getScanMode();
    }

    public boolean isDiscovering() {
        return mAdapter.isDiscovering();
    }
}
