// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_APP_REMOTING_TEST_DRIVER_ENVIRONMENT_H_
#define REMOTING_TEST_APP_REMOTING_TEST_DRIVER_ENVIRONMENT_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "remoting/test/remote_application_details.h"
#include "remoting/test/remote_host_info_fetcher.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class MessageLoopForIO;
}

namespace remoting {
namespace test {

class AccessTokenFetcher;
class RefreshTokenStore;
struct RemoteHostInfo;

// Globally accessible to all test fixtures and cases and has its
// lifetime managed by the GTest framework.  It is responsible for managing
// access tokens and retrieving remote host connection information.
class AppRemotingTestDriverEnvironment : public testing::Environment {
 public:
  AppRemotingTestDriverEnvironment(const std::string& user_name,
                                   ServiceEnvironment service_environment);
  ~AppRemotingTestDriverEnvironment() override;

  // Returns false if a valid access token cannot be retrieved.
  bool Initialize(const std::string& auth_code);

  // Synchronously request a new access token using |refresh_token_|.
  // Returns true if a valid access token has been retrieved.
  bool RefreshAccessToken();

  // Synchronously request remote host information for |application_id|.
  // Returns true if the request was successful and |remote_host_info| is valid.
  bool GetRemoteHostInfoForApplicationId(const std::string& application_id,
                                         RemoteHostInfo* remote_host_info);

  // Retrieves connection information for all known applications and displays
  // their availability to STDOUT.
  void ShowHostAvailability();

  // Provides the RemoteApplicationDetails for the specified |application_name|.
  const RemoteApplicationDetails& GetDetailsFromAppName(
      const std::string& application_name);

  // Used to set fake/mock objects for AppRemotingTestDriverEnvironment tests.
  // The caller retains ownership of the supplied objects, and must ensure that
  // they remain valid until the AppRemotingTestDriverEnvironment instance has
  // been destroyed.
  void SetAccessTokenFetcherForTest(AccessTokenFetcher* access_token_fetcher);
  void SetRefreshTokenStoreForTest(RefreshTokenStore* refresh_token_store);
  void SetRemoteHostInfoFetcherForTest(
      RemoteHostInfoFetcher* remote_host_info_fetcher);

  // Accessors for fields used by tests.
  const std::string& access_token() const { return access_token_; }
  const std::string& user_name() const { return user_name_; }

 private:
  // testing::Environment interface.
  void TearDown() override;

  // Used to retrieve an access token.  If |auth_code| is empty, then the stored
  // refresh_token will be used instead of |auth_code|.
  // Returns true if a new, valid access token has been retrieved.
  bool RetrieveAccessToken(const std::string& auth_code);

  // Called after the access token fetcher completes.
  // The tokens will be empty on failure.
  void OnAccessTokenRetrieved(base::Closure done_closure,
                              const std::string& access_token,
                              const std::string& refresh_token);

  // Called after the remote host info fetcher completes.
  // |remote_host_info| is modified on failure.
  void OnRemoteHostInfoRetrieved(
      base::Closure done_closure,
      RemoteHostInfo* remote_host_info,
      const RemoteHostInfo& retrieved_remote_host_info);

  // Populates |application_names_| with the names of the supported remote
  // applications.
  void PopulateApplicationNames();

  // Populates |application_details_map_| with the RemoteApplicationDetails for
  // all supported remote applications.
  void PopulateApplicationDetailsMap();

  // Used for authenticating with the app remoting service API.
  std::string access_token_;

  // Used to retrieve an access token.
  std::string refresh_token_;

  // Used for authentication.
  std::string user_name_;

  // Service API to target when retrieving remote host connection information.
  ServiceEnvironment service_environment_;

  // Access token fetcher used by TestDriverEnvironment tests.
  remoting::test::AccessTokenFetcher* test_access_token_fetcher_;

  // RefreshTokenStore used by TestDriverEnvironment tests.
  remoting::test::RefreshTokenStore* test_refresh_token_store_;

  // RemoteHostInfoFetcher used by TestDriverEnvironment tests.
  remoting::test::RemoteHostInfoFetcher* test_remote_host_info_fetcher_;

  // Used for running network request tasks.
  scoped_ptr<base::MessageLoopForIO> message_loop_;

  // Contains the names of all supported remote applications.
  // Once initialized, this vector is not modified.
  std::vector<std::string> application_names_;

  // Contains RemoteApplicationDetails for all supported remote applications.
  // Once initialized, this map is not modified.
  std::map<std::string, RemoteApplicationDetails> application_details_map_;

  DISALLOW_COPY_AND_ASSIGN(AppRemotingTestDriverEnvironment);
};

// Unfortunately a global var is how the GTEST framework handles sharing data
// between tests and keeping long-lived objects around.  Used to share auth
// tokens and remote host connection information across tests.
extern AppRemotingTestDriverEnvironment* AppRemotingSharedData;

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_APP_REMOTING_TEST_DRIVER_ENVIRONMENT_H_
