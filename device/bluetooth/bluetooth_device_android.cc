// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_device_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "jni/ChromeBluetoothDevice_jni.h"

using base::android::AttachCurrentThread;

namespace device {

// Create a BluetoothDeviceAndroid instance and return pointer for
// an already created org.chromium.device.bluetooth.BluetoothDevice |obj|.
static jlong Init(JNIEnv* env, jobject obj) {
  return reinterpret_cast<intptr_t>(new BluetoothDeviceAndroid(env, obj));
}

// static
BluetoothDeviceAndroid* BluetoothDeviceAndroid::FromJavaObject(jobject obj) {
  CHECK(obj);
  return reinterpret_cast<BluetoothDeviceAndroid*>(
      Java_ChromeBluetoothDevice_getNativePointer(AttachCurrentThread(), obj));
}

BluetoothDeviceAndroid::BluetoothDeviceAndroid(JNIEnv* env, jobject obj) {
  CHECK(obj);
  j_device_.Reset(env, obj);
}

BluetoothDeviceAndroid::~BluetoothDeviceAndroid() {
}

// static
bool BluetoothDeviceAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);  // Generated in ChromeBluetoothDevice_jni.h
}

uint32 BluetoothDeviceAndroid::GetBluetoothClass() const {
  return Java_ChromeBluetoothDevice_getBluetoothClass(AttachCurrentThread(),
                                                      j_device_.obj());
}

std::string BluetoothDeviceAndroid::GetAddress() const {
  return ConvertJavaStringToUTF8(Java_ChromeBluetoothDevice_getAddress(
      AttachCurrentThread(), j_device_.obj()));
}

BluetoothDevice::VendorIDSource BluetoothDeviceAndroid::GetVendorIDSource()
    const {
  // Android API does not provide Vendor ID.
  return VENDOR_ID_UNKNOWN;
}

uint16 BluetoothDeviceAndroid::GetVendorID() const {
  // Android API does not provide Vendor ID.
  return 0;
}

uint16 BluetoothDeviceAndroid::GetProductID() const {
  // Android API does not provide Product ID.
  return 0;
}

uint16 BluetoothDeviceAndroid::GetDeviceID() const {
  // Android API does not provide Device ID.
  return 0;
}

bool BluetoothDeviceAndroid::IsPaired() const {
  return Java_ChromeBluetoothDevice_isPaired(AttachCurrentThread(),
                                             j_device_.obj());
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

void BluetoothDeviceAndroid::Disconnect(const base::Closure& callback,
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
  return ConvertJavaStringToUTF8(Java_ChromeBluetoothDevice_getDeviceName(
      AttachCurrentThread(), j_device_.obj()));
}

}  // namespace device
