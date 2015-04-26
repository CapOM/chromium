// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_BLUETOOTH_ADAPTER_OBSERVER_H_
#define DEVICE_BLUETOOTH_TEST_BLUETOOTH_ADAPTER_OBSERVER_H_

#include "device/bluetooth/bluetooth_adapter.h"

namespace device {

// Measures observed notifications from BluetoothAdapter::Observer.
class TestBluetoothAdapterObserver : public BluetoothAdapter::Observer {
 public:
  TestObserver(scoped_refptr<BluetoothAdapter> adapter);
  ~TestObserver() override;

  void ResetCounters();

  // BluetoothAdapter::Observer
  void AdapterPresentChanged(BluetoothAdapter* adapter, bool present) override;

  void AdapterPoweredChanged(BluetoothAdapter* adapter, bool powered) override;

  void AdapterDiscoverableChanged(BluetoothAdapter* adapter,
                                  bool discoverable) override;
  void AdapterDiscoveringChanged(BluetoothAdapter* adapter,
                                 bool discovering) override;

  void DeviceAdded(BluetoothAdapter* adapter, BluetoothDevice* device) override;

  void DeviceChanged(BluetoothAdapter* adapter,
                     BluetoothDevice* device) override;

  void DeviceRemoved(BluetoothAdapter* adapter,
                     BluetoothDevice* device) override;

  int present_changed_count_;
  int powered_changed_count_;
  int discoverable_changed_count_;
  int discovering_changed_count_;
  bool last_present_;
  bool last_powered_;
  bool last_discovering_;
  int device_added_count_;
  int device_changed_count_;
  int device_removed_count_;
  BluetoothDevice* last_device_;
  std::string last_device_address_;

 private:
  // Some tests use a message loop since background processing is simulated;
  // break out of those loops.
  void QuitMessageLoop();

  scoped_refptr<BluetoothAdapter> adapter_;
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_BLUETOOTH_ADAPTER_OBSERVER_H_
