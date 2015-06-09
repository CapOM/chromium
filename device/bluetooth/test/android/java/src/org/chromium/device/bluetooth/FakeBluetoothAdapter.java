// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.annotation.TargetApi;
import android.bluetooth.le.BluetoothLeScanner;
import android.os.Build;

import org.chromium.base.CalledByNative;

/**
 * Fakes android.bluetooth.BluetoothAdapter.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class FakeBluetoothAdapter extends BluetoothAdapterWrapper {
    /**
     * Creates a FakeBluetoothAdapter.
     */
    @CalledByNative
    public static FakeBluetoothAdapter create() {
        return new FakeBluetoothAdapter();
    }

    public FakeBluetoothAdapter() {
        super(null);
    }

    public BluetoothLeScanner getBluetoothLeScanner() {
        return null;
    }

    public boolean isEnabled() {
        return true;
    }

    public String getAddress() {
        return "A1:B2:C3:D4:E5:F6";
    }

    public String getName() {
        return "FakeBluetoothAdapter";
    }

    public int getScanMode() {
        return android.bluetooth.BluetoothAdapter.SCAN_MODE_NONE;
    }

    public boolean isDiscovering() {
        return false;
    }
}
