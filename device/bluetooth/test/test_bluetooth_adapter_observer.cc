// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/test_bluetooth_adapter_observer.h"

#include "base/message_loop/message_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TestBluetoothAdapterObserver::TestBluetoothAdapterObserver(
    scoped_refptr<BluetoothAdapter> adapter)
    : adapter_(adapter) {
  ResetCounters();
  adapter_->AddObserver(this);
}

TestBluetoothAdapterObserver::~TestBluetoothAdapterObserver() {
  adapter_->RemoveObserver(this);
}

void TestBluetoothAdapterObserver::ResetCounters() {
  present_changed_count_ = 0;
  powered_changed_count_ = 0;
  discoverable_changed_count_ = 0;
  discovering_changed_count_ = 0;
  device_added_count_ = 0;
  device_changed_count_ = 0;
  device_removed_count_ = 0;
}

void TestBluetoothAdapterObserver::AdapterPresentChanged(BluetoothAdapter* adapter, bool present) {
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
