// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/session_restore_delegate.h"

#include "base/metrics/field_trial.h"
#include "chrome/browser/sessions/session_restore_stats_collector.h"
#include "chrome/browser/sessions/tab_loader.h"
#include "chrome/common/url_constants.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "content/public/browser/web_contents.h"

namespace {

bool IsInternalPage(const GURL& url) {
  // There are many chrome:// UI URLs, but only look for the ones that users
  // are likely to have open. Most of the benefit is from the NTP URL.
  const char* const kReloadableUrlPrefixes[] = {
      chrome::kChromeUIDownloadsURL,
      chrome::kChromeUIHistoryURL,
      chrome::kChromeUINewTabURL,
      chrome::kChromeUISettingsURL,
  };
  // Prefix-match against the table above. Use strncmp to avoid allocating
  // memory to convert the URL prefix constants into std::strings.
  for (size_t i = 0; i < arraysize(kReloadableUrlPrefixes); ++i) {
    if (!strncmp(url.spec().c_str(), kReloadableUrlPrefixes[i],
                 strlen(kReloadableUrlPrefixes[i])))
      return true;
  }
  return false;
}

}  // namespace

SessionRestoreDelegate::RestoredTab::RestoredTab(content::WebContents* contents,
                                                 bool is_active,
                                                 bool is_app,
                                                 bool is_pinned)
    : contents_(contents),
      is_active_(is_active),
      is_app_(is_app),
      is_internal_page_(IsInternalPage(contents->GetLastCommittedURL())),
      is_pinned_(is_pinned) {
}

bool SessionRestoreDelegate::RestoredTab::operator<(
    const RestoredTab& right) const {
  // Tab with internal web UI like NTP or Settings are good choices to
  // defer loading.
  if (is_internal_page_ != right.is_internal_page_)
    return !is_internal_page_;
  // Pinned tabs should be loaded first.
  if (is_pinned_ != right.is_pinned_)
    return is_pinned_;
  // Apps should be loaded before normal tabs.
  if (is_app_ != right.is_app_)
    return is_app_;
  // Restore using MRU. Behind an experiment for now.
  if (SessionRestore::GetSmartRestoreMode() ==
      SessionRestore::SMART_RESTORE_MODE_MRU)
    return contents_->GetLastActiveTime() >
           right.contents_->GetLastActiveTime();
  return false;
}

// static
void SessionRestoreDelegate::RestoreTabs(
    const std::vector<RestoredTab>& tabs,
    const base::TimeTicks& restore_started) {
  // This experiment allows us to have comparative numbers for session restore
  // metrics. It will be removed once those numbers are obtained.
  // TODO(georgesak): Remove this experiment when stats are collected.
  base::FieldTrial* trial =
      base::FieldTrialList::Find("IntelligentSessionRestore");
  if (!trial || trial->group_name() != "DontRestoreBackgroundTabs") {
    SessionRestoreStatsCollector::TrackTabs(tabs, restore_started);
    TabLoader::RestoreTabs(tabs, restore_started);
  } else {
    SessionRestoreStatsCollector::TrackActiveTabs(tabs, restore_started);
    for (auto& restored_tab : tabs) {
      if (!restored_tab.is_active()) {
        favicon::ContentFaviconDriver* favicon_driver =
            favicon::ContentFaviconDriver::FromWebContents(
                restored_tab.contents());
        favicon_driver->FetchFavicon(favicon_driver->GetActiveURL());
      }
    }
  }
}
