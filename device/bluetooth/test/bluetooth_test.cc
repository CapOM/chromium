// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/bluetooth_test.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace device {

BluetoothTestBase::BluetoothTestBase() {
}

BluetoothTestBase::~BluetoothTestBase() {
}

void BluetoothTestBase::TearDown() {
  for (ScopedVector<BluetoothDiscoverySession>::iterator iter =
           discovery_sessions_.begin();
       iter != discovery_sessions_.end(); ++iter) {
    BluetoothDiscoverySession* session = *iter;
    if (!session->IsActive())
      continue;
    callback_count_ = 0;
    session->Stop(GetCallback(), GetErrorCallback());
    WaitForCallbacks();
    ASSERT_EQ(1, callback_count_--);
  }
  discovery_sessions_.clear();
}

void BluetoothTestBase::Callback() {
  ++callback_count_;
}

void BluetoothTestBase::DiscoverySessionCallback(
    scoped_ptr<BluetoothDiscoverySession> discovery_session) {
  ++callback_count_;
  discovery_sessions_.push_back(discovery_session.release());
}

void BluetoothTestBase::ErrorCallback() {
  ++error_callback_count_;
}

base::Closure BluetoothTestBase::GetCallback() {
  return base::Bind(&BluetoothTestBase::Callback, base::Unretained(this));
}

BluetoothAdapter::DiscoverySessionCallback
BluetoothTestBase::GetDiscoverySessionCallback() {
  return base::Bind(&BluetoothTestBase::DiscoverySessionCallback,
                    base::Unretained(this));
}

BluetoothAdapter::ErrorCallback BluetoothTestBase::GetErrorCallback() {
  return base::Bind(&BluetoothTestBase::ErrorCallback, base::Unretained(this));
}

void BluetoothTestBase::WaitForCallbacks() {
  base::RunLoop().RunUntilIdle();

//base::RunLoop run_loop;
//message_loop_.task_runner()->PostTask(FROM_HERE, run_loop.QuitClosure());
//run_loop.Run();
  
}

}  // namespace device
