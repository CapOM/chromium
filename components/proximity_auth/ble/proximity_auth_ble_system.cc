// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/ble/proximity_auth_ble_system.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/thread_task_runner_handle.h"
#include "components/proximity_auth/ble/bluetooth_low_energy_connection.h"
#include "components/proximity_auth/ble/bluetooth_low_energy_connection_finder.h"
#include "components/proximity_auth/ble/fake_wire_message.h"
#include "components/proximity_auth/connection.h"
#include "components/proximity_auth/cryptauth/base64url.h"
#include "components/proximity_auth/cryptauth/cryptauth_client.h"
#include "components/proximity_auth/remote_device.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"

namespace proximity_auth {

namespace {

// The UUID of the Bluetooth Low Energy service.
const char kSmartLockServiceUUID[] = "b3b7e28e-a000-3e17-bd86-6e97b9e28c11";

// The UUID of the characteristic used to send data to the peripheral.
const char kToPeripheralCharUUID[] = "977c6674-1239-4e72-993b-502369b8bb5a";

// The UUID of the characteristic used to receive data from the peripheral.
const char kFromPeripheralCharUUID[] = "f4b904a2-a030-43b3-98a8-221c536c03cb";

// Polling interval in seconds.
const int kPollingIntervalSeconds = 5;

// String received when the remote device's screen is unlocked.
const char kScreenUnlocked[] = "Screen Unlocked";

// String send to poll the remote device screen state.
const char kPollScreenState[] = "PollScreenState";

// BluetoothLowEnergyConnection parameter, number of attempts to send a write
// request before failing.
const int kMaxNumberOfTries = 2;

}  // namespace

ProximityAuthBleSystem::ScreenlockBridgeAdapter::ScreenlockBridgeAdapter(
    ScreenlockBridge* screenlock_bridge)
    : screenlock_bridge_(screenlock_bridge) {
}

ProximityAuthBleSystem::ScreenlockBridgeAdapter::ScreenlockBridgeAdapter() {
}

ProximityAuthBleSystem::ScreenlockBridgeAdapter::~ScreenlockBridgeAdapter() {
}

void ProximityAuthBleSystem::ScreenlockBridgeAdapter::AddObserver(
    ScreenlockBridge::Observer* observer) {
  screenlock_bridge_->AddObserver(observer);
}

void ProximityAuthBleSystem::ScreenlockBridgeAdapter::RemoveObserver(
    ScreenlockBridge::Observer* observer) {
  screenlock_bridge_->RemoveObserver(observer);
}

void ProximityAuthBleSystem::ScreenlockBridgeAdapter::Unlock(
    content::BrowserContext* browser_context) {
  screenlock_bridge_->Unlock(browser_context);
}

ProximityAuthBleSystem::ProximityAuthBleSystem(
    ScreenlockBridge* screenlock_bridge,
    content::BrowserContext* browser_context,
    scoped_ptr<CryptAuthClientFactory> cryptauth_client_factory)
    : screenlock_bridge_(new ProximityAuthBleSystem::ScreenlockBridgeAdapter(
          screenlock_bridge)),
      browser_context_(browser_context),
      cryptauth_client_factory_(cryptauth_client_factory.Pass()),
      is_polling_screen_state_(false),
      weak_ptr_factory_(this) {
  VLOG(1) << "Starting Proximity Auth over Bluetooth Low Energy.";
  screenlock_bridge_->AddObserver(this);
}

ProximityAuthBleSystem::ProximityAuthBleSystem(
    ScreenlockBridgeAdapter* screenlock_bridge,
    content::BrowserContext* browser_context)
    : screenlock_bridge_(screenlock_bridge),
      browser_context_(browser_context),
      is_polling_screen_state_(false),
      weak_ptr_factory_(this) {
  VLOG(1) << "Starting Proximity Auth over Bluetooth Low Energy.";
  screenlock_bridge_->AddObserver(this);
}

ProximityAuthBleSystem::~ProximityAuthBleSystem() {
  VLOG(1) << "Stopping Proximity over Bluetooth Low Energy.";
  screenlock_bridge_->RemoveObserver(this);
  if (connection_)
    connection_->RemoveObserver(this);
}

void ProximityAuthBleSystem::OnGetMyDevices(
    const cryptauth::GetMyDevicesResponse& response) {
  VLOG(1) << "Found " << response.devices_size() << " devices on CryptAuth.";
  unlock_keys_.clear();
  for (const auto& device : response.devices()) {
    // Cache BLE devices (|bluetooth_address().empty() == true|) that are
    // keys (|unlock_key() == 1|).
    if (device.unlock_key() && device.bluetooth_address().empty()) {
      std::string base64_public_key;
      Base64UrlEncode(device.public_key(), &base64_public_key);
      unlock_keys_[base64_public_key] = device.friendly_device_name();

      VLOG(1) << "friendly_name = " << device.friendly_device_name();
      VLOG(1) << "public_key = " << base64_public_key;
    }
  }
  VLOG(1) << "Found " << unlock_keys_.size() << " unlock keys.";
}

void ProximityAuthBleSystem::OnGetMyDevicesError(const std::string& error) {
  VLOG(1) << "GetMyDevices failed: " << error;
}

// This should be called exclusively after the user has logged in. For instance,
// calling |GetUnlockKeys| from the constructor cause |GetMyDevices| to always
// return an error.
void ProximityAuthBleSystem::GetUnlockKeys() {
  if (cryptauth_client_factory_) {
    cryptauth_client_ = cryptauth_client_factory_->CreateInstance();
    cryptauth::GetMyDevicesRequest request;
    cryptauth_client_->GetMyDevices(
        request, base::Bind(&ProximityAuthBleSystem::OnGetMyDevices,
                            weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&ProximityAuthBleSystem::OnGetMyDevicesError,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void ProximityAuthBleSystem::OnScreenDidLock(
    ScreenlockBridge::LockHandler::ScreenType screen_type) {
  VLOG(1) << "OnScreenDidLock: " << screen_type;
  switch (screen_type) {
    case ScreenlockBridge::LockHandler::SIGNIN_SCREEN:
      connection_finder_.reset();
      break;
    case ScreenlockBridge::LockHandler::LOCK_SCREEN:
      DCHECK(!connection_finder_);
      connection_finder_.reset(CreateConnectionFinder());
      connection_finder_->Find(
          base::Bind(&ProximityAuthBleSystem::OnConnectionFound,
                     weak_ptr_factory_.GetWeakPtr()));
      break;
    case ScreenlockBridge::LockHandler::OTHER_SCREEN:
      connection_finder_.reset();
      break;
  }
}

ConnectionFinder* ProximityAuthBleSystem::CreateConnectionFinder() {
  return new BluetoothLowEnergyConnectionFinder(
      kSmartLockServiceUUID, kToPeripheralCharUUID, kFromPeripheralCharUUID,
      kMaxNumberOfTries);
}

void ProximityAuthBleSystem::OnScreenDidUnlock(
    ScreenlockBridge::LockHandler::ScreenType screen_type) {
  VLOG(1) << "OnScreenDidUnlock: " << screen_type;

  // Fetch the unlock keys when the user signs in.
  // TODO(sacomoto): refetch the keys periodically, in case a new device was
  // added.
  if (screen_type == ScreenlockBridge::LockHandler::SIGNIN_SCREEN)
    GetUnlockKeys();

  if (connection_) {
    // Note: it's important to remove the observer before calling
    // |Disconnect()|, otherwise |OnConnectedStatusChanged()| will be called
    // from |connection_| and a new instance for |connection_finder_| will be
    // created.
    connection_->RemoveObserver(this);
    connection_->Disconnect();
  }

  connection_.reset();
  connection_finder_.reset();
}

void ProximityAuthBleSystem::OnFocusedUserChanged(const std::string& user_id) {
  VLOG(1) << "OnFocusedUserChanged: " << user_id;
}

void ProximityAuthBleSystem::OnMessageReceived(const Connection& connection,
                                               const WireMessage& message) {
  VLOG(1) << "Message received: " << message.payload();

  // Unlock the screen when the remote device sends an unlock signal.
  //
  // Note that this magically unlocks Chrome (no user interaction is needed).
  // This user experience for this operation will be greately improved once
  // the Proximity Auth Unlock Manager migration to C++ is done.
  if (message.payload() == kScreenUnlocked) {
    VLOG(1) << "Device unlocked. Unlock.";
    screenlock_bridge_->Unlock(browser_context_);
  }
}

void ProximityAuthBleSystem::OnConnectionFound(
    scoped_ptr<Connection> connection) {
  VLOG(1) << "Connection found.";

  connection_ = connection.Pass();
  if (connection_) {
    connection_->AddObserver(this);

    if (!is_polling_screen_state_) {
      is_polling_screen_state_ = true;
      StartPollingScreenState();
    }
  }
}

void ProximityAuthBleSystem::OnConnectionStatusChanged(
    Connection* connection,
    Connection::Status old_status,
    Connection::Status new_status) {
  if (old_status == Connection::CONNECTED &&
      new_status == Connection::DISCONNECTED) {
    StopPollingScreenState();

    connection_->RemoveObserver(this);
    connection_.reset();

    connection_finder_.reset(CreateConnectionFinder());
    connection_finder_->Find(
        base::Bind(&ProximityAuthBleSystem::OnConnectionFound,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void ProximityAuthBleSystem::StartPollingScreenState() {
  if (is_polling_screen_state_) {
    if (!connection_ || !connection_->IsConnected()) {
      VLOG(1) << "Polling stopped";
      is_polling_screen_state_ = false;
      return;
    }

    // Sends message requesting screen state.
    connection_->SendMessage(
        make_scoped_ptr(new FakeWireMessage(kPollScreenState)));

    // Schedules the next message in |kPollingIntervalSeconds| ms.
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&ProximityAuthBleSystem::StartPollingScreenState,
                              weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromSeconds(kPollingIntervalSeconds));
  }
}

void ProximityAuthBleSystem::StopPollingScreenState() {
  is_polling_screen_state_ = false;
}

}  // namespace proximity_auth
