// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.Manifest;
import android.annotation.TargetApi;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.PackageManager;
import android.os.Build;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.Log;
import org.chromium.device.bluetooth.BluetoothDevice;

import java.util.List;

/**
 * Exposes android.bluetooth.BluetoothAdapter as necessary for C++
 * device::BluetoothAdapterAndroid.
 *
 * Lifetime is controlled by device::BluetoothAdapterAndroid.
 */
@JNINamespace("device")
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
final class BluetoothAdapter {
    private static final String TAG = Log.makeTag("Bluetooth");

    private long mNativeBluetoothAdapterAndroid;
    private final boolean mHasBluetoothCapability;
    private android.bluetooth.BluetoothAdapter mAdapter;
    private int mNumDiscoverySessions;
    private ScanCallback mLeScanCallback;

    // ---------------------------------------------------------------------------------------------
    // Construction and handler for C++ object destruction.

    // Constructs a BluetoothAdapter.
    private BluetoothAdapter(long nativeBluetoothAdapterAndroid, boolean hasPermissions,
            boolean hasLowEnergyFeature, android.bluetooth.BluetoothAdapter adapter) {
        mNativeBluetoothAdapterAndroid = nativeBluetoothAdapterAndroid;
        final boolean hasMinAPI = Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
        // Only Low Energy currently supported, see BluetoothAdapterAndroid class note.
        mHasBluetoothCapability = hasMinAPI && hasPermissions && hasLowEnergyFeature;
        if (!mHasBluetoothCapability) {
            if (!hasMinAPI) {
                Log.i(TAG, "Bluetooth API disabled; SDK version (%d) too low.",
                        Build.VERSION.SDK_INT);
            } else if (!hasPermissions) {
                Log.w(TAG, "Bluetooth API disabled; BLUETOOTH and BLUETOOTH_ADMIN permissions "
                                + "required.");
            } else if (!hasLowEnergyFeature) {
                Log.i(TAG, "Bluetooth API disabled; Low Energy not supported on system.");
            }
            return;
        }

        if (adapter == null) {
            mAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
            if (mAdapter == null) {
                Log.i(TAG, "BluetoothAdapter initialized, but default adapter not found.");
            } else {
                Log.i(TAG, "BluetoothAdapter initialized with default adapter.");
            }
        } else {
            mAdapter = adapter;
            Log.i(TAG, "BluetoothAdapter initialized with provided adapter.");
        }
    }

    /**
     * Shuts down any ongoing scan, removes reference to the C++ object.
     */
    @CalledByNative
    private void onBluetoothAdapterAndroidDestruction() {
        stopScan();
        mNativeBluetoothAdapterAndroid = 0;
    }

    // ---------------------------------------------------------------------------------------------
    // Public Methods:

    public void onDeviceAdded(BluetoothDevice bluetoothDevice) {
        nativeOnDeviceAdded(mNativeBluetoothAdapterAndroid, bluetoothDevice);
    }

    // ---------------------------------------------------------------------------------------------
    // BluetoothAdapterAndroid methods implemented in java:

    // Implements BluetoothAdapterAndroid::CreateAdapter.
    @CalledByNative
    private static BluetoothAdapter create(Context context, long nativeBluetoothAdapterAndroid) {
        final boolean hasPermissions =
                context.checkCallingOrSelfPermission(Manifest.permission.BLUETOOTH)
                        == PackageManager.PERMISSION_GRANTED
                && context.checkCallingOrSelfPermission(Manifest.permission.BLUETOOTH_ADMIN)
                        == PackageManager.PERMISSION_GRANTED;
        final boolean hasLowEnergyFeature =
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2
                && context.getPackageManager().hasSystemFeature(
                           PackageManager.FEATURE_BLUETOOTH_LE);
        return new BluetoothAdapter(nativeBluetoothAdapterAndroid, hasPermissions,
                hasLowEnergyFeature, /* adapter */ null);
    }

    // Implements BluetoothAdapterAndroid::CreateAdapterWithoutPermissionForTesting.
    @CalledByNative
    private static BluetoothAdapter createWithoutPermissionForTesting(
            Context context, long nativeBluetoothAdapterAndroid) {
        return new BluetoothAdapter(nativeBluetoothAdapterAndroid, /* hasPermissions */ false,
                /* hasLowEnergyFeature */ true, /* adapter */ null);
    }

