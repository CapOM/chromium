# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Run the first page of one benchmark for every module.

Only benchmarks that have a composable measurement are included.
Ideally this test would be comprehensive, however, running one page
of every benchmark would run impractically long.
"""

import os
import sys
import time
import unittest

from telemetry import benchmark as benchmark_module
from telemetry.core import discover
from telemetry.unittest_util import options_for_unittests
from telemetry.unittest_util import progress_reporter

from benchmarks import dom_perf
from benchmarks import indexeddb_perf
from benchmarks import image_decoding
from benchmarks import octane
from benchmarks import kraken
from benchmarks import sunspider
from benchmarks import rasterize_and_record_micro
from benchmarks import repaint
from benchmarks import spaceport
from benchmarks import speedometer
from benchmarks import jetstream


def SmokeTestGenerator(benchmark):
  # NOTE TO SHERIFFS: DO NOT DISABLE THIS TEST.
  #
  # This smoke test dynamically tests all benchmarks. So disabling it for one
  # failing or flaky benchmark would disable a much wider swath of coverage
  # than is usally intended. Instead, if a particular benchmark is failing,
  # disable it in tools/perf/benchmarks/*.
  @benchmark_module.Disabled('chromeos')  # crbug.com/351114
  def BenchmarkSmokeTest(self):
    # Only measure a single page so that this test cycles reasonably quickly.
    benchmark.options['pageset_repeat'] = 1
    benchmark.options['page_repeat'] = 1

    class SinglePageBenchmark(benchmark):  # pylint: disable=W0232
      def CreatePageSet(self, options):
        # pylint: disable=E1002
        ps = super(SinglePageBenchmark, self).CreatePageSet(options)
        for p in ps.pages:
          p.skip_waits = True
          ps.user_stories = [p]
          break
        return ps

    # Set the benchmark's default arguments.
    options = options_for_unittests.GetCopy()
    options.output_format = 'none'
    options.suppress_gtest_report = True
    parser = options.CreateParser()

    benchmark.AddCommandLineArgs(parser)
    benchmark_module.AddCommandLineArgs(parser)
    benchmark.SetArgumentDefaults(parser)
    options.MergeDefaultValues(parser.get_default_values())

    benchmark.ProcessCommandLineArgs(None, options)
    benchmark_module.ProcessCommandLineArgs(None, options)

    current = time.time()
    try:
      self.assertEqual(0, SinglePageBenchmark().Run(options),
                       msg='Failed: %s' % benchmark)
    finally:
      print 'Benchmark %s run takes %i seconds' % (
          benchmark.Name(), time.time() - current)

  return BenchmarkSmokeTest


# The list of benchmark modules to be excluded from our smoke tests.
_BLACK_LIST_TEST_MODULES = {
    dom_perf,   # Always fails on cq bot.
    image_decoding, # Always fails on Mac10.9 Tests builder.
    indexeddb_perf,  # Always fails on Win7 & Android Tests builder.
    octane,  # Often fails & take long time to timeout on cq bot.
    rasterize_and_record_micro,  # Always fails on cq bot.
    repaint,  # Often fails & takes long time to timeout on cq bot.
    spaceport,  # Takes 451 seconds.
    speedometer,  # Takes 101 seconds.
    jetstream,  # Take 206 seconds.
}

# Some smoke benchmark tests that run quickly on desktop platform can be very
# slow on Android. So we create a separate set of black list only for Android.
_ANDROID_BLACK_LIST_MODULES = {
    kraken,  # Takes 275 seconds on Android.
    sunspider  # Takes 163 seconds on Android.
}


def load_tests(loader, standard_tests, pattern):
  del loader, standard_tests, pattern  # unused
  suite = progress_reporter.TestSuite()

  benchmarks_dir = os.path.dirname(__file__)
  top_level_dir = os.path.dirname(benchmarks_dir)

  # Using the default of |one_class_per_module=True| means that if a module
  # has multiple benchmarks, only the first one is returned.
  all_benchmarks = discover.DiscoverClasses(
      benchmarks_dir, top_level_dir, benchmark_module.Benchmark,
      one_class_per_module=True)
  for benchmark in all_benchmarks:
    if sys.modules[benchmark.__module__] in _BLACK_LIST_TEST_MODULES:
      continue
    # TODO(tonyg): Smoke doesn't work with session_restore yet.
    if (benchmark.Name().startswith('session_restore') or
        benchmark.Name().startswith('skpicture_printer')):
      continue

    if hasattr(benchmark, 'generated_profile_archive'):
      # We'd like to test these, but don't know how yet.
      continue

    class BenchmarkSmokeTest(unittest.TestCase):
      pass

    method = SmokeTestGenerator(benchmark)

    # Make sure any decorators are propagated from the original declaration.
    # (access to protected members) pylint: disable=W0212
    # TODO(dpranke): Since we only pick the first test from every class
    # (above), if that test is disabled, we'll end up not running *any*
    # test from the class. We should probably discover all of the tests
    # in a class, and then throw the ones we don't need away instead.

    # Merge decorators.
    for attribute in ['_enabled_strings', '_disabled_strings']:
      # Do set union of attributes to eliminate duplicates.
      merged_attributes = list(set(getattr(method, attribute, []) +
                                   getattr(benchmark, attribute, [])))
      if merged_attributes:
        setattr(method, attribute, merged_attributes)

      # Handle the case where the benchmark is Enabled/Disabled everywhere.
      if (getattr(method, attribute, None) == [] or
          getattr(benchmark, attribute, None) == []):
        setattr(method, attribute, [])

    # Disable some tests on android platform only.
    if sys.modules[benchmark.__module__] in _ANDROID_BLACK_LIST_MODULES:
      method._disabled_strings.append('android')

    setattr(BenchmarkSmokeTest, benchmark.Name(), method)

    suite.addTest(BenchmarkSmokeTest(benchmark.Name()))

  return suite
