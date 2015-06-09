// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
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
final class ChromeBluetoothAdapter {
    private static final String TAG = Log.makeTag("Bluetooth");

    private BluetoothAdapterWrapper mAdapter;

    /**
     * Constructs a ChromeBluetoothAdapter.
     * @param assumeNoBluetoothSupportForTesting Causes initialization presuming no
     *                                           Bluetooth support, for testing
     *                                           situations where permissions, SDK
     *                                           version, or feature isn't
     *                                           available.
     * @param adapterWrapperForTesting null to use the default system Bluetooth
     *                                 adapter. Enables tests to provide a fake
     *                                 BluetoothAdapterWrapper.
     */
    public ChromeBluetoothAdapter(Context context, boolean assumeNoBluetoothSupportForTesting,
            BluetoothAdapterWrapper adapterWrapperForTesting) {
        if (assumeNoBluetoothSupportForTesting) {
            Log.i(TAG, "ChromeBluetoothAdapter initialized for test with no Bluetooth support.");
            return;
        } else if (adapterWrapperForTesting != null) {
            mAdapter = adapterWrapperForTesting;
            Log.i(TAG, "ChromeBluetoothAdapter initialized for test with fake Android adapter.");
        } else {
            final boolean hasPermissions =
                    context.checkCallingOrSelfPermission(Manifest.permission.BLUETOOTH)
                            == PackageManager.PERMISSION_GRANTED
                    && context.checkCallingOrSelfPermission(Manifest.permission.BLUETOOTH_ADMIN)
                            == PackageManager.PERMISSION_GRANTED;
            if (!hasPermissions) {
                Log.w(TAG, "ChromeBluetoothAdapter disabled, lacking Bluetooth permissions.");
                return;
            }

            mAdapter = BluetoothAdapterWrapper.getDefaultAdapter();
            if (mAdapter == null) {
                Log.i(TAG, "ChromeBluetoothAdapter initialized, but default adapter not found.");
            } else {
                Log.i(TAG, "ChromeBluetoothAdapter initialized with default adapter.");
            }
        }
    }

    // ---------------------------------------------------------------------------------------------
    // BluetoothAdapterAndroid methods implemented in java:

    // Implements BluetoothAdapterAndroid::Create.
    @CalledByNative
    private static ChromeBluetoothAdapter create(Context context,
            boolean assumeNoBluetoothSupportForTesting,
            BluetoothAdapterWrapper adapterWrapperForTesting) {
        return new ChromeBluetoothAdapter(
                context, assumeNoBluetoothSupportForTesting, adapterWrapperForTesting);
    }

    // Implements BluetoothAdapterAndroid::GetAddress.
    @CalledByNative
    private String getAddress() {
        if (isPresent()) {
            return mAdapter.getAddress();
        } else {
            return "";
        }
    }

    // Implements BluetoothAdapterAndroid::GetName.
    @CalledByNative
    private String getName() {
        if (isPresent()) {
            return mAdapter.getName();
        } else {
            return "";
        }
    }

    // Implements BluetoothAdapterAndroid::IsPresent.
    @CalledByNative
    private boolean isPresent() {
        return mAdapter != null;
    }

    // Implements BluetoothAdapterAndroid::IsPowered.
    @CalledByNative
    private boolean isPowered() {
        return isPresent() && mAdapter.isEnabled();
    }

    // Implements BluetoothAdapterAndroid::IsDiscoverable.
    @CalledByNative
    private boolean isDiscoverable() {
        return isPresent()
                && mAdapter.getScanMode() == BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE;
    }

    // Implements BluetoothAdapterAndroid::IsDiscovering.
    @CalledByNative
    private boolean isDiscovering() {
        return isPresent() && mAdapter.isDiscovering();
    }
}