    // Implements BluetoothAdapterAndroid::CreateAdapterWithFakeAdapterForTesting.
    @CalledByNative
    private static BluetoothAdapter createWithFakeAdapterForTesting(
            Context context, long nativeBluetoothAdapterAndroid) {
        // TODO:  A real testing adapter.
        android.bluetooth.BluetoothAdapter testingAdapter =
                android.bluetooth.BluetoothAdapter.getDefaultAdapter();
        BluetoothAdapter adapter = new BluetoothAdapter(nativeBluetoothAdapterAndroid,
                /* hasPermissions */ true, /* hasLowEnergyFeature */ true, testingAdapter);
        return adapter;
    }

    // Implements BluetoothAdapterAndroid::HasBluetoothCapability.
    @CalledByNative
    private boolean hasBluetoothCapability() {
        return mHasBluetoothCapability;
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
                && mAdapter.getScanMode()
                == android.bluetooth.BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE;
    }

    // Implements BluetoothAdapterAndroid::IsDiscovering.
    @CalledByNative
    private boolean isDiscovering() {
        return isPresent() && (mAdapter.isDiscovering() || mLeScanCallback != null);
    }

    // Implements BluetoothAdapterAndroid::AddDiscoverySession.
    @CalledByNative
    private boolean addDiscoverySession() {
        if (!isPowered()) {
            Log.d(TAG, "addDiscoverySession: Fails: !isPowered");
            return false;
        }

        mNumDiscoverySessions++;
        Log.d(TAG, "addDiscoverySession: Now %d sessions.", mNumDiscoverySessions);
        if (mNumDiscoverySessions > 1) {
            return true;
        }

        if (startScan()) {
            return true;
        } else {
            mNumDiscoverySessions--;
            return false;
        }
    }

    // Implements BluetoothAdapterAndroid::RemoveDiscoverySession.
    @CalledByNative
    private boolean removeDiscoverySession() {
        if (mNumDiscoverySessions == 0) {
            assert false;
            Log.w(TAG, "removeDiscoverySession: No scan in progress.");
            return false;
        }

        --mNumDiscoverySessions;

        if (mNumDiscoverySessions == 0) {
            Log.d(TAG, "removeDiscoverySession: Now 0 sessions. Stopping scan.");
            return stopScan();
        } else {
            Log.d(TAG, "removeDiscoverySession: Now %d sessions.", mNumDiscoverySessions);
        }
        return true;
    }

    // ---------------------------------------------------------------------------------------------
    // Implementation details:

    /**
     * Starts a Low Energy scan.
     * @return True on success.
     */
    private boolean startScan() {
        // ScanSettings Note: SCAN_FAILED_FEATURE_UNSUPPORTED is caused (at least on some devices)
        // if setReportDelay() is used or if SCAN_MODE_LOW_LATENCY isn't used.
        ScanSettings scanSettings =
                new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();

        assert mLeScanCallback == null;
        mLeScanCallback = new DiscoveryScanCallback();

        try {
            mAdapter.getBluetoothLeScanner().startScan(
                    null /* filters */, scanSettings, mLeScanCallback);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Cannot start scan: " + e);
            return false;
        }
        return true;
    }

    /**
     * Stops the Low Energy scan.
     * @return True if a scan was in progress.
     */
    private boolean stopScan() {
        if (mLeScanCallback == null) {
            return false;
        }
        mAdapter.getBluetoothLeScanner().stopScan(mLeScanCallback);
        mLeScanCallback = null;
        return true;
    }

    /**
     * Implements callbacks used during a Low Energy scan by notifying upon
     * devices discovered or detecting a scan failure.
     */
    private class DiscoveryScanCallback extends ScanCallback {
        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            Log.v(TAG, "onBatchScanResults");
        }

        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            Log.v(TAG, "onScanResult %s %s", result.getDevice().getAddress(),
                    result.getDevice().getName());
            new BluetoothDevice(result.getDevice(), BluetoothAdapter.this);
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.w(TAG, "onScanFailed: %d", errorCode);
            nativeOnScanFailed(mNativeBluetoothAdapterAndroid);
            mNumDiscoverySessions = 0;
        }
    }

    // ---------------------------------------------------------------------------------------------
    // bluetooth_adapter_android.cc C++ methods declared for access from java:

    // Binds to BluetoothAdapterAndroid::OnScanFailed.
    private native void nativeOnScanFailed(long nativeBluetoothAdapterAndroid);

    // Binds to BluetoothAdapterAndroid::OnDeviceAdded.
    private native void nativeOnDeviceAdded(
            long nativeBluetoothAdapterAndroid, BluetoothDevice bluetoothDevice);
}
