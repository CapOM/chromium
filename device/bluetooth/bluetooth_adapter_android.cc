// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_android.h"

namespace device {

// static
base::WeakPtr<BluetoothAdapter> BluetoothAdapter::CreateAdapter(
    const InitCallback& init_callback) {
  return BluetoothAdapterAndroid::CreateAdapter();
}

// static
base::WeakPtr<BluetoothAdapter> BluetoothAdapterAndroid::CreateAdapter() {
  BluetoothAdapterAndroid* adapter = new BluetoothAdapterAndroid();
  return adapter->weak_ptr_factory_.GetWeakPtr();
}

std::string BluetoothAdapterAndroid::GetAddress() const {
  return address_;
}

std::string BluetoothAdapterAndroid::GetName() const {
  return name_;
}

void BluetoothAdapterAndroid::SetName(const std::string& name,
                                      const base::Closure& callback,
                                      const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsInitialized() const {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsPresent() const {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsPowered() const {
  NOTIMPLEMENTED();
}

void BluetoothAdapterAndroid::SetPowered(bool powered,
                                         const base::Closure& callback,
                                         const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsDiscoverable() const {
  NOTIMPLEMENTED();
}

void BluetoothAdapterAndroid::SetDiscoverable(
    bool discoverable,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsDiscovering() const {
  NOTIMPLEMENTED();
}

void BluetoothAdapterAndroid::DeleteOnCorrectThread() const {
  if (ui_task_runner_->RunsTasksOnCurrentThread() ||
      !ui_task_runner_->DeleteSoon(FROM_HERE, this))
    delete this;
}

void BluetoothAdapterAndroid::StartDiscoverySessionWithFilter(
    scoped_ptr<BluetoothDiscoveryFilter> discovery_filter,
    const DiscoverySessionCallback& callback,
    const ErrorCallback& error_callback) {
  error_callback.Run();
}

void BluetoothAdapterAndroid::StartDiscoverySession(
    const DiscoverySessionCallback& callback,
    const ErrorCallback& error_callback) {
  error_callback.Run();
}

void BluetoothAdapterAndroid::CreateRfcommService(
    const BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  error_callback.Run();
}

void BluetoothAdapterAndroid::CreateL2capService(
    const BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  error_callback.Run();
}

void BluetoothAdapterAndroid::RegisterAudioSink(
    const BluetoothAudioSink::Options& options,
    const AcquiredCallback& callback,
    const BluetoothAudioSink::ErrorCallback& error_callback) {
}

BluetoothAdapterAndroid::BluetoothAdapterAndroid() {
  ui_task_runner_ = base::ThreadTaskRunnerHandle::Get();
}

BluetoothAdapterAndroid::~BluetoothAdapterAndroid() {
}

void BluetoothAdapterAndroid::AddDiscoverySession(
    device::BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
}

void BluetoothAdapterAndroid::RemoveDiscoverySession(
    device::BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
}

void BluetoothAdapterAndroid::SetDiscoveryFilter(
    scoped_ptr<device::BluetoothDiscoveryFilter> discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
}

void BluetoothAdapterAndroid::RemovePairingDelegateInternal(
    device::BluetoothDevice::PairingDelegate* pairing_delegate) {
}

}  // namespace device
