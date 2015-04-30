// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/mouse_cursor_event_filter.h"

#include <cmath>

#include "ash/display/cursor_window_controller.h"
#include "ash/display/display_manager.h"
#include "ash/display/mouse_warp_controller.h"
#include "ash/shell.h"

namespace ash {

MouseCursorEventFilter::MouseCursorEventFilter() : mouse_warp_enabled_(true) {
  Shell::GetInstance()->display_controller()->AddObserver(this);
}

MouseCursorEventFilter::~MouseCursorEventFilter() {
  Shell::GetInstance()->display_controller()->RemoveObserver(this);
}

void MouseCursorEventFilter::ShowSharedEdgeIndicator(aura::Window* from) {
  mouse_warp_controller_ = Shell::GetInstance()
                               ->display_manager()
                               ->CreateMouseWarpController(from)
                               .Pass();
}

void MouseCursorEventFilter::HideSharedEdgeIndicator() {
  OnDisplayConfigurationChanged();
}

void MouseCursorEventFilter::OnDisplaysInitialized() {
  OnDisplayConfigurationChanged();
}

void MouseCursorEventFilter::OnDisplayConfigurationChanged() {
  mouse_warp_controller_ = Shell::GetInstance()
                               ->display_manager()
                               ->CreateMouseWarpController(nullptr)
                               .Pass();
}

void MouseCursorEventFilter::OnMouseEvent(ui::MouseEvent* event) {
  // Don't warp due to synthesized event.
  if (event->flags() & ui::EF_IS_SYNTHESIZED)
    return;

  // Handle both MOVED and DRAGGED events here because when the mouse pointer
  // enters the other root window while dragging, the underlying window system
  // (at least X11) stops generating a ui::ET_MOUSE_MOVED event.
  if (event->type() != ui::ET_MOUSE_MOVED &&
      event->type() != ui::ET_MOUSE_DRAGGED) {
      return;
  }

  Shell::GetInstance()->display_controller()->
      cursor_window_controller()->UpdateLocation();
  mouse_warp_controller_->SetEnabled(mouse_warp_enabled_);

  if (mouse_warp_controller_->WarpMouseCursor(event))
    event->StopPropagation();
}

}  // namespace ash
