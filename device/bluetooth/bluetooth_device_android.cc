// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_device_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "jni/BluetoothDevice_jni.h"

using base::android::AttachCurrentThread;

namespace device {

uint32 BluetoothDeviceAndroid::GetBluetoothClass() const {
  NOTIMPLEMENTED();
  return 0;
}

std::string BluetoothDeviceAndroid::GetAddress() const {
  NOTIMPLEMENTED();
  return "";
}

BluetoothDevice::VendorIDSource
BluetoothDeviceAndroid::GetVendorIDSource() const {
  NOTIMPLEMENTED();
  return VENDOR_ID_UNKNOWN;
}

uint16 BluetoothDeviceAndroid::GetVendorID() const {
  NOTIMPLEMENTED();
  return 0;
}

uint16 BluetoothDeviceAndroid::GetProductID() const {
  NOTIMPLEMENTED();
  return 0;
}

uint16 BluetoothDeviceAndroid::GetDeviceID() const {
  NOTIMPLEMENTED();
  return 0;
}

bool BluetoothDeviceAndroid::IsPaired() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceAndroid::IsConnected() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceAndroid::IsConnectable() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceAndroid::IsConnecting() const {
  NOTIMPLEMENTED();
  return false;
}

BluetoothDevice::UUIDList BluetoothDeviceAndroid::GetUUIDs() const {
  NOTIMPLEMENTED();
  return BluetoothDevice::UUIDList();
}

int16 BluetoothDeviceAndroid::GetInquiryRSSI() const {
  NOTIMPLEMENTED();
  return kUnknownPower;
}

int16 BluetoothDeviceAndroid::GetInquiryTxPower() const {
  NOTIMPLEMENTED();
  return kUnknownPower;
}

bool BluetoothDeviceAndroid::ExpectingPinCode() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceAndroid::ExpectingPasskey() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceAndroid::ExpectingConfirmation() const {
  NOTIMPLEMENTED();
  return false;
}

void BluetoothDeviceAndroid::GetConnectionInfo(
    const ConnectionInfoCallback& callback) {
  NOTIMPLEMENTED();
  callback.Run(ConnectionInfo());
}

void BluetoothDeviceAndroid::Connect(
    PairingDelegate* pairing_delegate,
    const base::Closure& callback,
    const ConnectErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::SetPinCode(const std::string& pincode) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::SetPasskey(uint32 passkey) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::ConfirmPairing() {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::RejectPairing() {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::CancelPairing() {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::Disconnect(
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::Forget(const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::ConnectToService(
    const BluetoothUUID& uuid,
    const ConnectToServiceCallback& callback,
    const ConnectToServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::ConnectToServiceInsecurely(
    const BluetoothUUID& uuid,
    const ConnectToServiceCallback& callback,
    const ConnectToServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceAndroid::CreateGattConnection(
      const GattConnectionCallback& callback,
      const ConnectErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

std::string BluetoothDeviceAndroid::GetDeviceName() const {
  NOTIMPLEMENTED();
  return "";
}

}  // namespace device
