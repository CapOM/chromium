// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/external_component_loader.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/bookmarks/enhanced_bookmarks_features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/component_extensions_whitelist/whitelist.h"
#include "chrome/browser/search/hotword_service.h"
#include "chrome/browser/search/hotword_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_constants.h"
#include "components/signin/core/browser/signin_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/features/simple_feature.h"

#if defined(OS_CHROMEOS)
#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#endif

#if defined(ENABLE_APP_LIST) && defined(OS_CHROMEOS)
#include "chrome/browser/ui/app_list/google_now_extension.h"
#endif

namespace extensions {

ExternalComponentLoader::ExternalComponentLoader(Profile* profile)
    : profile_(profile) {
}

ExternalComponentLoader::~ExternalComponentLoader() {}

// static
bool ExternalComponentLoader::IsModifiable(const Extension* extension) {
  if (extension->location() == Manifest::EXTERNAL_COMPONENT) {
    static const char* const kEnhancedExtensions[] = {
        "D5736E4B5CF695CB93A2FB57E4FDC6E5AFAB6FE2",  // http://crbug.com/312900
        "D57DE394F36DC1C3220E7604C575D29C51A6C495",  // http://crbug.com/319444
        "3F65507A3B39259B38C8173C6FFA3D12DF64CCE9"   // http://crbug.com/371562
    };
    return SimpleFeature::IsIdInArray(
        extension->id(), kEnhancedExtensions, arraysize(kEnhancedExtensions));
  }
  return false;
}

void ExternalComponentLoader::StartLoading() {
  prefs_.reset(new base::DictionaryValue());
  AddExternalExtension(extension_misc::kInAppPaymentsSupportAppId);

  if (HotwordServiceFactory::IsHotwordAllowed(profile_))
    AddExternalExtension(extension_misc::kHotwordSharedModuleId);

  {
    std::string extension_id;
    if (IsEnhancedBookmarksEnabled(&extension_id))
      AddExternalExtension(extension_id);
  }

#if defined(OS_CHROMEOS)
  {
    base::CommandLine* const command_line =
        base::CommandLine::ForCurrentProcess();
    if (!command_line->HasSwitch(chromeos::switches::kDisableNewZIPUnpacker))
      AddExternalExtension(extension_misc::kZIPUnpackerExtensionId);
  }
#endif


#if defined(ENABLE_MEDIA_ROUTER)
  if (switches::MediaRouterEnabled())
    AddExternalExtension(extension_misc::kMediaRouterStableExtensionId);
#endif  // defined(ENABLE_MEDIA_ROUTER)

#if defined(ENABLE_APP_LIST) && defined(OS_CHROMEOS)
  std::string google_now_extension_id;
  if (GetGoogleNowExtensionId(&google_now_extension_id))
    AddExternalExtension(google_now_extension_id);
#endif

  LoadFinished();
}

void ExternalComponentLoader::AddExternalExtension(
    const std::string& extension_id) {
  if (!IsComponentExtensionWhitelisted(extension_id))
    return;

  prefs_->SetString(extension_id + ".external_update_url",
                    extension_urls::GetWebstoreUpdateUrl().spec());
}

}  // namespace extensions
