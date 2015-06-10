// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_system_impl.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_error_reporter.h"
#include "chrome/browser/extensions/extension_management.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system_factory.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/install_verifier.h"
#include "chrome/browser/extensions/navigation_observer.h"
#include "chrome/browser/extensions/shared_module_service.h"
#include "chrome/browser/extensions/shared_user_script_master.h"
#include "chrome/browser/extensions/state_store_notification_observer.h"
#include "chrome/browser/extensions/unpacked_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/extensions/extension_icon_source.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/features/feature_channel.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/url_data_source.h"
#include "extensions/browser/content_verifier.h"
#include "extensions/browser/content_verifier_delegate.h"
#include "extensions/browser/extension_pref_store.h"
#include "extensions/browser/extension_pref_value_map.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/management_policy.h"
#include "extensions/browser/quota_service.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/browser/state_store.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_url_handlers.h"
#include "net/base/escape.h"

#if defined(ENABLE_NOTIFICATIONS)
#include "chrome/browser/notifications/desktop_notification_service.h"
#include "chrome/browser/notifications/desktop_notification_service_factory.h"
#include "ui/message_center/notifier_settings.h"
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/chromeos/extensions/device_local_account_management_policy_provider.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/extensions/extension_assets_manager_chromeos.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/login/login_state.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#endif

using content::BrowserThread;

namespace {

const char kContentVerificationExperimentName[] =
    "ExtensionContentVerification";

}  // namespace

namespace extensions {

//
// ExtensionSystemImpl::Shared
//

ExtensionSystemImpl::Shared::Shared(Profile* profile)
    : profile_(profile) {
}

ExtensionSystemImpl::Shared::~Shared() {
}

void ExtensionSystemImpl::Shared::InitPrefs() {
  // Two state stores. The latter, which contains declarative rules, must be
  // loaded immediately so that the rules are ready before we issue network
  // requests.
  state_store_.reset(new StateStore(
      profile_,
      profile_->GetPath().AppendASCII(extensions::kStateStoreName),
      true));
  state_store_notification_observer_.reset(
      new StateStoreNotificationObserver(state_store_.get()));

  rules_store_.reset(new StateStore(
      profile_,
      profile_->GetPath().AppendASCII(extensions::kRulesStoreName),
      false));

#if defined(OS_CHROMEOS)
  const user_manager::User* user =
      user_manager::UserManager::Get()->GetActiveUser();
  policy::DeviceLocalAccount::Type device_local_account_type;
  if (user && policy::IsDeviceLocalAccountUser(user->email(),
                                               &device_local_account_type)) {
    device_local_account_management_policy_provider_.reset(
        new chromeos::DeviceLocalAccountManagementPolicyProvider(
            device_local_account_type));
  }
#endif  // defined(OS_CHROMEOS)
}

void ExtensionSystemImpl::Shared::RegisterManagementPolicyProviders() {
  management_policy_->RegisterProviders(
      ExtensionManagementFactory::GetForBrowserContext(profile_)
          ->GetProviders());

#if defined(OS_CHROMEOS)
  if (device_local_account_management_policy_provider_) {
    management_policy_->RegisterProvider(
        device_local_account_management_policy_provider_.get());
  }
#endif  // defined(OS_CHROMEOS)

  management_policy_->RegisterProvider(InstallVerifier::Get(profile_));
}

namespace {

class ContentVerifierDelegateImpl : public ContentVerifierDelegate {
 public:
  explicit ContentVerifierDelegateImpl(content::BrowserContext* context)
      : context_(context), default_mode_(GetDefaultMode()) {}

  ~ContentVerifierDelegateImpl() override {}

