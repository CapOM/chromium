// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/services/gcm/instance_id/instance_id_profile_service.h"

#include "base/logging.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/services/gcm/gcm_profile_service.h"
#include "chrome/browser/services/gcm/gcm_profile_service_factory.h"
#include "components/gcm_driver/instance_id/instance_id_driver.h"

namespace instance_id {

// static
bool InstanceIDProfileService::IsInstanceIDEnabled(Profile* profile) {
  // Instance ID depends on GCM which has to been enabled.
  if (!gcm::GCMProfileService::IsGCMEnabled(profile))
    return false;

  return InstanceIDDriver::IsInstanceIDEnabled();
}

InstanceIDProfileService::InstanceIDProfileService(Profile* profile) {
  DCHECK(!profile->IsOffTheRecord());

  driver_.reset(new InstanceIDDriver(
      gcm::GCMProfileServiceFactory::GetForProfile(profile)->driver()));
}

InstanceIDProfileService::~InstanceIDProfileService() {
}

}  // namespace instance_id
