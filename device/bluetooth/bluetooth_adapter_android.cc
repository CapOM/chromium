// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "device/bluetooth/android/bluetooth_adapter_wrapper.h"
#include "device/bluetooth/bluetooth_advertisement.h"
#include "device/bluetooth/bluetooth_device_android.h"
#include "jni/ChromeBluetoothAdapter_jni.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;

namespace device {

// static
base::WeakPtr<BluetoothAdapter> BluetoothAdapter::CreateAdapter(
    const InitCallback& init_callback) {
  return BluetoothAdapterAndroid::Create(
      BluetoothAdapterWrapper::CreateWithDefaultAdapter().obj());
}

// static
base::WeakPtr<BluetoothAdapterAndroid> BluetoothAdapterAndroid::Create(
    jobject java_bluetooth_adapter_wrapper) {
  BluetoothAdapterAndroid* adapter = new BluetoothAdapterAndroid();

  adapter->j_bluetooth_adapter_.Reset(Java_ChromeBluetoothAdapter_create(
      AttachCurrentThread(), java_bluetooth_adapter_wrapper));

  return adapter->weak_ptr_factory_.GetWeakPtr();
}

// static
bool BluetoothAdapterAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);  // Generated in BluetoothAdapter_jni.h
}

std::string BluetoothAdapterAndroid::GetAddress() const {
  return ConvertJavaStringToUTF8(Java_ChromeBluetoothAdapter_getAddress(
      AttachCurrentThread(), j_bluetooth_adapter_.obj()));
}

std::string BluetoothAdapterAndroid::GetName() const {
  return ConvertJavaStringToUTF8(Java_ChromeBluetoothAdapter_getName(
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
  return Java_ChromeBluetoothAdapter_isPresent(AttachCurrentThread(),
                                               j_bluetooth_adapter_.obj());
}

bool BluetoothAdapterAndroid::IsPowered() const {
  return Java_ChromeBluetoothAdapter_isPowered(AttachCurrentThread(),
                                               j_bluetooth_adapter_.obj());
}

void BluetoothAdapterAndroid::SetPowered(bool powered,
                                         const base::Closure& callback,
                                         const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsDiscoverable() const {
  return Java_ChromeBluetoothAdapter_isDiscoverable(AttachCurrentThread(),
                                                    j_bluetooth_adapter_.obj());
}

void BluetoothAdapterAndroid::SetDiscoverable(
    bool discoverable,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterAndroid::IsDiscovering() const {
  return Java_ChromeBluetoothAdapter_isDiscovering(AttachCurrentThread(),
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

void BluetoothAdapterAndroid::OnDeviceAdded(JNIEnv* env,
                                            jobject obj,
                                            jobject device_android) {
  BluetoothDeviceAndroid* device =
      BluetoothDeviceAndroid::FromJavaObject(device_android);

  const std::string& address = device->GetAddress();
  if (devices_.count(address) == 0) {
    devices_[address] = device;

    FOR_EACH_OBSERVER(BluetoothAdapter::Observer, observers_,
                      DeviceAdded(this, device));
  } else {
    LOG(WARNING) << "Redundant device found"; // TODO??? Need this log? Common?
    // TODO(scheib): Refactor to check first before constructing everything?
    delete device;
  }
}

BluetoothAdapterAndroid::BluetoothAdapterAndroid() : weak_ptr_factory_(this) {
}

BluetoothAdapterAndroid::~BluetoothAdapterAndroid() {
  // TODO(scheib): Delete devices_.
  Java_ChromeBluetoothAdapter_onBluetoothAdapterAndroidDestruction(
      AttachCurrentThread(), j_bluetooth_adapter_.obj());
}

void BluetoothAdapterAndroid::AddDiscoverySession(
    BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  // TODO(scheib): Support filters crbug.com/490401
  if (Java_ChromeBluetoothAdapter_addDiscoverySession(AttachCurrentThread(),
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
  if (Java_ChromeBluetoothAdapter_removeDiscoverySession(
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
  // TODO(scheib): Support filters crbug.com/490401
  NOTIMPLEMENTED();
  error_callback.Run();
}

void BluetoothAdapterAndroid::RemovePairingDelegateInternal(
    device::BluetoothDevice::PairingDelegate* pairing_delegate) {
}

}  // namespace device