  Mode ShouldBeVerified(const Extension& extension) override {
#if defined(OS_CHROMEOS)
    if (ExtensionAssetsManagerChromeOS::IsSharedInstall(&extension))
      return ContentVerifierDelegate::ENFORCE_STRICT;
#endif

    if (!extension.is_extension() && !extension.is_legacy_packaged_app())
      return ContentVerifierDelegate::NONE;
    if (!Manifest::IsAutoUpdateableLocation(extension.location()))
      return ContentVerifierDelegate::NONE;

    if (!ManifestURL::UpdatesFromGallery(&extension)) {
      // It's possible that the webstore update url was overridden for testing
      // so also consider extensions with the default (production) update url
      // to be from the store as well.
      GURL default_webstore_url = extension_urls::GetDefaultWebstoreUpdateUrl();
      if (ManifestURL::GetUpdateURL(&extension) != default_webstore_url)
        return ContentVerifierDelegate::NONE;
    }

    return default_mode_;
  }

  const ContentVerifierKey& PublicKey() override {
    static ContentVerifierKey key(
        extension_misc::kWebstoreSignaturesPublicKey,
        extension_misc::kWebstoreSignaturesPublicKeySize);
    return key;
  }

  GURL GetSignatureFetchUrl(const std::string& extension_id,
                            const base::Version& version) override {
    // TODO(asargent) Factor out common code from the extension updater's
    // ManifestFetchData class that can be shared for use here.
    std::vector<std::string> parts;
    parts.push_back("uc");
    parts.push_back("installsource=signature");
    parts.push_back("id=" + extension_id);
    parts.push_back("v=" + version.GetString());
    std::string x_value =
        net::EscapeQueryParamValue(JoinString(parts, "&"), true);
    std::string query = "response=redirect&x=" + x_value;

    GURL base_url = extension_urls::GetWebstoreUpdateUrl();
    GURL::Replacements replacements;
    replacements.SetQuery(query.c_str(), url::Component(0, query.length()));
    return base_url.ReplaceComponents(replacements);
  }

  std::set<base::FilePath> GetBrowserImagePaths(
      const extensions::Extension* extension) override {
    return ExtensionsClient::Get()->GetBrowserImagePaths(extension);
  }

  void VerifyFailed(const std::string& extension_id,
                    ContentVerifyJob::FailureReason reason) override {
    ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
    const Extension* extension =
        registry->GetExtensionById(extension_id, ExtensionRegistry::ENABLED);
    if (!extension)
      return;
    ExtensionSystem* system = ExtensionSystem::Get(context_);
    Mode mode = ShouldBeVerified(*extension);
    if (mode >= ContentVerifierDelegate::ENFORCE) {
      if (!system->management_policy()->UserMayModifySettings(extension,
                                                              NULL)) {
        LogFailureForPolicyForceInstall(extension_id);
        return;
      }
      system->extension_service()->DisableExtension(
          extension_id, Extension::DISABLE_CORRUPTED);
      ExtensionPrefs::Get(context_)->IncrementCorruptedDisableCount();
      UMA_HISTOGRAM_BOOLEAN("Extensions.CorruptExtensionBecameDisabled", true);
      UMA_HISTOGRAM_ENUMERATION("Extensions.CorruptExtensionDisabledReason",
          reason, ContentVerifyJob::FAILURE_REASON_MAX);
    } else if (!ContainsKey(would_be_disabled_ids_, extension_id)) {
      UMA_HISTOGRAM_BOOLEAN("Extensions.CorruptExtensionWouldBeDisabled", true);
      would_be_disabled_ids_.insert(extension_id);
    }
  }

