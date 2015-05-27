// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "device/bluetooth/bluetooth_advertisement.h"
#include "jni/BluetoothAdapter_jni.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;

namespace device {

// static
base::WeakPtr<BluetoothAdapter> BluetoothAdapter::CreateAdapter(
    const InitCallback& init_callback) {
  return BluetoothAdapterAndroid::CreateAdapter();
}

// static
base::WeakPtr<BluetoothAdapterAndroid>
BluetoothAdapterAndroid::CreateAdapter() {
  BluetoothAdapterAndroid* adapter = new BluetoothAdapterAndroid();
  adapter->j_bluetooth_adapter_.Reset(Java_BluetoothAdapter_create(
      AttachCurrentThread(), base::android::GetApplicationContext(),
      reinterpret_cast<jlong>(adapter)));
  return adapter->weak_ptr_factory_.GetWeakPtr();
}

base::WeakPtr<BluetoothAdapterAndroid>
BluetoothAdapterAndroid::CreateAdapterWithoutPermissionForTesting() {
  BluetoothAdapterAndroid* adapter = new BluetoothAdapterAndroid();
  adapter->j_bluetooth_adapter_.Reset(
      Java_BluetoothAdapter_createWithoutPermissionForTesting(
          AttachCurrentThread(), base::android::GetApplicationContext(),
          reinterpret_cast<jlong>(adapter)));
  return adapter->weak_ptr_factory_.GetWeakPtr();
}

// static
bool BluetoothAdapterAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);  // Generated in BluetoothAdapter_jni.h
}

bool BluetoothAdapterAndroid::HasBluetoothCapability() const {
  return Java_BluetoothAdapter_hasBluetoothCapability(
      AttachCurrentThread(), j_bluetooth_adapter_.obj());
}

std::string BluetoothAdapterAndroid::GetAddress() const {
  return ConvertJavaStringToUTF8(Java_BluetoothAdapter_getAddress(
      AttachCurrentThread(), j_bluetooth_adapter_.obj()));
}

std::string BluetoothAdapterAndroid::GetName() const {
  return ConvertJavaStringToUTF8(Java_BluetoothAdapter_getName(
      AttachCurrentThread(), j_bluetooth_adapter_.obj()));
}

void BluetoothAdapterAndroid::SetName(const std::string& name,
                                      const base::Closure& callback,
                                      const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsInitialized() const {
  return true;
}

bool BluetoothAdapterAndroid::IsPresent() const {
  return Java_BluetoothAdapter_isPresent(AttachCurrentThread(),
                                         j_bluetooth_adapter_.obj());
}

bool BluetoothAdapterAndroid::IsPowered() const {
  return Java_BluetoothAdapter_isPowered(AttachCurrentThread(),
                                         j_bluetooth_adapter_.obj());
}

void BluetoothAdapterAndroid::SetPowered(bool powered,
                                         const base::Closure& callback,
                                         const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsDiscoverable() const {
  return Java_BluetoothAdapter_isDiscoverable(AttachCurrentThread(),
                                              j_bluetooth_adapter_.obj());
}

void BluetoothAdapterAndroid::SetDiscoverable(
    bool discoverable,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsDiscovering() const {
  return Java_BluetoothAdapter_isDiscovering(AttachCurrentThread(),
                                             j_bluetooth_adapter_.obj());
}

void BluetoothAdapterAndroid::CreateRfcommService(
    const BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
  error_callback.Run("Not Implemented");
}

void BluetoothAdapterAndroid::CreateL2capService(
    const BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
  error_callback.Run("Not Implemented");
}

void BluetoothAdapterAndroid::RegisterAudioSink(
    const BluetoothAudioSink::Options& options,
    const AcquiredCallback& callback,
    const BluetoothAudioSink::ErrorCallback& error_callback) {
  error_callback.Run(BluetoothAudioSink::ERROR_UNSUPPORTED_PLATFORM);
}

void BluetoothAdapterAndroid::RegisterAdvertisement(
    scoped_ptr<BluetoothAdvertisement::Data> advertisement_data,
    const CreateAdvertisementCallback& callback,
    const CreateAdvertisementErrorCallback& error_callback) {
  error_callback.Run(BluetoothAdvertisement::ERROR_UNSUPPORTED_PLATFORM);
}

void BluetoothAdapterAndroid::OnScanFailed(JNIEnv* env, jobject obj) {
  MarkDiscoverySessionsAsInactive();
}

void BluetoothAdapterAndroid::OnDeviceDiscovered(JNIEnv* env, jobject obj) {
  BluetoothDeviceAndroid* device = BluetoothDeviceAndroid::FromJavaObject(obj);

  const std::string& address = device_chromeos->GetAddress();
  if (devices_.count(address) == 0) {
    devices_[address] = device;

    FOR_EACH_OBSERVER(BluetoothAdapter::Observer, observers_,
                      DeviceAdded(this, device));
  }
}

BluetoothAdapterAndroid::BluetoothAdapterAndroid() : weak_ptr_factory_(this) {
}

BluetoothAdapterAndroid::~BluetoothAdapterAndroid() {
  Java_BluetoothAdapter_onBluetoothAdapterAndroidDestruction(
      AttachCurrentThread(), j_bluetooth_adapter_.obj());
}

void BluetoothAdapterAndroid::AddDiscoverySession(
    BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  // TODO(scheib): Support filters crbug.com/490401
  if (Java_BluetoothAdapter_addDiscoverySession(AttachCurrentThread(),
                                                j_bluetooth_adapter_.obj())) {
    callback.Run();
  } else {
    error_callback.Run();
  }
}

void BluetoothAdapterAndroid::RemoveDiscoverySession(
    BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  if (Java_BluetoothAdapter_removeDiscoverySession(
          AttachCurrentThread(), j_bluetooth_adapter_.obj())) {
    callback.Run();
  } else {
    error_callback.Run();
  }
}

void BluetoothAdapterAndroid::SetDiscoveryFilter(
    scoped_ptr<BluetoothDiscoveryFilter> discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
  error_callback.Run();
}

void BluetoothAdapterAndroid::RemovePairingDelegateInternal(
    device::BluetoothDevice::PairingDelegate* pairing_delegate) {
}

}  // namespace device
