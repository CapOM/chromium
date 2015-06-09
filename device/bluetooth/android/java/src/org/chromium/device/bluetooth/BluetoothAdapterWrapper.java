// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

/**
 * Wraps android.bluetooth.BluetoothAdapter, pasing through to a provided
 * object. This indirection enables fake implementations when running tests.
 */
public class BluetoothAdapterWrapper {
    private final android.bluetooth.BluetoothAdapter mAdapter;

    public static BluetoothAdapterWrapper getDefaultAdapter() {
        android.bluetooth.BluetoothAdapter adapter =
                android.bluetooth.BluetoothAdapter.getDefaultAdapter();
        if (adapter == null) {
            return null;
        }
        return new BluetoothAdapterWrapper(adapter);
    }

    public BluetoothAdapterWrapper(android.bluetooth.BluetoothAdapter adapter) {
        assert adapter != null;
        mAdapter = adapter;
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