  static Mode GetDefaultMode() {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

    Mode experiment_value = NONE;
    const std::string group = base::FieldTrialList::FindFullName(
        kContentVerificationExperimentName);
    if (group == "EnforceStrict")
      experiment_value = ContentVerifierDelegate::ENFORCE_STRICT;
    else if (group == "Enforce")
      experiment_value = ContentVerifierDelegate::ENFORCE;
    else if (group == "Bootstrap")
      experiment_value = ContentVerifierDelegate::BOOTSTRAP;

    // The field trial value that normally comes from the server can be
    // overridden on the command line, which we don't want to allow since
    // malware can set chrome command line flags. There isn't currently a way
    // to find out what the server-provided value is in this case, so we
    // conservatively default to the strictest mode if we detect our experiment
    // name being overridden.
    if (command_line->HasSwitch(switches::kForceFieldTrials)) {
      std::string forced_trials =
          command_line->GetSwitchValueASCII(switches::kForceFieldTrials);
      if (forced_trials.find(kContentVerificationExperimentName) !=
              std::string::npos)
        experiment_value = ContentVerifierDelegate::ENFORCE_STRICT;
    }

    Mode cmdline_value = NONE;
    if (command_line->HasSwitch(switches::kExtensionContentVerification)) {
      std::string switch_value = command_line->GetSwitchValueASCII(
          switches::kExtensionContentVerification);
      if (switch_value == switches::kExtensionContentVerificationBootstrap)
        cmdline_value = ContentVerifierDelegate::BOOTSTRAP;
      else if (switch_value == switches::kExtensionContentVerificationEnforce)
        cmdline_value = ContentVerifierDelegate::ENFORCE;
      else if (switch_value ==
              switches::kExtensionContentVerificationEnforceStrict)
        cmdline_value = ContentVerifierDelegate::ENFORCE_STRICT;
      else
        // If no value was provided (or the wrong one), just default to enforce.
        cmdline_value = ContentVerifierDelegate::ENFORCE;
    }

    // We don't want to allow the command-line flags to eg disable enforcement
    // if the experiment group says it should be on, or malware may just modify
    // the command line flags. So return the more restrictive of the 2 values.
    return std::max(experiment_value, cmdline_value);
  }

 private:
  void LogFailureForPolicyForceInstall(const std::string& extension_id) {
    if (!ContainsKey(corrupt_policy_extensions_, extension_id)) {
      corrupt_policy_extensions_.insert(extension_id);
      UMA_HISTOGRAM_BOOLEAN("Extensions.CorruptPolicyExtensionWouldBeDisabled",
                            true);
    }
  }

  content::BrowserContext* context_;
  ContentVerifierDelegate::Mode default_mode_;

  // For reporting metrics in BOOTSTRAP mode, when an extension would be
  // disabled if content verification was in ENFORCE mode.
  std::set<std::string> would_be_disabled_ids_;

  // Currently enterprise policy extensions that must remain enabled will not
  // be disabled due to content verification failure, but we are considering
  // changing this (crbug.com/447040), so for now we are tracking how often
  // this happens to help inform the decision.
  std::set<std::string> corrupt_policy_extensions_;

