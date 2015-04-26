// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter.h"

namespace device {

TestBluetoothAdapterObserver::TestObserver(
    scoped_refptr<BluetoothAdapter> adapter)
    : last_present_(false),
      last_powered_(false),
      last_discovering_(false),
      last_device_(NULL),
      adapter_(adapter) {
  ResetCounters();
  adapter_->AddObserver(this);
}

TestBluetoothAdapterObserver::~TestObserver() {
  adapter_->RemoveObserver(this);
}

void TestBluetoothAdapterObserver::ResetCounters() {
  int present_changed_count_;
  int powered_changed_count_;
  int discoverable_changed_count_;
  int discovering_changed_count_;
  int device_added_count_;
  int device_changed_count_;
  int device_removed_count_;
}

void TestBluetoothAdapterObserver::TestBluetoothAdapterObserver::
    AdapterPresentChanged(BluetoothAdapter* adapter, bool present) {
  EXPECT_EQ(adapter_.get(), adapter);

  ++present_changed_count_;
  last_present_ = present;
}

void TestBluetoothAdapterObserver::AdapterPoweredChanged(
    BluetoothAdapter* adapter,
    bool powered) {
  EXPECT_EQ(adapter_.get(), adapter);

  ++powered_changed_count_;
  last_powered_ = powered;
}

void TestBluetoothAdapterObserver::AdapterDiscoverableChanged(
    BluetoothAdapter* adapter,
    bool discoverable) {
  EXPECT_EQ(adapter_.get(), adapter);

  ++discoverable_changed_count_;
}

void TestBluetoothAdapterObserver::AdapterDiscoveringChanged(
    BluetoothAdapter* adapter,
    bool discovering) {
  EXPECT_EQ(adapter_.get(), adapter);

  ++discovering_changed_count_;
  last_discovering_ = discovering;
}

void TestBluetoothAdapterObserver::DeviceAdded(BluetoothAdapter* adapter,
                                               BluetoothDevice* device) {
  EXPECT_EQ(adapter_.get(), adapter);

  ++device_added_count_;
  last_device_ = device;
  last_device_address_ = device->GetAddress();

  QuitMessageLoop();
}

void TestBluetoothAdapterObserver::DeviceChanged(BluetoothAdapter* adapter,
                                                 BluetoothDevice* device) {
  EXPECT_EQ(adapter_.get(), adapter);

  ++device_changed_count_;
  last_device_ = device;
  last_device_address_ = device->GetAddress();

  QuitMessageLoop();
}

void TestBluetoothAdapterObserver::DeviceRemoved(BluetoothAdapter* adapter,
                                                 BluetoothDevice* device) {
  EXPECT_EQ(adapter_.get(), adapter);

  ++device_removed_count_;
  // Can't save device, it may be freed
  last_device_address_ = device->GetAddress();

  QuitMessageLoop();
}

void TestBluetoothAdapterObserver::QuitMessageLoop() {
  if (base::MessageLoop::current() &&
      base::MessageLoop::current()->is_running())
    base::MessageLoop::current()->Quit();
}

}  // namespace device
