// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TASK_MANAGEMENT_PROVIDERS_WEB_CONTENTS_WEB_CONTENTS_TAGS_MANAGER_H_
#define CHROME_BROWSER_TASK_MANAGEMENT_PROVIDERS_WEB_CONTENTS_WEB_CONTENTS_TAGS_MANAGER_H_

#include <set>

#include "chrome/browser/task_management/providers/web_contents/web_contents_tag.h"

template<typename T>
struct DefaultSingletonTraits;

namespace task_management {

class WebContentsTaskProvider;

// Defines a manager to track the various TaskManager-specific WebContents
// UserData (task_management::WebContentsTags). This is used by the
// WebContentsTaskProvider to get all the pre-existing WebContents once
// WebContentsTaskProvider::StartUpdating() is called.
class WebContentsTagsManager {
 public:
  static WebContentsTagsManager* GetInstance();

  void AddTag(WebContentsTag* tag);
  void RemoveTag(WebContentsTag* tag);

  // This is how the WebContentsTaskProvider starts and stops observing the
  // creation of WebContents.
  // There must be no or only one given provider at any given time.
  void SetProvider(WebContentsTaskProvider* provider);
  void ClearProvider();

  const std::set<WebContentsTag*>& tracked_tags() const {
    return tracked_tags_;
  }

 private:
  friend struct DefaultSingletonTraits<WebContentsTagsManager>;

  WebContentsTagsManager();
  ~WebContentsTagsManager();

  // The provider that's currently observing the creation of WebContents.
  WebContentsTaskProvider* provider_;

  // A set of all the WebContentsTags seen so far.
  std::set<WebContentsTag*> tracked_tags_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsTagsManager);
};

}  // namespace task_management


#endif  // CHROME_BROWSER_TASK_MANAGEMENT_PROVIDERS_WEB_CONTENTS_WEB_CONTENTS_TAGS_MANAGER_H_
