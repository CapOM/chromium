// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_SETTINGS_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_SETTINGS_SERVICE_H_

#include <deque>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/ownership/owner_settings_service.h"
#include "components/policy/core/common/cloud/cloud_policy_validator.h"
#include "crypto/scoped_nss_types.h"
#include "policy/proto/device_management_backend.pb.h"

namespace crypto {
class RSAPrivateKey;
}

namespace ownership {
class OwnerKeyUtil;
class PublicKey;
}

namespace chromeos {

class SessionManagerOperation;

// Deals with the low-level interface to Chromium OS device settings. Device
// settings are stored in a protobuf that's protected by a cryptographic
// signature generated by a key in the device owner's possession. Key and
// settings are brokered by the session_manager daemon.
//
// The purpose of DeviceSettingsService is to keep track of the current key and
// settings blob. For reading and writing device settings, use CrosSettings
// instead, which provides a high-level interface that allows for manipulation
// of individual settings.
//
// DeviceSettingsService generates notifications for key and policy update
// events so interested parties can reload state as appropriate.
class DeviceSettingsService : public SessionManagerClient::Observer {
 public:
  // Indicates ownership status of the device.
  enum OwnershipStatus {
    // Listed in upgrade order.
    OWNERSHIP_UNKNOWN = 0,
    OWNERSHIP_NONE,
    OWNERSHIP_TAKEN
  };

  typedef base::Callback<void(OwnershipStatus)> OwnershipStatusCallback;

  // Status codes for Store().
  enum Status {
    STORE_SUCCESS,
    STORE_KEY_UNAVAILABLE,       // Owner key not yet configured.
    STORE_POLICY_ERROR,          // Failure constructing the settings blob.
    STORE_OPERATION_FAILED,      // IPC to session_manager daemon failed.
    STORE_NO_POLICY,             // No settings blob present.
    STORE_INVALID_POLICY,        // Invalid settings blob.
    STORE_VALIDATION_ERROR,      // Unrecoverable policy validation failure.
    STORE_TEMP_VALIDATION_ERROR, // Temporary policy validation failure.
  };

  // Observer interface.
  class Observer {
   public:
    virtual ~Observer();

    // Indicates device ownership status changes.
    virtual void OwnershipStatusChanged() = 0;

    // Gets call after updates to the device settings.
    virtual void DeviceSettingsUpdated() = 0;

    virtual void OnDeviceSettingsServiceShutdown() = 0;
  };

  // Manage singleton instance.
  static void Initialize();
  static bool IsInitialized();
  static void Shutdown();
  static DeviceSettingsService* Get();

  // Creates a device settings service instance. This is meant for unit tests,
  // production code uses the singleton returned by Get() above.
  DeviceSettingsService();
  ~DeviceSettingsService() override;

  // To be called on startup once threads are initialized and DBus is ready.
  void SetSessionManager(SessionManagerClient* session_manager_client,
                         scoped_refptr<ownership::OwnerKeyUtil> owner_key_util);

  // Prevents the service from making further calls to session_manager_client
  // and stops any pending operations.
  void UnsetSessionManager();

  SessionManagerClient* session_manager_client() const {
    return session_manager_client_;
  }

  // Returns the currently active device settings. Returns NULL if the device
  // settings have not been retrieved from session_manager yet.
  const enterprise_management::PolicyData* policy_data() {
    return policy_data_.get();
  }
  const enterprise_management::ChromeDeviceSettingsProto*
      device_settings() const {
    return device_settings_.get();
  }

  // Returns the currently used owner key.
  scoped_refptr<ownership::PublicKey> GetPublicKey();

  // Returns the status generated by the last operation.
  Status status() {
    return store_status_;
  }

  // Triggers an attempt to pull the public half of the owner key from disk and
  // load the device settings.
  void Load();

