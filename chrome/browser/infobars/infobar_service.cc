// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/infobars/infobar_service.h"

#include "base/command_line.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/infobars/insecure_content_infobar_delegate.h"
#include "chrome/common/render_messages.h"
#include "components/content_settings/content/common/content_settings_messages.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"


DEFINE_WEB_CONTENTS_USER_DATA_KEY(InfoBarService);

// static
infobars::InfoBarDelegate::NavigationDetails
    InfoBarService::NavigationDetailsFromLoadCommittedDetails(
        const content::LoadCommittedDetails& details) {
  infobars::InfoBarDelegate::NavigationDetails navigation_details;
  navigation_details.entry_id = details.entry->GetUniqueID();
  navigation_details.is_navigation_to_different_page =
      details.is_navigation_to_different_page();
  navigation_details.did_replace_entry = details.did_replace_entry;
  navigation_details.is_redirect =
      ui::PageTransitionIsRedirect(details.entry->GetTransitionType());
  return navigation_details;
}

// static
content::WebContents* InfoBarService::WebContentsFromInfoBar(
    infobars::InfoBar* infobar) {
  if (!infobar || !infobar->owner())
    return NULL;
  InfoBarService* infobar_service =
      static_cast<InfoBarService*>(infobar->owner());
  return infobar_service->web_contents();
}

InfoBarService::InfoBarService(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      ignore_next_reload_(false) {
  DCHECK(web_contents);
}

InfoBarService::~InfoBarService() {
  ShutDown();
}

int InfoBarService::GetActiveEntryID() {
  content::NavigationEntry* active_entry =
      web_contents()->GetController().GetActiveEntry();
  return active_entry ? active_entry->GetUniqueID() : 0;
}

void InfoBarService::NotifyInfoBarAdded(infobars::InfoBar* infobar) {
  infobars::InfoBarManager::NotifyInfoBarAdded(infobar);
  // TODO(droger): Remove the notifications and have listeners change to be
  // InfoBarManager::Observers instead. See http://crbug.com/354380
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_ADDED,
      content::Source<InfoBarService>(this),
      content::Details<infobars::InfoBar::AddedDetails>(infobar));
}

void InfoBarService::NotifyInfoBarRemoved(infobars::InfoBar* infobar,
                                          bool animate) {
  infobars::InfoBarManager::NotifyInfoBarRemoved(infobar, animate);
  // TODO(droger): Remove the notifications and have listeners change to be
  // InfoBarManager::Observers instead. See http://crbug.com/354380
  infobars::InfoBar::RemovedDetails removed_details(infobar, animate);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_REMOVED,
      content::Source<InfoBarService>(this),
      content::Details<infobars::InfoBar::RemovedDetails>(&removed_details));
}

// InfoBarService::CreateConfirmInfoBar() is implemented in platform-specific
// files.

void InfoBarService::RenderProcessGone(base::TerminationStatus status) {
  RemoveAllInfoBars(true);
}

void InfoBarService::DidStartNavigationToPendingEntry(
    const GURL& url,
    content::NavigationController::ReloadType reload_type) {
  ignore_next_reload_ = false;
}

void InfoBarService::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  const bool ignore = ignore_next_reload_ &&
      (ui::PageTransitionStripQualifier(
          load_details.entry->GetTransitionType()) ==
          ui::PAGE_TRANSITION_RELOAD);
  ignore_next_reload_ = false;
  if (!ignore)
    OnNavigation(NavigationDetailsFromLoadCommittedDetails(load_details));
}

void InfoBarService::WebContentsDestroyed() {
  // The WebContents is going away; be aggressively paranoid and delete
  // ourselves lest other parts of the system attempt to add infobars or use
  // us otherwise during the destruction.
  web_contents()->RemoveUserData(UserDataKey());
  // That was the equivalent of "delete this". This object is now destroyed;
  // returning from this function is the only safe thing to do.
}

bool InfoBarService::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(InfoBarService, message)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_DidBlockDisplayingInsecureContent,
                        OnDidBlockDisplayingInsecureContent)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void InfoBarService::OnDidBlockDisplayingInsecureContent() {
  InsecureContentInfoBarDelegate::Create(this);
}
