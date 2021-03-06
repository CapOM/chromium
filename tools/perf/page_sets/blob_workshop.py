# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module
from telemetry.page import page_test as page_test


NUM_BLOB_MASS_CREATE_READS = 15


class BlobCreateThenRead(page_module.Page):

  def __init__(self, write_method, blob_sizes, page_set):
    super(BlobCreateThenRead, self).__init__(
      url='file://blob/blob-workshop.html',
      page_set=page_set,
      name='blob-create-read-' + write_method)
    self._blob_sizes = blob_sizes

  def RunPageInteractions(self, action_runner):
    action_runner.ExecuteJavaScript('disableUI = true;')

    for size_bytes in self._blob_sizes:
      with action_runner.CreateInteraction('Action_CreateAndReadBlob',
                                           repeatable=True):
        action_runner.ExecuteJavaScript(
            'createAndRead(' + str(size_bytes) + ');')
        action_runner.WaitForJavaScriptCondition(
            'doneReading === true || errors', 60)

    errors = action_runner.EvaluateJavaScript('errors')
    if errors:
      raise page_test.Failure('Errors on page: ' + ', '.join(self.errors))


class BlobMassCreate(page_module.Page):
  def __init__(self, write_method, blob_sizes, page_set):
    super(BlobMassCreate, self).__init__(
      url='file://blob/blob-workshop.html',
      page_set=page_set,
      name='blob-mass-create-' + write_method)
    self._blob_sizes = blob_sizes

  def RunPageInteractions(self, action_runner):
    action_runner.ExecuteJavaScript('disableUI = true;')

    # Add blobs
    for size_bytes in self._blob_sizes:
      with action_runner.CreateInteraction('Action_CreateBlob',
                                           repeatable=True):
        action_runner.ExecuteJavaScript('createBlob(' + str(size_bytes) + ');')

    # Read blobs
    for _ in range(0, NUM_BLOB_MASS_CREATE_READS):
      with action_runner.CreateInteraction('Action_ReadBlobs',
                                           repeatable=True):
        action_runner.ExecuteJavaScript('readBlobsSerially();')
        action_runner.WaitForJavaScriptCondition(
            'doneReading === true || errors', 60)

    errors = action_runner.EvaluateJavaScript('errors')
    if errors:
      raise page_test.Failure('Errors on page: ' + ', '.join(self.errors))


class BlobWorkshopPageSet(page_set_module.PageSet):
  """The BlobWorkshop page set."""

  def __init__(self):
    super(BlobWorkshopPageSet, self).__init__()
    self.AddUserStory(
        BlobMassCreate('2Bx200', [2] * 200, self))
    self.AddUserStory(
        BlobMassCreate('1KBx200', [1024] * 200, self))
    self.AddUserStory(
        BlobMassCreate('150KBx200', [150 * 1024] * 200, self))
    self.AddUserStory(
        BlobMassCreate('1MBx200', [1024 * 1024] * 200, self))
    self.AddUserStory(
        BlobMassCreate('10MBx30', [10 * 1024 * 1024] * 30, self))
    self.AddUserStory(
        BlobMassCreate('100MBx5', [100 * 1024 * 1024] * 5, self))

    self.AddUserStory(BlobCreateThenRead('2Bx200', [2] * 200, self))
    self.AddUserStory(BlobCreateThenRead('1KBx200', [1024] * 200, self))
    self.AddUserStory(
        BlobCreateThenRead('150KBx200', [150 * 1024 - 1] * 200, self))
    self.AddUserStory(BlobCreateThenRead('1MBx200', [1024 * 1024] * 200, self))
    self.AddUserStory(
        BlobCreateThenRead('10MBx30', [10 * 1024 * 1024] * 30, self))
    self.AddUserStory(
        BlobCreateThenRead('100MBx5', [100 * 1024 * 1024] * 5, self))
