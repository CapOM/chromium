# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import benchmark

from measurements import blink_style
import page_sets

@benchmark.Disabled('reference')
class BlinkStyleTop25(benchmark.Benchmark):
  """Measures performance of Blink's style engine (CSS Parsing, Style Recalc,
  etc.) on the top 25 pages.
  """
  test = blink_style.BlinkStyle
  page_set = page_sets.Top25PageSet

  @classmethod
  def Name(cls):
    return 'blink_style.top_25'


@benchmark.Disabled('reference')
@benchmark.Enabled('android')
class BlinkStyleKeyMobileSites(benchmark.Benchmark):
  """Measures performance of Blink's style engine (CSS Parsing, Style Recalc,
  etc.) on key mobile sites.
  """
  test = blink_style.BlinkStyle
  page_set = page_sets.KeyMobileSitesPageSet

  @classmethod
  def Name(cls):
    return 'blink_style.key_mobile_sites'


@benchmark.Disabled('reference')
@benchmark.Enabled('android')
class BlinkStylePolymer(benchmark.Benchmark):
  """Measures performance of Blink's style engine (CSS Parsing, Style Recalc,
  etc.) for Polymer cases.
  """
  test = blink_style.BlinkStyle
  page_set = page_sets.PolymerPageSet

  @classmethod
  def Name(cls):
    return 'blink_style.polymer'
