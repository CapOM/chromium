// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/views/cocoa/bridged_native_widget.h"

#import <Cocoa/Cocoa.h>

#import "base/mac/mac_util.h"
#import "base/mac/sdk_forward_declarations.h"
#import "ui/base/test/nswindow_fullscreen_notification_waiter.h"
#include "ui/views/test/widget_test.h"

namespace views {

class BridgedNativeWidgetUITest : public test::WidgetTest {
 public:
  BridgedNativeWidgetUITest() {}

  // testing::Test:
  void SetUp() override {
    WidgetTest::SetUp();
    Widget::InitParams init_params =
        CreateParams(Widget::InitParams::TYPE_WINDOW);
    init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    widget_.reset(new Widget);
    widget_->Init(init_params);
  }

  void TearDown() override {
    // Ensures any compositor is removed before ViewsTestBase tears down the
    // ContextFactory.
    widget_.reset();
    WidgetTest::TearDown();
  }

  NSWindow* test_window() {
    return widget_->GetNativeWindow();
  }

 protected:
  scoped_ptr<Widget> widget_;
};

// Tests for correct fullscreen tracking, regardless of whether it is initiated
// by the Widget code or elsewhere (e.g. by the user).
TEST_F(BridgedNativeWidgetUITest, FullscreenSynchronousState) {
  EXPECT_FALSE(widget_->IsFullscreen());
  if (base::mac::IsOSSnowLeopard())
    return;

  // Allow user-initiated fullscreen changes on the Window.
  [test_window()
      setCollectionBehavior:[test_window() collectionBehavior] |
                            NSWindowCollectionBehaviorFullScreenPrimary];

  base::scoped_nsobject<NSWindowFullscreenNotificationWaiter> waiter(
      [[NSWindowFullscreenNotificationWaiter alloc]
          initWithWindow:test_window()]);
  const gfx::Rect restored_bounds = widget_->GetRestoredBounds();

  // First show the widget. A user shouldn't be able to initiate fullscreen
  // unless the window is visible in the first place.
  widget_->Show();

  // Simulate a user-initiated fullscreen. Note trying to to this again before
  // spinning a runloop will cause Cocoa to emit text to stdio and ignore it.
  [test_window() toggleFullScreen:nil];
  EXPECT_TRUE(widget_->IsFullscreen());
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());

  // Note there's now an animation running. While that's happening, toggling the
  // state should work as expected, but do "nothing".
  widget_->SetFullscreen(false);
  EXPECT_FALSE(widget_->IsFullscreen());
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());
  widget_->SetFullscreen(false);  // Same request - should no-op.
  EXPECT_FALSE(widget_->IsFullscreen());
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());

  widget_->SetFullscreen(true);
  EXPECT_TRUE(widget_->IsFullscreen());
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());

  // Always finish out of fullscreen. Otherwise there are 4 NSWindow objects
  // that Cocoa creates which don't close themselves and will be seen by the Mac
  // test harness on teardown. Note that the test harness will be waiting until
  // all animations complete, since these temporary animation windows will not
  // be removed from the window list until they do.
  widget_->SetFullscreen(false);
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());

  // Now we must wait for the notifications. Since, if the widget is torn down,
  // the NSWindowDelegate is removed, and the pending request to take out of
  // fullscreen is lost. Since a message loop has not yet spun up in this test
  // we can reliably say there will be one enter and one exit, despite all the
  // toggling above.
  [waiter waitForEnterCount:1 exitCount:1];
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());
}

// Test fullscreen without overlapping calls and without changing collection
// behavior on the test window.
TEST_F(BridgedNativeWidgetUITest, FullscreenEnterAndExit) {
  base::scoped_nsobject<NSWindowFullscreenNotificationWaiter> waiter(
      [[NSWindowFullscreenNotificationWaiter alloc]
          initWithWindow:test_window()]);

  EXPECT_FALSE(widget_->IsFullscreen());
  const gfx::Rect restored_bounds = widget_->GetRestoredBounds();
  EXPECT_FALSE(restored_bounds.IsEmpty());

  // Ensure this works without having to change collection behavior as for the
  // test above. Also check that making a hidden widget fullscreen shows it.
  EXPECT_FALSE(widget_->IsVisible());
  widget_->SetFullscreen(true);
  EXPECT_TRUE(widget_->IsVisible());
  if (base::mac::IsOSSnowLeopard()) {
    // On Snow Leopard, SetFullscreen() isn't implemented. But shouldn't crash.
    EXPECT_FALSE(widget_->IsFullscreen());
    return;
  }

  EXPECT_TRUE(widget_->IsFullscreen());
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());

  // Should be zero until the runloop spins.
  EXPECT_EQ(0, [waiter enterCount]);
  [waiter waitForEnterCount:1 exitCount:0];

  // Verify it hasn't exceeded.
  EXPECT_EQ(1, [waiter enterCount]);
  EXPECT_EQ(0, [waiter exitCount]);
  EXPECT_TRUE(widget_->IsFullscreen());
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());

  widget_->SetFullscreen(false);
  EXPECT_FALSE(widget_->IsFullscreen());
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());

  [waiter waitForEnterCount:1 exitCount:1];
  EXPECT_EQ(1, [waiter enterCount]);
  EXPECT_EQ(1, [waiter exitCount]);
  EXPECT_EQ(restored_bounds, widget_->GetRestoredBounds());
}

}  // namespace views
