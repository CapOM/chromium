// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Tests the order is sorted correctly for each of the columns.
 */
testcase.sortColumns = function() {
  var appId;

  var NAME_ASC = TestEntryInfo.getExpectedRows([
    ENTRIES.photos,
    ENTRIES.beautiful,
    ENTRIES.hello,
    ENTRIES.desktop,
    ENTRIES.world
  ]);

  var SIZE_DESC = TestEntryInfo.getExpectedRows([
    ENTRIES.photos,
    ENTRIES.world,
    ENTRIES.beautiful,
    ENTRIES.desktop,
    ENTRIES.hello
  ]);

  var TYPE_ASC = TestEntryInfo.getExpectedRows([
    ENTRIES.photos,
    ENTRIES.beautiful,
    ENTRIES.world,
    ENTRIES.hello,
    ENTRIES.desktop
  ]);

  var DATE_DESC = TestEntryInfo.getExpectedRows([
    ENTRIES.photos,
    ENTRIES.beautiful,
    ENTRIES.desktop,
    ENTRIES.world,
    ENTRIES.hello
  ]);

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Click the 'Name' column header and check the list.
    function(inAppId) {
      appId = inAppId;
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['.table-header-cell:nth-of-type(1)'],
                                    this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '.table-header-sort-image-asc').
          then(this.next);
    },
    function() {
      remoteCall.waitForFiles(appId, NAME_ASC, {orderCheck: true}).
          then(this.next);
    },
    // Click the 'Size' column header and check the list.
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['.table-header-cell:nth-of-type(2)'],
                                    this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '.table-header-sort-image-desc').
          then(this.next);
    },
    function() {
      remoteCall.waitForFiles(appId, SIZE_DESC, {orderCheck: true}).
          then(this.next);
    },
    // 'Size' should be checked in the sort menu.
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['#sort-button'],
                                    this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#sort-menu-sort-by-size[checked]').
          then(this.next);
    },
    // Click the 'Type' column header and check the list.
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['.table-header-cell:nth-of-type(4)'],
                                    this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '.table-header-sort-image-asc').
          then(this.next);
    },
    function() {
      remoteCall.waitForFiles(appId, TYPE_ASC, {orderCheck: true}).
          then(this.next);
    },
    // Click the 'Date modified' column header and check the list.
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['.table-header-cell:nth-of-type(5)'],
                                    this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '.table-header-sort-image-desc').
          then(this.next);
    },
    function() {
      remoteCall.waitForFiles(appId, DATE_DESC, {orderCheck: true}).
          then(this.next);
    },
    // 'Date modified' should be checked in the sort menu.
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['#sort-button'],
                                    this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#sort-menu-sort-by-date[checked]').
          then(this.next);
    },
    // Click 'Name' in the sort menu and check the result.
    function() {
      remoteCall.callRemoteTestUtil('fakeMouseClick',
                                    appId,
                                    ['#sort-menu-sort-by-name'],
                                    this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '.table-header-sort-image-asc').
          then(this.next);
    },
    function() {
      remoteCall.waitForFiles(appId, NAME_ASC, {orderCheck: true}).
          then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};
