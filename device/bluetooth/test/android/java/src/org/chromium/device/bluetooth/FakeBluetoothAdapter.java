// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.annotation.TargetApi;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanSettings;
import android.os.Build;

import org.chromium.base.CalledByNative;
import org.chromium.base.Log;

import java.util.List;

/**
 * Fakes android.bluetooth.BluetoothAdapter.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class FakeBluetoothAdapter extends BluetoothAdapterWrapper {
    private static final String TAG = "cr.Bluetooth";

    /**
     * Creates a FakeBluetoothAdapter.
     */
    @CalledByNative
    public static FakeBluetoothAdapter create() {
        Log.i(TAG, "FakeBluetoothAdapter created.");
        return new FakeBluetoothAdapter();
    }

    private FakeBluetoothAdapter() {
        super(null, new FakeBluetoothLeScanner());
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

    /**
     * Fakes android.bluetooth.le.BluetoothLeScanner.
     */
    public static class FakeBluetoothLeScanner extends BluetoothAdapterWrapper.BluetoothLeScannerWrapper {
        private ScanCallback mCallback;

        private FakeBluetoothLeScanner() {
            super(null);
        }

        public void startScan(
                List<ScanFilter> filters, ScanSettings settings, ScanCallback callback) {
            if (callback != null) {
                throw new IllegalArgumentException(
                        "FakeBluetoothLeScanner does not support multiple scans.");
            }
            mCallback = callback;
        }

        public void stopScan(ScanCallback callback) {
            if (callback != mCallback) {
                throw new IllegalArgumentException("No scan in progress.");
            }
            callback = null;
        }
    }
}