  // Stores a policy blob to session_manager. The result of the operation is
  // reported through |callback|. If successful, the updated device settings are
  // present in policy_data() and device_settings() when the callback runs.
  void Store(scoped_ptr<enterprise_management::PolicyFetchResponse> policy,
             const base::Closure& callback);

  // Returns the ownership status. May return OWNERSHIP_UNKNOWN if the disk
  // hasn't been checked yet.
  OwnershipStatus GetOwnershipStatus();

  // Determines the ownership status and reports the result to |callback|. This
  // is guaranteed to never return OWNERSHIP_UNKNOWN.
  void GetOwnershipStatusAsync(const OwnershipStatusCallback& callback);

  // Checks whether we have the private owner key.
  //
  // DEPRECATED (ygorshenin@, crbug.com/433840): this method should
  // not be used since private key is a profile-specific resource and
  // should be checked and used in a profile-aware manner, through
  // OwnerSettingsService.
  bool HasPrivateOwnerKey();

  // Sets the identity of the user that's interacting with the service. This is
  // relevant only for writing settings through SignAndStore().
  //
  // TODO (ygorshenin@, crbug.com/433840): get rid of the method when
  // write path for device settings will be removed from
  // DeviceSettingsProvider and all existing clients will be switched
  // to OwnerSettingsServiceChromeOS.
  void InitOwner(const std::string& username,
                 const base::WeakPtr<ownership::OwnerSettingsService>&
                     owner_settings_service);

  const std::string& GetUsername() const;

  ownership::OwnerSettingsService* GetOwnerSettingsService() const;

  // Adds an observer.
  void AddObserver(Observer* observer);
  // Removes an observer.
  void RemoveObserver(Observer* observer);

  // SessionManagerClient::Observer:
  void OwnerKeySet(bool success) override;
  void PropertyChangeComplete(bool success) override;

 private:
  friend class OwnerSettingsServiceChromeOS;

  // Enqueues a new operation. Takes ownership of |operation| and starts it
  // right away if there is no active operation currently.
  void Enqueue(const linked_ptr<SessionManagerOperation>& operation);

  // Enqueues a load operation.
  void EnqueueLoad(bool force_key_load);

  // Makes sure there's a reload operation so changes to the settings (and key,
  // in case force_key_load is set) are getting picked up.
  void EnsureReload(bool force_key_load);

  // Runs the next pending operation.
  void StartNextOperation();

  // Updates status, policy data and owner key from a finished operation.
  // Starts the next pending operation if available.
  void HandleCompletedOperation(const base::Closure& callback,
                                SessionManagerOperation* operation,
                                Status status);

  // Updates status and invokes the callback immediately.
  void HandleError(Status status, const base::Closure& callback);

  SessionManagerClient* session_manager_client_;
  scoped_refptr<ownership::OwnerKeyUtil> owner_key_util_;

  Status store_status_;

  std::vector<OwnershipStatusCallback> pending_ownership_status_callbacks_;

  std::string username_;
  scoped_refptr<ownership::PublicKey> public_key_;
  base::WeakPtr<ownership::OwnerSettingsService> owner_settings_service_;

  scoped_ptr<enterprise_management::PolicyData> policy_data_;
  scoped_ptr<enterprise_management::ChromeDeviceSettingsProto> device_settings_;

  // The queue of pending operations. The first operation on the queue is
  // currently active; it gets removed and destroyed once it completes.
  std::deque<linked_ptr<SessionManagerOperation>> pending_operations_;

  base::ObserverList<Observer> observers_;

  // For recoverable load errors how many retries are left before we give up.
  int load_retries_left_;

  base::WeakPtrFactory<DeviceSettingsService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceSettingsService);
};

// Helper class for tests. Initializes the DeviceSettingsService singleton on
// construction and tears it down again on destruction.
class ScopedTestDeviceSettingsService {
 public:
  ScopedTestDeviceSettingsService();
  ~ScopedTestDeviceSettingsService();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedTestDeviceSettingsService);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_SETTINGS_SERVICE_H_
