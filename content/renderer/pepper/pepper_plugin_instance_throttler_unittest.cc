// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/renderer/pepper/pepper_plugin_instance_throttler.h"
#include "content/renderer/pepper/plugin_power_saver_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

using testing::_;
using testing::Return;

namespace content {

namespace {

class MockPluginPowerSaverHelper : public PluginPowerSaverHelper {
 public:
  MockPluginPowerSaverHelper() : PluginPowerSaverHelper(NULL) {
    EXPECT_CALL(*this, ShouldThrottleContent(_, _, _, _))
        .WillRepeatedly(Return(true));
  }

  MOCK_CONST_METHOD4(ShouldThrottleContent, bool(const GURL&, int, int, bool*));

  void RegisterPeripheralPlugin(
      const GURL& content_origin,
      const base::Closure& unthrottle_callback) override {
    unthrottle_callback_ = unthrottle_callback;
  }

  void WhitelistContentOrigin(const GURL& content_origin) override {
    DCHECK(!unthrottle_callback_.is_null());
    unthrottle_callback_.Run();
  }

  base::Closure& unthrottle_callback() { return unthrottle_callback_; }

 private:
  base::Closure unthrottle_callback_;
};

}  // namespace

class PepperPluginInstanceThrottlerTest : public testing::Test {
 protected:
  PepperPluginInstanceThrottlerTest() : change_callback_calls_(0) {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnablePluginPowerSaver);

    blink::WebRect rect;
    rect.width = 100;
    rect.height = 100;
    throttler_.reset(new PepperPluginInstanceThrottler(
        &power_saver_helper_, rect, kFlashPluginName,
        GURL("http://example.com"),
        base::Bind(&PepperPluginInstanceThrottlerTest::ChangeCallback,
                   base::Unretained(this))));
  }

  PepperPluginInstanceThrottler* throttler() {
    DCHECK(throttler_.get());
    return throttler_.get();
  }

  MockPluginPowerSaverHelper& power_saver_helper() {
    return power_saver_helper_;
  }

  int change_callback_calls() { return change_callback_calls_; }

  void EngageThrottle() {
    throttler_->SetPluginThrottled(true);
  }

  void SendEventAndTest(blink::WebInputEvent::Type event_type,
                        bool expect_consumed,
                        bool expect_throttled,
                        int expect_change_callback_count) {
    blink::WebMouseEvent event;
    event.type = event_type;
    EXPECT_EQ(expect_consumed, throttler()->ConsumeInputEvent(event));
    EXPECT_EQ(expect_throttled, throttler()->is_throttled());
    EXPECT_EQ(expect_change_callback_count, change_callback_calls());
  }

 private:
  void ChangeCallback() { ++change_callback_calls_; }

  scoped_ptr<PepperPluginInstanceThrottler> throttler_;
  MockPluginPowerSaverHelper power_saver_helper_;

  int change_callback_calls_;

  base::MessageLoop loop_;
};

TEST_F(PepperPluginInstanceThrottlerTest, ThrottleAndUnthrottleByClick) {
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(0, change_callback_calls());

  EngageThrottle();
  EXPECT_TRUE(throttler()->is_throttled());
  EXPECT_EQ(1, change_callback_calls());

  // MouseUp while throttled should be consumed and disengage throttling.
  SendEventAndTest(blink::WebInputEvent::Type::MouseUp, true, false, 2);
}

TEST_F(PepperPluginInstanceThrottlerTest, IgnoreThrottlingAfterMouseUp) {
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(0, change_callback_calls());

  // MouseUp before throttling engaged should not be consumed, but should
  // prevent subsequent throttling from engaging.
  SendEventAndTest(blink::WebInputEvent::Type::MouseUp, false, false, 0);

  EngageThrottle();
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(0, change_callback_calls());
}

TEST_F(PepperPluginInstanceThrottlerTest, FastWhitelisting) {
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(0, change_callback_calls());

  power_saver_helper().WhitelistContentOrigin(GURL("http://example.com"));

  EngageThrottle();
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(1, change_callback_calls());
}

TEST_F(PepperPluginInstanceThrottlerTest, SlowWhitelisting) {
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(0, change_callback_calls());

  EngageThrottle();
  EXPECT_TRUE(throttler()->is_throttled());
  EXPECT_EQ(1, change_callback_calls());

  power_saver_helper().WhitelistContentOrigin(GURL("http://example.com"));
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(2, change_callback_calls());
}

TEST_F(PepperPluginInstanceThrottlerTest, EventConsumption) {
  EXPECT_FALSE(throttler()->is_throttled());
  EXPECT_EQ(0, change_callback_calls());

  EngageThrottle();
  EXPECT_TRUE(throttler()->is_throttled());
  EXPECT_EQ(1, change_callback_calls());

  // Consume but don't unthrottle on a variety of other events.
  SendEventAndTest(blink::WebInputEvent::Type::MouseDown, true, true, 1);
  SendEventAndTest(blink::WebInputEvent::Type::MouseWheel, true, true, 1);
  SendEventAndTest(blink::WebInputEvent::Type::MouseMove, true, true, 1);
  SendEventAndTest(blink::WebInputEvent::Type::KeyDown, true, true, 1);
  SendEventAndTest(blink::WebInputEvent::Type::KeyUp, true, true, 1);

  // Consume and unthrottle on MouseUp
  SendEventAndTest(blink::WebInputEvent::Type::MouseUp, true, false, 2);

  // Don't consume events after unthrottle.
  SendEventAndTest(blink::WebInputEvent::Type::MouseDown, false, false, 2);
  SendEventAndTest(blink::WebInputEvent::Type::MouseWheel, false, false, 2);
  SendEventAndTest(blink::WebInputEvent::Type::MouseMove, false, false, 2);
  SendEventAndTest(blink::WebInputEvent::Type::KeyDown, false, false, 2);
  SendEventAndTest(blink::WebInputEvent::Type::KeyUp, false, false, 2);

  // Subsequent MouseUps should also not be consumed.
  SendEventAndTest(blink::WebInputEvent::Type::MouseUp, false, false, 2);
}

}  // namespace content
