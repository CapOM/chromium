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

import java.lang.Override;
import java.util.List;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class Fakes {
    /**
     * Fakes android.bluetooth.BluetoothAdapter.
     */
    public static class FakeBluetoothAdapter extends Wrappers.BluetoothAdapterWrapper {
        private static final String TAG = "cr.Bluetooth";

        /**
         * Creates a FakeBluetoothAdapter.
         */
        @CalledByNative("FakeBluetoothAdapter")
        public static FakeBluetoothAdapter create() {
            Log.i(TAG, "FakeBluetoothAdapter created.");
            return new FakeBluetoothAdapter();
        }

        private FakeBluetoothAdapter() {
            super(null, new FakeBluetoothLeScanner());
        }

        /**
         * Creates and discovers a new device.
         */
        @CalledByNative("FakeBluetoothAdapter")
        public void DiscoverANewDevice() {}

        // ---------------------------------------------------------------------------------------------
        // BluetoothAdapterWrapper overrides:

        @Override
        public boolean isEnabled() {
            return true;
        }

        @Override
        public String getAddress() {
            return "A1:B2:C3:D4:E5:F6";
        }

        @Override
        public String getName() {
            return "FakeBluetoothAdapter";
        }

        @Override
        public int getScanMode() {
            return android.bluetooth.BluetoothAdapter.SCAN_MODE_NONE;
        }

        @Override
        public boolean isDiscovering() {
            return false;
        }
    }

    /**
     * Fakes android.bluetooth.le.BluetoothLeScanner.
     */
    public static class FakeBluetoothLeScanner
            extends Wrappers.BluetoothLeScannerWrapper {
        private ScanCallback mCallback;

        private FakeBluetoothLeScanner() {
            super(null);
        }

        @Override
        public void startScan(
                List<ScanFilter> filters, ScanSettings settings, ScanCallback callback) {
            if (mCallback != null) {
                throw new IllegalArgumentException(
                        "FakeBluetoothLeScanner does not support multiple scans.");
            }
            mCallback = callback;
        }

        @Override
        public void stopScan(ScanCallback callback) {
            if (mCallback != callback) {
                throw new IllegalArgumentException("No scan in progress.");
            }
            mCallback = null;
        }
    }

    /**
     * Fakes android.bluetooth.le.ScanResult
     */
    public static class FakeScanResult extends Wrappers.ScanResultWrapper {
        FakeScanResult() {
            super(null);
        }
    }
}
