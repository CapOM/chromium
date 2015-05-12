// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_GCM_CLIENT_IMPL_H_
#define COMPONENTS_GCM_DRIVER_GCM_CLIENT_IMPL_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/stl_util.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/gcm_driver/gcm_stats_recorder_impl.h"
#include "google_apis/gcm/base/mcs_message.h"
#include "google_apis/gcm/engine/gcm_store.h"
#include "google_apis/gcm/engine/gservices_settings.h"
#include "google_apis/gcm/engine/mcs_client.h"
#include "google_apis/gcm/engine/registration_request.h"
#include "google_apis/gcm/engine/unregistration_request.h"
#include "google_apis/gcm/protocol/android_checkin.pb.h"
#include "google_apis/gcm/protocol/checkin.pb.h"
#include "net/log/net_log.h"
#include "net/url_request/url_request_context_getter.h"

class GURL;

namespace base {
class Clock;
class Time;
}  // namespace base

namespace mcs_proto {
class DataMessageStanza;
}  // namespace mcs_proto

namespace net {
class HttpNetworkSession;
}  // namespace net

namespace gcm {

class CheckinRequest;
class ConnectionFactory;
class GCMClientImplTest;

// Helper class for building GCM internals. Allows tests to inject fake versions
// as necessary.
class GCMInternalsBuilder {
 public:
  GCMInternalsBuilder();
  virtual ~GCMInternalsBuilder();

