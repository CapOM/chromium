// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"

#include "base/prefs/pref_service.h"
#include "chrome/browser/autocomplete/autocomplete_classifier.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/autocomplete/in_memory_url_index_factory.h"
#include "chrome/browser/autocomplete/shortcuts_backend_factory.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/history/core/browser/history_service.h"
#include "content/public/browser/notification_service.h"

#if defined(ENABLE_EXTENSIONS)
#include "chrome/browser/autocomplete/keyword_extensions_delegate_impl.h"
#endif

ChromeAutocompleteProviderClient::ChromeAutocompleteProviderClient(
    Profile* profile)
    : profile_(profile),
      scheme_classifier_(profile),
      search_terms_data_(profile_) {
}

ChromeAutocompleteProviderClient::~ChromeAutocompleteProviderClient() {
}

net::URLRequestContextGetter*
ChromeAutocompleteProviderClient::GetRequestContext() {
  return profile_->GetRequestContext();
}

PrefService* ChromeAutocompleteProviderClient::GetPrefs() {
  return profile_->GetPrefs();
}

const AutocompleteSchemeClassifier&
ChromeAutocompleteProviderClient::GetSchemeClassifier() const {
  return scheme_classifier_;
}

AutocompleteClassifier*
ChromeAutocompleteProviderClient::GetAutocompleteClassifier() {
  return AutocompleteClassifierFactory::GetForProfile(profile_);
}

history::HistoryService* ChromeAutocompleteProviderClient::GetHistoryService() {
  return HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
}

scoped_refptr<history::TopSites>
ChromeAutocompleteProviderClient::GetTopSites() {
  return TopSitesFactory::GetForProfile(profile_);
}

bookmarks::BookmarkModel* ChromeAutocompleteProviderClient::GetBookmarkModel() {
  return BookmarkModelFactory::GetForProfile(profile_);
}

history::URLDatabase* ChromeAutocompleteProviderClient::GetInMemoryDatabase() {
  history::HistoryService* history_service = GetHistoryService();

  // This method is called in unit test contexts where the HistoryService isn't
  // loaded.
  return history_service ? history_service->InMemoryDatabase() : NULL;
}

InMemoryURLIndex* ChromeAutocompleteProviderClient::GetInMemoryURLIndex() {
  return InMemoryURLIndexFactory::GetForProfile(profile_);
}

TemplateURLService* ChromeAutocompleteProviderClient::GetTemplateURLService() {
  return TemplateURLServiceFactory::GetForProfile(profile_);
}

const TemplateURLService*
ChromeAutocompleteProviderClient::GetTemplateURLService() const {
  return TemplateURLServiceFactory::GetForProfile(profile_);
}

const
SearchTermsData& ChromeAutocompleteProviderClient::GetSearchTermsData() const {
  return search_terms_data_;
}

scoped_refptr<ShortcutsBackend>
ChromeAutocompleteProviderClient::GetShortcutsBackend() {
  return ShortcutsBackendFactory::GetForProfile(profile_);
}

scoped_refptr<ShortcutsBackend>
ChromeAutocompleteProviderClient::GetShortcutsBackendIfExists() {
  return ShortcutsBackendFactory::GetForProfileIfExists(profile_);
}

scoped_ptr<KeywordExtensionsDelegate>
ChromeAutocompleteProviderClient::GetKeywordExtensionsDelegate(
    KeywordProvider* keyword_provider) {
#if defined(ENABLE_EXTENSIONS)
  return make_scoped_ptr(
      new KeywordExtensionsDelegateImpl(profile_, keyword_provider));
#else
  return nullptr;
#endif
}

std::string ChromeAutocompleteProviderClient::GetAcceptLanguages() const {
  return profile_->GetPrefs()->GetString(prefs::kAcceptLanguages);
}

bool ChromeAutocompleteProviderClient::IsOffTheRecord() const {
  return profile_->IsOffTheRecord();
}

bool ChromeAutocompleteProviderClient::SearchSuggestEnabled() const {
  return profile_->GetPrefs()->GetBoolean(prefs::kSearchSuggestEnabled);
}

bool ChromeAutocompleteProviderClient::BookmarkBarIsVisible() const {
  return profile_->GetPrefs()->GetBoolean(bookmarks::prefs::kShowBookmarkBar);
}

bool ChromeAutocompleteProviderClient::TabSyncEnabledAndUnencrypted() const {
  // Check field trials and settings allow sending the URL on suggest requests.
  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile_);
  sync_driver::SyncPrefs sync_prefs(profile_->GetPrefs());
  return service && service->CanSyncStart() &&
         sync_prefs.GetPreferredDataTypes(syncer::UserTypes())
             .Has(syncer::PROXY_TABS) &&
         !service->GetEncryptedDataTypes().Has(syncer::SESSIONS);
}

void ChromeAutocompleteProviderClient::Classify(
    const base::string16& text,
    bool prefer_keyword,
    bool allow_exact_keyword_match,
    metrics::OmniboxEventProto::PageClassification page_classification,
    AutocompleteMatch* match,
    GURL* alternate_nav_url) {
  AutocompleteClassifier* classifier = GetAutocompleteClassifier();
  DCHECK(classifier);
  classifier->Classify(text, prefer_keyword, allow_exact_keyword_match,
                       page_classification, match, alternate_nav_url);
}

void ChromeAutocompleteProviderClient::DeleteMatchingURLsForKeywordFromHistory(
    history::KeywordID keyword_id,
    const base::string16& term) {
  GetHistoryService()->DeleteMatchingURLsForKeyword(keyword_id, term);
}

void ChromeAutocompleteProviderClient::PrefetchImage(const GURL& url) {
  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  DCHECK(image_service);
  image_service->Prefetch(url);
}

void ChromeAutocompleteProviderClient::OnAutocompleteControllerResultReady(
    AutocompleteController* controller) {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_AUTOCOMPLETE_CONTROLLER_RESULT_READY,
      content::Source<AutocompleteController>(controller),
      content::NotificationService::NoDetails());
}
