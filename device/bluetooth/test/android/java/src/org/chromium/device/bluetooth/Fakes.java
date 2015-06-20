// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import java.lang.Override;
import java.util.List;

import org.chromium.base.CalledByNative;
import org.chromium.base.Log;
import org.chromium.device.bluetooth.Wrappers.ScanResultWrapper;

import android.annotation.TargetApi;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanSettings;
import android.os.Build;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class Fakes {
    private static final String TAG = "cr.Bluetooth";

    /**
     * Fakes android.bluetooth.BluetoothAdapter.
     */
    public static class FakeBluetoothAdapter extends Wrappers.BluetoothAdapterWrapper {
        private final FakeBluetoothLeScanner mFakeScanner;

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
            mFakeScanner = (FakeBluetoothLeScanner)mScanner;
        }

        /**
         * Creates and discovers a new device.
         */
        @CalledByNative("FakeBluetoothAdapter")
        public void discoverANewDevice() {
            mFakeScanner.mCallback.onScanResultWrapper(
                    ScanSettings.CALLBACK_TYPE_ALL_MATCHES, // TODO(scheib) Check what the actual
                    // system is reporting and use that.
                    new FakeScanResult(new FakeBluetoothDevice()));
        }

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
    public static class FakeBluetoothLeScanner extends Wrappers.BluetoothLeScannerWrapper {
        public Wrappers.ScanCallbackWrapper mCallback;

        private FakeBluetoothLeScanner() {
            super(null);
        }

        @Override
        public void startScan(List<ScanFilter> filters, ScanSettings settings,
                Wrappers.ScanCallbackWrapper callback) {
            if (mCallback != null) {
                throw new IllegalArgumentException(
                        "FakeBluetoothLeScanner does not support multiple scans.");
            }
            mCallback = callback;
        }

        @Override
        public void stopScan(Wrappers.ScanCallbackWrapper callback) {
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
        private final FakeBluetoothDevice mDevice;

        FakeScanResult(FakeBluetoothDevice device) {
            super(null);
            mDevice = device;
        }

        @Override
        public Wrappers.BluetoothDeviceWrapper getDevice() {
            return mDevice;
        }
    }

    /**
     * Fakes android.bluetooth.BluetoothDevice.
     */
    public static class FakeBluetoothDevice extends Wrappers.BluetoothDeviceWrapper {
        private final String mAddress = "A1:B2:C3:DD:DD:DD";
        private String mName = "FakeBluetoothDevice";

        public FakeBluetoothDevice() {
            super(null);
        }

        @Override
        public String getAddress() {
            return mAddress;
        }

        @Override
        public String getName() {
            return mName;
        }
    }

}
