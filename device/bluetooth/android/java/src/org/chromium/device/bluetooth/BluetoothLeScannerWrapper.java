// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.Manifest;
import android.annotation.TargetApi;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanCallback;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.Log;

import java.util.List;

/**
 * Wraps android.bluetooth.BluetoothLeScanner, pasing through to a provided
 * object. This indirection enables fake implementations when running tests.
 */
@JNINamespace("device")
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class BluetoothLeScannerWrapper {
    private static final String TAG = "cr.Bluetooth";
    private final BluetoothLeScanner mScanner;

    public BluetoothLeScannerWrapper(BluetoothLeScanner scanner) {
        mScanner = scanner;
    }

    public void startScan(List<ScanFilter> filters, ScanSettings settings, ScanCallback callback)
        scanner.startScan(filters, settings, callback);
    }

    public void stopScan(ScanCallback callback) {
        scanner.stopScan(callback);
    }
}
