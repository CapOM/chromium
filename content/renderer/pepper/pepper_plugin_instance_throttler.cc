// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_plugin_instance_throttler.h"

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/metrics/sparse_histogram.h"
#include "base/time/time.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/renderer/pepper/plugin_power_saver_helper.h"
#include "content/renderer/render_thread_impl.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

namespace content {

namespace {

static const int kInfiniteRatio = 99999;

#define UMA_HISTOGRAM_ASPECT_RATIO(name, width, height) \
  UMA_HISTOGRAM_SPARSE_SLOWLY(                          \
      name, (height) ? ((width)*100) / (height) : kInfiniteRatio);

// Histogram tracking prevalence of tiny Flash instances. Units in pixels.
enum PluginFlashTinyContentSize {
  TINY_CONTENT_SIZE_1_1 = 0,
  TINY_CONTENT_SIZE_5_5 = 1,
  TINY_CONTENT_SIZE_10_10 = 2,
  TINY_CONTENT_SIZE_LARGE = 3,
  TINY_CONTENT_SIZE_NUM_ITEMS
};

// How the throttled power saver is unthrottled, if ever.
// These numeric values are used in UMA logs; do not change them.
enum PowerSaverUnthrottleMethod {
  UNTHROTTLE_METHOD_NEVER = 0,
  UNTHROTTLE_METHOD_BY_CLICK = 1,
  UNTHROTTLE_METHOD_BY_WHITELIST = 2,
  UNTHROTTLE_METHOD_NUM_ITEMS
};

const char kFlashClickSizeAspectRatioHistogram[] =
    "Plugin.Flash.ClickSize.AspectRatio";
const char kFlashClickSizeHeightHistogram[] = "Plugin.Flash.ClickSize.Height";
const char kFlashClickSizeWidthHistogram[] = "Plugin.Flash.ClickSize.Width";
const char kFlashTinyContentSizeHistogram[] = "Plugin.Flash.TinyContentSize";
const char kPowerSaverUnthrottleHistogram[] = "Plugin.PowerSaver.Unthrottle";

// Record size metrics for all Flash instances.
void RecordFlashSizeMetric(int width, int height) {
  PluginFlashTinyContentSize size = TINY_CONTENT_SIZE_LARGE;

  if (width <= 1 && height <= 1)
    size = TINY_CONTENT_SIZE_1_1;
  else if (width <= 5 && height <= 5)
    size = TINY_CONTENT_SIZE_5_5;
  else if (width <= 10 && height <= 10)
    size = TINY_CONTENT_SIZE_10_10;

  UMA_HISTOGRAM_ENUMERATION(kFlashTinyContentSizeHistogram, size,
                            TINY_CONTENT_SIZE_NUM_ITEMS);
}

void RecordUnthrottleMethodMetric(PowerSaverUnthrottleMethod method) {
  UMA_HISTOGRAM_ENUMERATION(kPowerSaverUnthrottleHistogram, method,
                            UNTHROTTLE_METHOD_NUM_ITEMS);
}

// Records size metrics for Flash instances that are clicked.
void RecordFlashClickSizeMetric(int width, int height) {
  base::HistogramBase* width_histogram = base::LinearHistogram::FactoryGet(
      kFlashClickSizeWidthHistogram,
      0,    // minimum width
      500,  // maximum width
      100,  // number of buckets.
      base::HistogramBase::kUmaTargetedHistogramFlag);
  width_histogram->Add(width);

  base::HistogramBase* height_histogram = base::LinearHistogram::FactoryGet(
      kFlashClickSizeHeightHistogram,
      0,    // minimum height
      400,  // maximum height
      100,  // number of buckets.
      base::HistogramBase::kUmaTargetedHistogramFlag);
  height_histogram->Add(height);

  UMA_HISTOGRAM_ASPECT_RATIO(kFlashClickSizeAspectRatioHistogram, width,
                             height);
}

// When we give up waiting for a suitable preview frame, and simply suspend
// the plugin where it's at. In milliseconds.
const int kThrottleTimeout = 5000;

}  // namespace

PepperPluginInstanceThrottler::PepperPluginInstanceThrottler(
    PluginPowerSaverHelper* power_saver_helper,
    const blink::WebRect& bounds,
    const std::string& module_name,
    const GURL& plugin_url,
    const base::Closure& throttle_change_callback)
    : bounds_(bounds),
      throttle_change_callback_(throttle_change_callback),
      is_flash_plugin_(module_name == kFlashPluginName),
      has_been_clicked_(false),
      power_saver_enabled_(false),
      is_peripheral_content_(false),
      plugin_throttled_(false),
      weak_factory_(this) {
  GURL content_origin = plugin_url.GetOrigin();

  if (is_flash_plugin_ && RenderThread::Get()) {
    RenderThread::Get()->RecordAction(
        base::UserMetricsAction("Flash.PluginInstanceCreated"));
    RecordFlashSizeMetric(bounds.width, bounds.height);
  }

  bool cross_origin = false;
  is_peripheral_content_ =
      is_flash_plugin_ &&
      power_saver_helper->ShouldThrottleContent(content_origin, bounds.width,
                                                bounds.height, &cross_origin);

  power_saver_enabled_ = is_peripheral_content_ &&
                         base::CommandLine::ForCurrentProcess()->HasSwitch(
                             switches::kEnablePluginPowerSaver);

  if (is_peripheral_content_) {
    // To collect UMAs, register peripheral content even if we don't throttle.
    power_saver_helper->RegisterPeripheralPlugin(
        content_origin, base::Bind(&PepperPluginInstanceThrottler::
                                       DisablePowerSaverByRetroactiveWhitelist,
                                   weak_factory_.GetWeakPtr()));

    if (power_saver_enabled_) {
      base::MessageLoop::current()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&PepperPluginInstanceThrottler::SetPluginThrottled,
                     weak_factory_.GetWeakPtr(), true /* throttled */),
          base::TimeDelta::FromMilliseconds(kThrottleTimeout));
    }
  } else if (cross_origin) {
    power_saver_helper->WhitelistContentOrigin(content_origin);
  }
}

PepperPluginInstanceThrottler::~PepperPluginInstanceThrottler() {
}

bool PepperPluginInstanceThrottler::ConsumeInputEvent(
    const blink::WebInputEvent& event) {
  if (!has_been_clicked_ && is_flash_plugin_ &&
      event.type == blink::WebInputEvent::MouseDown) {
    has_been_clicked_ = true;
    RecordFlashClickSizeMetric(bounds_.width, bounds_.height);
  }

  if (event.type == blink::WebInputEvent::MouseUp && is_peripheral_content_) {
    is_peripheral_content_ = false;
    power_saver_enabled_ = false;

    RecordUnthrottleMethodMetric(UNTHROTTLE_METHOD_BY_CLICK);

    if (plugin_throttled_) {
      SetPluginThrottled(false /* throttled */);
      return true;
    }
  }

  return plugin_throttled_;
}

void PepperPluginInstanceThrottler::SetPluginThrottled(bool throttled) {
  // Do not throttle if we've already disabled power saver.
  if (!power_saver_enabled_ && throttled)
    return;

  plugin_throttled_ = throttled;
  throttle_change_callback_.Run();
}

void PepperPluginInstanceThrottler::DisablePowerSaverByRetroactiveWhitelist() {
  if (!is_peripheral_content_)
    return;

  is_peripheral_content_ = false;
  power_saver_enabled_ = false;
  SetPluginThrottled(false);

  RecordUnthrottleMethodMetric(UNTHROTTLE_METHOD_BY_WHITELIST);
}

}  // namespace content
