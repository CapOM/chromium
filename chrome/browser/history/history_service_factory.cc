// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/history_service_factory.h"

#include "base/prefs/pref_service.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/chrome_bookmark_client.h"
#include "chrome/browser/bookmarks/chrome_bookmark_client_factory.h"
#include "chrome/browser/history/chrome_history_client.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/history/content/browser/content_visit_delegate.h"
#include "components/history/content/browser/history_database_helper.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/service_access_type.h"

// static
history::HistoryService* HistoryServiceFactory::GetForProfile(
    Profile* profile,
    ServiceAccessType sat) {
  // If saving history is disabled, only allow explicit access.
  if (profile->GetPrefs()->GetBoolean(prefs::kSavingBrowserHistoryDisabled) &&
      sat != ServiceAccessType::EXPLICIT_ACCESS)
    return NULL;

  return static_cast<history::HistoryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
history::HistoryService* HistoryServiceFactory::GetForProfileIfExists(
    Profile* profile,
    ServiceAccessType sat) {
  // If saving history is disabled, only allow explicit access.
  if (profile->GetPrefs()->GetBoolean(prefs::kSavingBrowserHistoryDisabled) &&
      sat != ServiceAccessType::EXPLICIT_ACCESS)
    return NULL;

  return static_cast<history::HistoryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

// static
history::HistoryService* HistoryServiceFactory::GetForProfileWithoutCreating(
    Profile* profile) {
  return static_cast<history::HistoryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

// static
HistoryServiceFactory* HistoryServiceFactory::GetInstance() {
  return Singleton<HistoryServiceFactory>::get();
}

// static
void HistoryServiceFactory::ShutdownForProfile(Profile* profile) {
  HistoryServiceFactory* factory = GetInstance();
  factory->BrowserContextDestroyed(profile);
}

HistoryServiceFactory::HistoryServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "HistoryService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(BookmarkModelFactory::GetInstance());
  DependsOn(ChromeBookmarkClientFactory::GetInstance());
}

HistoryServiceFactory::~HistoryServiceFactory() {
}

KeyedService* HistoryServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  scoped_ptr<history::HistoryService> history_service(
      new history::HistoryService(
          make_scoped_ptr(new ChromeHistoryClient(
              BookmarkModelFactory::GetForProfile(profile))),
          make_scoped_ptr(new history::ContentVisitDelegate(profile))));
  if (!history_service->Init(
          profile->GetPrefs()->GetString(prefs::kAcceptLanguages),
          history::HistoryDatabaseParamsForPath(profile->GetPath()))) {
    return nullptr;
  }
  ChromeBookmarkClientFactory::GetForProfile(profile)
      ->SetHistoryService(history_service.get());
  return history_service.release();
}

content::BrowserContext* HistoryServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool HistoryServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
