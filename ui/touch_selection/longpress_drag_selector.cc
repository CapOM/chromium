// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/touch_selection/longpress_drag_selector.h"

#include "base/auto_reset.h"
#include "ui/events/gesture_detection/motion_event.h"

namespace ui {

LongPressDragSelector::LongPressDragSelector(
    LongPressDragSelectorClient* client)
    : client_(client),
      state_(INACTIVE),
      has_longpress_drag_start_anchor_(false) {
}

LongPressDragSelector::~LongPressDragSelector() {
}

bool LongPressDragSelector::WillHandleTouchEvent(const MotionEvent& event) {
  switch (event.GetAction()) {
    case MotionEvent::ACTION_DOWN:
      touch_down_position_.SetPoint(event.GetX(), event.GetY());
      touch_down_time_ = event.GetEventTime();
      has_longpress_drag_start_anchor_ = false;
      SetState(LONGPRESS_PENDING);
      return false;

    case MotionEvent::ACTION_UP:
    case MotionEvent::ACTION_CANCEL:
      SetState(INACTIVE);
      return false;

    case MotionEvent::ACTION_MOVE:
      break;

    default:
      return false;
  }

  if (state_ != DRAG_PENDING && state_ != DRAGGING)
    return false;

  gfx::PointF position(event.GetX(), event.GetY());
  if (state_ == DRAGGING) {
    gfx::PointF drag_position = position + longpress_drag_selection_offset_;
    client_->OnDragUpdate(*this, drag_position);
    return true;
  }

  // We can't use |touch_down_position_| as the offset anchor, as
  // showing the selection UI may have shifted the motion coordinates.
  if (!has_longpress_drag_start_anchor_) {
    has_longpress_drag_start_anchor_ = true;
    longpress_drag_start_anchor_ = position;
    return true;
  }

  // Allow an additional slop affordance after the longpress occurs.
  gfx::Vector2dF delta = position - longpress_drag_start_anchor_;
  if (client_->IsWithinTapSlop(delta))
    return true;

  gfx::PointF selection_start = client_->GetSelectionStart();
  gfx::PointF selection_end = client_->GetSelectionEnd();
  bool extend_selection_start = false;
  if (std::abs(delta.y()) > std::abs(delta.x())) {
    // If initial motion is up/down, extend the start/end selection bound.
    extend_selection_start = delta.y() < 0;
  } else {
    // Otherwise extend the selection bound toward which we're moving.
    // Note that, for mixed RTL text, or for multiline selections triggered
    // by longpress, this may not pick the most suitable drag target
    gfx::Vector2dF start_delta = selection_start - position;

    // The vectors must be normalized to make dot product comparison meaningful.
    if (!start_delta.IsZero())
      start_delta.Scale(1.f / start_delta.Length());
    gfx::Vector2dF end_delta = selection_end - position;
    if (!end_delta.IsZero())
      end_delta.Scale(1.f / start_delta.Length());

    // The larger the dot product the more similar the direction.
    extend_selection_start =
        gfx::DotProduct(start_delta, delta) > gfx::DotProduct(end_delta, delta);
  }

  gfx::PointF extent = extend_selection_start ? selection_start : selection_end;
  longpress_drag_selection_offset_ = extent - position;
  client_->OnDragBegin(*this, extent);
  SetState(DRAGGING);
  return true;
}

bool LongPressDragSelector::IsActive() const {
  return state_ != INACTIVE && state_ != LONGPRESS_PENDING;
}

void LongPressDragSelector::OnLongPressEvent(base::TimeTicks event_time,
                                             const gfx::PointF& position) {
  // We have no guarantees that the current gesture stream is aligned with the
  // observed touch stream. We only know that the gesture sequence is downstream
  // from the touch sequence. Using a time/distance heuristic helps ensure that
  // the observed longpress corresponds to the active touch sequence.
  if (state_ == LONGPRESS_PENDING &&
      // Ensure the down event occurs *before* the longpress event. Use a
      // small time epsilon to account for floating point time conversion.
      (touch_down_time_ < event_time + base::TimeDelta::FromMicroseconds(10)) &&
      client_->IsWithinTapSlop(touch_down_position_ - position)) {
    SetState(SELECTION_PENDING);
  }
}

void LongPressDragSelector::OnSelectionActivated() {
  if (state_ == SELECTION_PENDING)
    SetState(DRAG_PENDING);
}

void LongPressDragSelector::OnSelectionDeactivated() {
  SetState(INACTIVE);
}

void LongPressDragSelector::SetState(SelectionState state) {
  if (state_ == state)
    return;

  const bool was_dragging = state_ == DRAGGING;
  const bool was_active = IsActive();
  state_ = state;

  // TODO(jdduke): Add UMA for tracking relative longpress drag frequency.
  if (was_dragging)
    client_->OnDragEnd(*this);

  if (was_active != IsActive())
    client_->OnLongPressDragActiveStateChanged();
}

}  // namespace ui
