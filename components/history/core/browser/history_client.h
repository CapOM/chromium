// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_CLIENT_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_CLIENT_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "sql/init_status.h"

class GURL;

namespace base {
class FilePath;
}

namespace history {

class HistoryBackend;
class HistoryBackendClient;
class HistoryDatabase;
class ThumbnailDatabase;

// This class abstracts operations that depend on the embedder's environment,
// e.g. Chrome.
class HistoryClient {
 public:
  HistoryClient() {}
  virtual ~HistoryClient() {}

  // Called before HistoryService is shutdown.
  virtual void Shutdown() = 0;

  // Returns true if this look like the type of URL that should be added to the
  // history.
  virtual bool CanAddURL(const GURL& url) = 0;

  // Notifies the embedder that there was a problem reading the database.
  virtual void NotifyProfileError(sql::InitStatus init_status) = 0;

  // Returns a new HistoryBackendClient instance.
  virtual scoped_ptr<HistoryBackendClient> CreateBackendClient() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(HistoryClient);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_CLIENT_H_
