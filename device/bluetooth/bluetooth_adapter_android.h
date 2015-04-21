// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_ANDROID_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_ANDROID_H_

#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace device {

// The BluetoothAdapterAndroid class implements BluetoothAdapter for the
// Android platform.
class DEVICE_BLUETOOTH_EXPORT BluetoothAdapterAndroid
    : public device::BluetoothAdapter {
 public:
  static base::WeakPtr<BluetoothAdapter> CreateAdapter();

  // BluetoothAdapter:
  std::string GetAddress() const override;
  std::string GetName() const override;
  void SetName(const std::string& name,
               const base::Closure& callback,
               const ErrorCallback& error_callback) override;
  bool IsInitialized() const override;
  bool IsPresent() const override;
  bool IsPowered() const override;
  void SetPowered(bool powered,
                  const base::Closure& callback,
                  const ErrorCallback& error_callback) override;
  bool IsDiscoverable() const override;
  void SetDiscoverable(bool discoverable,
                       const base::Closure& callback,
                       const ErrorCallback& error_callback) override;
  bool IsDiscovering() const override;
  void CreateRfcommService(
      const device::BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) override;
  void CreateL2capService(
      const device::BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) override;
  void RegisterAudioSink(
      const device::BluetoothAudioSink::Options& options,
      const device::BluetoothAdapter::AcquiredCallback& callback,
      const device::BluetoothAudioSink::ErrorCallback& error_callback) override;

 protected:
  BluetoothAdapterAndroid();
  ~BluetoothAdapterAndroid();

  // BluetoothAdapter:
  void AddDiscoverySession(device::BluetoothDiscoveryFilter* discovery_filter,
                           const base::Closure& callback,
                           const ErrorCallback& error_callback) override;
  void RemoveDiscoverySession(
      device::BluetoothDiscoveryFilter* discovery_filter,
      const base::Closure& callback,
      const ErrorCallback& error_callback) override;
  void SetDiscoveryFilter(
      scoped_ptr<device::BluetoothDiscoveryFilter> discovery_filter,
      const base::Closure& callback,
      const ErrorCallback& error_callback) override;
  void RemovePairingDelegateInternal(
      device::BluetoothDevice::PairingDelegate* pairing_delegate) override;

  std::string address_;
  std::string name_;

  // Task runner from the thread this object is created on.
  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothAdapterAndroid> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAdapterAndroid);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_ANDROID_H_
