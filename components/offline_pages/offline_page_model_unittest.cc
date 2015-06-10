// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/offline_page_model.h"

#include "components/offline_pages/offline_page_metadata_store.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

class OfflinePageTestStore : public OfflinePageMetadataStore {
 public:
  ~OfflinePageTestStore() override;

  // OfflinePageMetadataStore overrides:
  void Load(const LoadCallback& callback) override;
  void AddOfflinePage(const OfflinePageItem& offline_page,
                      const UpdateCallback& callback) override;
  void RemoveOfflinePage(const GURL& page_url,
                         const UpdateCallback& callback) override;
};

OfflinePageTestStore::~OfflinePageTestStore() {
}

void OfflinePageTestStore::Load(const LoadCallback& callback) {
}

void OfflinePageTestStore::AddOfflinePage(const OfflinePageItem& offline_page,
                                          const UpdateCallback& callback) {
}

void OfflinePageTestStore::RemoveOfflinePage(const GURL& page_url,
                                             const UpdateCallback& callback) {
}

class OfflinePageModelTest : public testing::Test {
 public:
  OfflinePageModelTest();
  ~OfflinePageModelTest() override;

  scoped_ptr<OfflinePageMetadataStore> BuildStore();
};

OfflinePageModelTest::OfflinePageModelTest() {
}

OfflinePageModelTest::~OfflinePageModelTest() {
}

scoped_ptr<OfflinePageMetadataStore> OfflinePageModelTest::BuildStore() {
  return scoped_ptr<OfflinePageMetadataStore>(new OfflinePageTestStore());
}

TEST_F(OfflinePageModelTest, Initialize) {
  scoped_ptr<OfflinePageMetadataStore> store = BuildStore();
  OfflinePageMetadataStore* store_ptr = store.get();
  OfflinePageModel model(store.Pass());
  EXPECT_EQ(store_ptr, model.GetStoreForTesting());
}

}  // namespace

}  // namespace offline_pages