  virtual scoped_ptr<base::Clock> BuildClock();
  virtual scoped_ptr<MCSClient> BuildMCSClient(
      const std::string& version,
      base::Clock* clock,
      ConnectionFactory* connection_factory,
      GCMStore* gcm_store,
      GCMStatsRecorder* recorder);
  virtual scoped_ptr<ConnectionFactory> BuildConnectionFactory(
      const std::vector<GURL>& endpoints,
      const net::BackoffEntry::Policy& backoff_policy,
      const scoped_refptr<net::HttpNetworkSession>& gcm_network_session,
      const scoped_refptr<net::HttpNetworkSession>& http_network_session,
      net::NetLog* net_log,
      GCMStatsRecorder* recorder);
};

// Implements the GCM Client. It is used to coordinate MCS Client (communication
// with MCS) and other pieces of GCM infrastructure like Registration and
// Checkins. It also allows for registering user delegates that host
// applications that send and receive messages.
class GCMClientImpl
    : public GCMClient, public GCMStatsRecorder::Delegate,
      public ConnectionFactory::ConnectionListener {
 public:
  // State representation of the GCMClient.
  // Any change made to this enum should have corresponding change in the
  // GetStateString(...) function.
  enum State {
    // Uninitialized.
    UNINITIALIZED,
    // Initialized,
    INITIALIZED,
    // GCM store loading is in progress.
    LOADING,
    // GCM store is loaded.
    LOADED,
    // Initial device checkin is in progress.
    INITIAL_DEVICE_CHECKIN,
    // Ready to accept requests.
    READY,
  };

  explicit GCMClientImpl(scoped_ptr<GCMInternalsBuilder> internals_builder);
  ~GCMClientImpl() override;

  // GCMClient implementation.
  void Initialize(
      const ChromeBuildInfo& chrome_build_info,
      const base::FilePath& store_path,
      const scoped_refptr<base::SequencedTaskRunner>& blocking_task_runner,
      const scoped_refptr<net::URLRequestContextGetter>&
          url_request_context_getter,
      scoped_ptr<Encryptor> encryptor,
      GCMClient::Delegate* delegate) override;
  void Start(StartMode start_mode) override;
  void Stop() override;
  void Register(const std::string& app_id,
                const std::vector<std::string>& sender_ids) override;
  void Unregister(const std::string& app_id) override;
  void Send(const std::string& app_id,
            const std::string& receiver_id,
            const OutgoingMessage& message) override;
  void SetRecording(bool recording) override;
  void ClearActivityLogs() override;
  GCMStatistics GetStatistics() const override;
  void SetAccountTokens(
      const std::vector<AccountTokenInfo>& account_tokens) override;
  void UpdateAccountMapping(const AccountMapping& account_mapping) override;
  void RemoveAccountMapping(const std::string& account_id) override;
  void SetLastTokenFetchTime(const base::Time& time) override;
  void UpdateHeartbeatTimer(scoped_ptr<base::Timer> timer) override;
  void AddInstanceIDData(const std::string& app_id,
                         const std::string& instance_id_data) override;
  void RemoveInstanceIDData(const std::string& app_id) override;
  std::string GetInstanceIDData(const std::string& app_id) override;

  // GCMStatsRecorder::Delegate implemenation.
  void OnActivityRecorded() override;

  // ConnectionFactory::ConnectionListener implementation.
  void OnConnected(const GURL& current_server,
                   const net::IPEndPoint& ip_endpoint) override;
  void OnDisconnected() override;

 private:
  // The check-in info for the device.
  // TODO(fgorski): Convert to a class with explicit getters/setters.
  struct CheckinInfo {
    CheckinInfo();
    ~CheckinInfo();
    bool IsValid() const { return android_id != 0 && secret != 0; }
    void SnapshotCheckinAccounts();
    void Reset();

    // Android ID of the device as assigned by the server.
    uint64 android_id;
    // Security token of the device as assigned by the server.
    uint64 secret;
    // True if accounts were already provided through SetAccountsForCheckin(),
    // or when |last_checkin_accounts| was loaded as empty.
    bool accounts_set;
    // Map of account email addresses and OAuth2 tokens that will be sent to the
    // checkin server on a next checkin.
    std::map<std::string, std::string> account_tokens;
    // As set of accounts last checkin was completed with.
    std::set<std::string> last_checkin_accounts;
  };

  // Collection of pending registration requests. Keys are app IDs, while values
  // are pending registration requests to obtain a registration ID for
  // requesting application.
  typedef std::map<std::string, RegistrationRequest*>
      PendingRegistrationRequests;

  // Collection of pending unregistration requests. Keys are app IDs, while
  // values are pending unregistration requests to disable the registration ID
  // currently assigned to the application.
  typedef std::map<std::string, UnregistrationRequest*>
      PendingUnregistrationRequests;

  friend class GCMClientImplTest;

  // Returns text representation of the enum State.
  std::string GetStateString() const;

  // Callbacks for the MCSClient.
  // Receives messages and dispatches them to relevant user delegates.
  void OnMessageReceivedFromMCS(const gcm::MCSMessage& message);
  // Receives confirmation of sent messages or information about errors.
  void OnMessageSentToMCS(int64 user_serial_number,
                          const std::string& app_id,
                          const std::string& message_id,
                          MCSClient::MessageSendStatus status);
  // Receives information about mcs_client_ errors.
  void OnMCSError();

  // Runs after GCM Store load is done to trigger continuation of the
  // initialization.
  void OnLoadCompleted(scoped_ptr<GCMStore::LoadResult> result);
  // Starts the GCM.
  void StartGCM();
  // Initializes mcs_client_, which handles the connection to MCS.
  void InitializeMCSClient();
  // Complets the first time device checkin.
  void OnFirstTimeDeviceCheckinCompleted(const CheckinInfo& checkin_info);
  // Starts a login on mcs_client_.
  void StartMCSLogin();
  // Resets the GCM store when it is corrupted.
  void ResetStore();
  // Sets state to ready. This will initiate the MCS login and notify the
  // delegates.
  void OnReady(const std::vector<AccountMapping>& account_mappings,
               const base::Time& last_token_fetch_time);

  // Starts a first time device checkin.
  void StartCheckin();
  // Completes the device checkin request by parsing the |checkin_response|.
  // Function also cleans up the pending checkin.
  void OnCheckinCompleted(
      const checkin_proto::AndroidCheckinResponse& checkin_response);

  // Callback passed to GCMStore::SetGServicesSettings.
  void SetGServicesSettingsCallback(bool success);

  // Schedules next periodic device checkin and makes sure there is at most one
  // pending checkin at a time. This function is meant to be called after a
  // successful checkin.
  void SchedulePeriodicCheckin();
  // Gets the time until next checkin.
  base::TimeDelta GetTimeToNextCheckin() const;
  // Callback for setting last checkin information in the |gcm_store_|.
  void SetLastCheckinInfoCallback(bool success);

  // Callback for persisting device credentials in the |gcm_store_|.
  void SetDeviceCredentialsCallback(bool success);

  // Callback for persisting registration info in the |gcm_store_|.
  void UpdateRegistrationCallback(bool success);

  // Callback for all store operations that do not try to recover, if write in
  // |gcm_store_| fails.
  void DefaultStoreCallback(bool success);

  // Callback for store operation where result does not matter.
  void IgnoreWriteResultCallback(bool success);

  // Callback for resetting the GCM store.
  void ResetStoreCallback(bool success);

  // Completes the registration request.
  void OnRegisterCompleted(const std::string& app_id,
                           const std::vector<std::string>& sender_ids,
                           RegistrationRequest::Status status,
                           const std::string& registration_id);

  // Completes the unregistration request.
  void OnUnregisterCompleted(const std::string& app_id,
                             UnregistrationRequest::Status status);

  // Completes the GCM store destroy request.
  void OnGCMStoreDestroyed(bool success);

  // Handles incoming data message and dispatches it the delegate of this class.
  void HandleIncomingMessage(const gcm::MCSMessage& message);

  // Fires OnMessageReceived event on the delegate of this class, based on the
  // details in |data_message_stanza| and |message_data|.
  void HandleIncomingDataMessage(
      const mcs_proto::DataMessageStanza& data_message_stanza,
      MessageData& message_data);

  // Fires OnMessageSendError event on the delegate of this calss, based on the
  // details in |data_message_stanza| and |message_data|.
  void HandleIncomingSendError(
      const mcs_proto::DataMessageStanza& data_message_stanza,
      MessageData& message_data);

  // Is there any standalone app being registered for GCM?
  bool HasStandaloneRegisteredApp() const;

  // Builder for the GCM internals (mcs client, etc.).
  scoped_ptr<GCMInternalsBuilder> internals_builder_;

  // Recorder that logs GCM activities.
  GCMStatsRecorderImpl recorder_;

  // State of the GCM Client Implementation.
  State state_;

  GCMClient::Delegate* delegate_;

  // Flag to indicate if the GCM should be delay started until it is actually
  // used in either of the following cases:
  // 1) The GCM store contains the registration records.
  // 2) GCM functionailities are explicitly called.
  StartMode start_mode_;

  // Device checkin info (android ID and security token used by device).
  CheckinInfo device_checkin_info_;

  // Clock used for timing of retry logic. Passed in for testing. Owned by
  // GCMClientImpl.
  scoped_ptr<base::Clock> clock_;

  // Information about the chrome build.
  // TODO(fgorski): Check if it can be passed in constructor and made const.
  ChromeBuildInfo chrome_build_info_;

  // Persistent data store for keeping device credentials, messages and user to
  // serial number mappings.
  scoped_ptr<GCMStore> gcm_store_;

  // Data loaded from the GCM store.
  scoped_ptr<GCMStore::LoadResult> load_result_;

  // Tracks if the GCM store has been reset. This is used to prevent from
  // resetting and loading from the store again and again.
  bool gcm_store_reset_;

  scoped_refptr<net::HttpNetworkSession> network_session_;
  net::BoundNetLog net_log_;
  scoped_ptr<ConnectionFactory> connection_factory_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Controls receiving and sending of packets and reliable message queueing.
  scoped_ptr<MCSClient> mcs_client_;

  scoped_ptr<CheckinRequest> checkin_request_;

  // Cached registration info.
  RegistrationInfoMap registrations_;

  // Currently pending registration requests. GCMClientImpl owns the
  // RegistrationRequests.
  PendingRegistrationRequests pending_registration_requests_;
  STLValueDeleter<PendingRegistrationRequests>
      pending_registration_requests_deleter_;

  // Currently pending unregistration requests. GCMClientImpl owns the
  // UnregistrationRequests.
  PendingUnregistrationRequests pending_unregistration_requests_;
  STLValueDeleter<PendingUnregistrationRequests>
      pending_unregistration_requests_deleter_;

  // G-services settings that were provided by MCS.
  GServicesSettings gservices_settings_;

  // Time of the last successful checkin.
  base::Time last_checkin_time_;

  // Cached instance ID data, key is app id.
  std::map<std::string, std::string> instance_id_data_;

  // Factory for creating references when scheduling periodic checkin.
  base::WeakPtrFactory<GCMClientImpl> periodic_checkin_ptr_factory_;

  // Factory for creating references in callbacks.
  base::WeakPtrFactory<GCMClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GCMClientImpl);
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_GCM_CLIENT_IMPL_H_
