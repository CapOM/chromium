// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/threading/non_thread_safe.h"
#include "components/invalidation/invalidation_service.h"
#include "components/invalidation/p2p_invalidator.h"
#include "components/keyed_service/core/keyed_service.h"

#ifndef COMPONENTS_INVALIDATION_P2P_INVALIDATION_SERVICE_H_
#define COMPONENTS_INVALIDATION_P2P_INVALIDATION_SERVICE_H_

namespace net {
class URLRequestContextGetter;
}

namespace syncer {
class P2PInvalidator;
}

namespace invalidation {

class InvalidationLogger;

// This service is a wrapper around P2PInvalidator.  Unlike other
// InvalidationServices, it can both send and receive invalidations.  It is used
// only in tests, where we're unable to connect to a real invalidations server.
class P2PInvalidationService : public base::NonThreadSafe,
                               public InvalidationService {
 public:
  P2PInvalidationService(
      scoped_ptr<IdentityProvider> identity_provider,
      const scoped_refptr<net::URLRequestContextGetter>& request_context,
      syncer::P2PNotificationTarget notification_target);
  ~P2PInvalidationService() override;

  // InvalidationService implementation.
  // It is an error to have registered handlers when the service is destroyed.
  void RegisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;
  bool UpdateRegisteredInvalidationIds(syncer::InvalidationHandler* handler,
                                       const syncer::ObjectIdSet& ids) override;
  void UnregisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;
  syncer::InvalidatorState GetInvalidatorState() const override;
  std::string GetInvalidatorClientId() const override;
  InvalidationLogger* GetInvalidationLogger() override;
  void RequestDetailedStatus(
      base::Callback<void(const base::DictionaryValue&)> caller) const override;
  IdentityProvider* GetIdentityProvider() override;

  void UpdateCredentials(const std::string& username,
                         const std::string& password);

  void SendInvalidation(const syncer::ObjectIdSet& ids);

 private:
  scoped_ptr<IdentityProvider> identity_provider_;
  scoped_ptr<syncer::P2PInvalidator> invalidator_;
  std::string invalidator_id_;

  DISALLOW_COPY_AND_ASSIGN(P2PInvalidationService);
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_P2P_INVALIDATION_SERVICE_H_
