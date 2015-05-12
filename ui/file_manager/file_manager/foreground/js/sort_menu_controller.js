// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!cr.ui.MenuButton} sortButton
 * @param {!FileListModel} fileListModel
 * @constructor
 * @struct
 */
function SortMenuController(sortButton, fileListModel) {
  /** @private {!FileListModel} */
  this.fileListModel_ = fileListModel;

  /** @private {!HTMLElement} */
  this.sortByNameButton_ = queryRequiredElement(
      sortButton.menu, '#sort-menu-sort-by-name');
  /** @private {!HTMLElement} */
  this.sortBySizeButton_ = queryRequiredElement(
      sortButton.menu, '#sort-menu-sort-by-size');
  /** @private {!HTMLElement} */
  this.sortByTypeButton_ = queryRequiredElement(
      sortButton.menu, '#sort-menu-sort-by-type');
  /** @private {!HTMLElement} */
  this.sortByDateButton_ = queryRequiredElement(
      sortButton.menu, '#sort-menu-sort-by-date');

  sortButton.addEventListener('menushow', this.updateCheckmark_.bind(this));
};

/**
 * Update checkmarks for each sort options.
 * @private
 */
SortMenuController.prototype.updateCheckmark_ = function() {
  var sortField = this.fileListModel_.sortStatus.field;

  this.setCheckStatus_(this.sortByNameButton_, sortField === 'name');
  this.setCheckStatus_(this.sortBySizeButton_, sortField === 'size');
  this.setCheckStatus_(this.sortByTypeButton_, sortField === 'type');
  this.setCheckStatus_(this.sortByDateButton_,
                       sortField === 'modificationTime');
};

/**
 * Set attribute 'checked' for the menu item.
 * @param {!HTMLElement} menuItem
 * @param {boolean} checked True if the item should have 'checked' attribute.
 * @private
 */
SortMenuController.prototype.setCheckStatus_ = function(menuItem, checked) {
  if (checked)
    menuItem.setAttribute('checked', '');
  else
    menuItem.removeAttribute('checked');
};
