# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import os
import re
import tempfile
import types

from pylib import cmd_helper
from pylib import constants
from pylib.utils import device_temp_file

MD5SUM_DEVICE_LIB_PATH = '/data/local/tmp/md5sum/'
MD5SUM_DEVICE_BIN_PATH = MD5SUM_DEVICE_LIB_PATH + 'md5sum_bin'

MD5SUM_DEVICE_SCRIPT_FORMAT = (
    'test -f {path} -o -d {path} '
    '&& LD_LIBRARY_PATH={md5sum_lib} {device_pie_wrapper} {md5sum_bin} {path}')

_STARTS_WITH_CHECKSUM_RE = re.compile(r'^\s*[0-9a-fA-f]{32}\s+')


def CalculateHostMd5Sums(paths):
  """Calculates the MD5 sum value for all items in |paths|.

  Args:
    paths: A list of host paths to md5sum.
  Returns:
    A dict mapping paths to their respective md5sum checksums.
  """
  if isinstance(paths, basestring):
    paths = [paths]

  out = cmd_helper.GetCmdOutput(
      [os.path.join(constants.GetOutDirectory(), 'md5sum_bin_host')] +
          [p for p in paths])

  return _ParseMd5SumOutput(out.splitlines())


def CalculateDeviceMd5Sums(paths, device):
  """Calculates the MD5 sum value for all items in |paths|.

  Args:
    paths: A list of device paths to md5sum.
  Returns:
    A dict mapping paths to their respective md5sum checksums.
  """
  if isinstance(paths, basestring):
    paths = [paths]

  if not device.FileExists(MD5SUM_DEVICE_BIN_PATH):
    device.adb.Push(
        os.path.join(constants.GetOutDirectory(), 'md5sum_dist'),
        MD5SUM_DEVICE_LIB_PATH)

  out = []

  with tempfile.NamedTemporaryFile() as md5sum_script_file:
    with device_temp_file.DeviceTempFile(
        device.adb) as md5sum_device_script_file:
      device_pie_wrapper = device.GetDevicePieWrapper()
      md5sum_script = (
          MD5SUM_DEVICE_SCRIPT_FORMAT.format(
              path=p, md5sum_lib=MD5SUM_DEVICE_LIB_PATH,
              device_pie_wrapper=device_pie_wrapper,
              md5sum_bin=MD5SUM_DEVICE_BIN_PATH)
          for p in paths)
      md5sum_script_file.write('; '.join(md5sum_script))
      md5sum_script_file.flush()
      device.adb.Push(md5sum_script_file.name, md5sum_device_script_file.name)
      out = device.RunShellCommand(['sh', md5sum_device_script_file.name])

  return _ParseMd5SumOutput(out)


def _ParseMd5SumOutput(out):
  hash_and_path = (l.split(None, 1) for l in out
                   if l and _STARTS_WITH_CHECKSUM_RE.match(l))
  return dict((p, h) for h, p in hash_and_path)

