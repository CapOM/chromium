// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

/**
 * Wraps android.bluetooth.BluetoothDevice, pasing through to a provided object.
 * This indirection enables fake implementations when running tests.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class BluetoothAdapterWrapper {
    private static final String TAG = Log.makeTag("Bluetooth");
    final android.bluetooth.BluetoothDevice mAdapter;

    BluetoothAdapterWrapper(android.bluetooth.BluetoothDevice adapter) {
        mAdapter = adapter;
    }

    // 481
    public BluetoothLeScanner getBluetoothLeScanner() {
        return mAdapter.getBluetoothLeScanner();
    }

    // 501
    public boolean isEnabled() {
        return mAdapter.isEnabled();
    }

    // 633
    public String getAddress() {
        return mAdapter.getAddress();
    }

    // 647
    public String getName() {
        return mAdapter.getName();
    }

    // 732
    public int getScanMode() {
        return mAdapter.getScanMode();
    }

    // 894
    public String isDiscovering() {
        return mAdapter.isDiscovering();
    }
}

public class BluetoothAdapterTestWrapper extends BluetoothAdapterWrapper {
    @overrides
    public boolean isEnabled() {
    }

}

