// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.annotation.TargetApi;
import android.os.Build;

import org.chromium.base.CalledByNative;
import org.chromium.base.Log;

import java.lang.IllegalArgumentException;

/**
 * Fakes android.bluetooth.le.BluetoothLeScanner.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class FakeBluetoothLeScanner extends BluetoothLeScannerWrapper {
    private static final String TAG = "cr.Bluetooth";
    private ScanCallback mCallback;

    private FakeBluetoothLeScanner() {
        super(null);
    }

    public void startScan(List<ScanFilter> filters, ScanSettings settings, ScanCallback callback)
        if (callback != null) {
            throw new IllegalArgumentException(
                "FakeBluetoothLeScanner implementation does not support multiple scans.");
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