  DISALLOW_COPY_AND_ASSIGN(ContentVerifierDelegateImpl);
};

}  // namespace

void ExtensionSystemImpl::Shared::Init(bool extensions_enabled) {
  TRACE_EVENT0("browser,startup", "ExtensionSystemImpl::Shared::Init");
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  navigation_observer_.reset(new NavigationObserver(profile_));

  bool allow_noisy_errors = !command_line->HasSwitch(switches::kNoErrorDialogs);
  ExtensionErrorReporter::Init(allow_noisy_errors);

  shared_user_script_master_.reset(new SharedUserScriptMaster(profile_));

  // ExtensionService depends on RuntimeData.
  runtime_data_.reset(new RuntimeData(ExtensionRegistry::Get(profile_)));

  bool autoupdate_enabled = !profile_->IsGuestSession() &&
                            !profile_->IsSystemProfile();
#if defined(OS_CHROMEOS)
  if (!extensions_enabled)
    autoupdate_enabled = false;
#endif  // defined(OS_CHROMEOS)
  extension_service_.reset(new ExtensionService(
      profile_, base::CommandLine::ForCurrentProcess(),
      profile_->GetPath().AppendASCII(extensions::kInstallDirectoryName),
      ExtensionPrefs::Get(profile_), Blacklist::Get(profile_),
      autoupdate_enabled, extensions_enabled, &ready_));

  // These services must be registered before the ExtensionService tries to
  // load any extensions.
  {
    InstallVerifier::Get(profile_)->Init();
    content_verifier_ = new ContentVerifier(
        profile_, new ContentVerifierDelegateImpl(profile_));
    ContentVerifierDelegate::Mode mode =
        ContentVerifierDelegateImpl::GetDefaultMode();
#if defined(OS_CHROMEOS)
    mode = std::max(mode, ContentVerifierDelegate::BOOTSTRAP);
#endif  // defined(OS_CHROMEOS)
    if (mode >= ContentVerifierDelegate::BOOTSTRAP)
      content_verifier_->Start();
    info_map()->SetContentVerifier(content_verifier_.get());

    management_policy_.reset(new ManagementPolicy);
    RegisterManagementPolicyProviders();
  }

  bool skip_session_extensions = false;
#if defined(OS_CHROMEOS)
  // Skip loading session extensions if we are not in a user session.
  skip_session_extensions = !chromeos::LoginState::Get()->IsUserLoggedIn();
  if (chrome::IsRunningInForcedAppMode()) {
    extension_service_->component_loader()->
        AddDefaultComponentExtensionsForKioskMode(skip_session_extensions);
  } else {
    extension_service_->component_loader()->AddDefaultComponentExtensions(
        skip_session_extensions);
  }
#else
  extension_service_->component_loader()->AddDefaultComponentExtensions(
      skip_session_extensions);
#endif
  if (command_line->HasSwitch(switches::kLoadComponentExtension)) {
    base::CommandLine::StringType path_list =
        command_line->GetSwitchValueNative(switches::kLoadComponentExtension);
    base::StringTokenizerT<base::CommandLine::StringType,
                           base::CommandLine::StringType::const_iterator>
        t(path_list, FILE_PATH_LITERAL(","));
    while (t.GetNext()) {
      // Load the component extension manifest synchronously.
      // Blocking the UI thread is acceptable here since
      // this flag designated for developers.
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      extension_service_->component_loader()->AddOrReplace(
          base::FilePath(t.token()));
    }
  }
  extension_service_->Init();

  // Make the chrome://extension-icon/ resource available.
  content::URLDataSource::Add(profile_, new ExtensionIconSource(profile_));

  quota_service_.reset(new QuotaService);

  if (extensions_enabled) {
    // Load any extensions specified with --load-extension.
    // TODO(yoz): Seems like this should move into ExtensionService::Init.
    // But maybe it's no longer important.
    if (command_line->HasSwitch(switches::kLoadExtension)) {
      base::CommandLine::StringType path_list =
          command_line->GetSwitchValueNative(switches::kLoadExtension);
      base::StringTokenizerT<base::CommandLine::StringType,
                             base::CommandLine::StringType::const_iterator>
          t(path_list, FILE_PATH_LITERAL(","));
      while (t.GetNext()) {
        std::string extension_id;
        UnpackedInstaller::Create(extension_service_.get())->
            LoadFromCommandLine(base::FilePath(t.token()), &extension_id);
      }
    }
  }
}

void ExtensionSystemImpl::Shared::Shutdown() {
  if (content_verifier_.get())
    content_verifier_->Shutdown();
  if (extension_service_)
    extension_service_->Shutdown();
}

StateStore* ExtensionSystemImpl::Shared::state_store() {
  return state_store_.get();
}

StateStore* ExtensionSystemImpl::Shared::rules_store() {
  return rules_store_.get();
}

ExtensionService* ExtensionSystemImpl::Shared::extension_service() {
  return extension_service_.get();
}

RuntimeData* ExtensionSystemImpl::Shared::runtime_data() {
  return runtime_data_.get();
}

ManagementPolicy* ExtensionSystemImpl::Shared::management_policy() {
  return management_policy_.get();
}

SharedUserScriptMaster*
ExtensionSystemImpl::Shared::shared_user_script_master() {
  return shared_user_script_master_.get();
}

InfoMap* ExtensionSystemImpl::Shared::info_map() {
  if (!extension_info_map_.get())
    extension_info_map_ = new InfoMap();
  return extension_info_map_.get();
}

QuotaService* ExtensionSystemImpl::Shared::quota_service() {
  return quota_service_.get();
}

ContentVerifier* ExtensionSystemImpl::Shared::content_verifier() {
  return content_verifier_.get();
}

//
// ExtensionSystemImpl
//

ExtensionSystemImpl::ExtensionSystemImpl(Profile* profile)
    : profile_(profile) {
  shared_ = ExtensionSystemSharedFactory::GetForBrowserContext(profile);

  if (!profile->IsOffTheRecord()) {
    shared_->InitPrefs();
  }
}

ExtensionSystemImpl::~ExtensionSystemImpl() {
}

void ExtensionSystemImpl::Shutdown() {
}

void ExtensionSystemImpl::InitForRegularProfile(bool extensions_enabled) {
  TRACE_EVENT0("browser,startup", "ExtensionSystemImpl::InitForRegularProfile");
  DCHECK(!profile_->IsOffTheRecord());
  if (shared_user_script_master() || extension_service())
    return;  // Already initialized.

  // The InfoMap needs to be created before the ProcessManager.
  shared_->info_map();
  shared_->Init(extensions_enabled);
}

ExtensionService* ExtensionSystemImpl::extension_service() {
  return shared_->extension_service();
}

RuntimeData* ExtensionSystemImpl::runtime_data() {
  return shared_->runtime_data();
}

ManagementPolicy* ExtensionSystemImpl::management_policy() {
  return shared_->management_policy();
}

SharedUserScriptMaster* ExtensionSystemImpl::shared_user_script_master() {
  return shared_->shared_user_script_master();
}

StateStore* ExtensionSystemImpl::state_store() {
  return shared_->state_store();
}

StateStore* ExtensionSystemImpl::rules_store() {
  return shared_->rules_store();
}

InfoMap* ExtensionSystemImpl::info_map() { return shared_->info_map(); }

const OneShotEvent& ExtensionSystemImpl::ready() const {
  return shared_->ready();
}

QuotaService* ExtensionSystemImpl::quota_service() {
  return shared_->quota_service();
}

ContentVerifier* ExtensionSystemImpl::content_verifier() {
  return shared_->content_verifier();
}

scoped_ptr<ExtensionSet> ExtensionSystemImpl::GetDependentExtensions(
    const Extension* extension) {
  return extension_service()->shared_module_service()->GetDependentExtensions(
      extension);
}

void ExtensionSystemImpl::RegisterExtensionWithRequestContexts(
    const Extension* extension) {
  base::Time install_time;
  if (extension->location() != Manifest::COMPONENT) {
    install_time = ExtensionPrefs::Get(profile_)->
        GetInstallTime(extension->id());
  }
  bool incognito_enabled = util::IsIncognitoEnabled(extension->id(), profile_);

  bool notifications_disabled = false;
#if defined(ENABLE_NOTIFICATIONS)
  message_center::NotifierId notifier_id(
      message_center::NotifierId::APPLICATION,
      extension->id());

  DesktopNotificationService* notification_service =
      DesktopNotificationServiceFactory::GetForProfile(profile_);
  notifications_disabled =
      !notification_service->IsNotifierEnabled(notifier_id);
#endif

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::AddExtension, info_map(),
                 make_scoped_refptr(extension), install_time,
                 incognito_enabled, notifications_disabled));
}

void ExtensionSystemImpl::UnregisterExtensionWithRequestContexts(
    const std::string& extension_id,
    const UnloadedExtensionInfo::Reason reason) {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&InfoMap::RemoveExtension, info_map(), extension_id, reason));
}

}  // namespace extensions
