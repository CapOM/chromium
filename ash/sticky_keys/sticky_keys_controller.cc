// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/sticky_keys/sticky_keys_controller.h"

#include "ash/sticky_keys/sticky_keys_overlay.h"
#include "base/basictypes.h"
#include "base/debug/stack_trace.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tracker.h"
#include "ui/aura/window_tree_host.h"
#include "ui/events/event.h"
#include "ui/events/event_processor.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

namespace ash {

namespace {

// Returns true if the type of mouse event should be modified by sticky keys.
bool ShouldModifyMouseEvent(const ui::MouseEvent& event) {
  ui::EventType type = event.type();
  return type == ui::ET_MOUSE_PRESSED || type == ui::ET_MOUSE_RELEASED ||
         type == ui::ET_MOUSEWHEEL;
}

// Handle the common tail of event rewriting.
ui::EventRewriteStatus RewriteUpdate(bool consumed,
                                     bool released,
                                     int mod_down_flags,
                                     int* flags) {
  int changed_down_flags = mod_down_flags & ~*flags;
  *flags |= mod_down_flags;
  if (consumed)
    return ui::EVENT_REWRITE_DISCARD;
  if (released)
    return ui::EVENT_REWRITE_DISPATCH_ANOTHER;
  if (changed_down_flags)
    return ui::EVENT_REWRITE_REWRITTEN;
  return ui::EVENT_REWRITE_CONTINUE;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//  StickyKeys
StickyKeysController::StickyKeysController()
    : enabled_(false),
      mod3_enabled_(false),
      altgr_enabled_(false) {
}

StickyKeysController::~StickyKeysController() {
}

void StickyKeysController::Enable(bool enabled) {
  if (enabled_ != enabled) {
    enabled_ = enabled;

    // Reset key handlers when activating sticky keys to ensure all
    // the handlers' states are reset.
    if (enabled_) {
      shift_sticky_key_.reset(new StickyKeysHandler(ui::EF_SHIFT_DOWN));
      alt_sticky_key_.reset(new StickyKeysHandler(ui::EF_ALT_DOWN));
      altgr_sticky_key_.reset(new StickyKeysHandler(ui::EF_ALTGR_DOWN));
      ctrl_sticky_key_.reset(new StickyKeysHandler(ui::EF_CONTROL_DOWN));
      mod3_sticky_key_.reset(new StickyKeysHandler(ui::EF_MOD3_DOWN));

      overlay_.reset(new StickyKeysOverlay());
      overlay_->SetModifierVisible(ui::EF_ALTGR_DOWN, altgr_enabled_);
      overlay_->SetModifierVisible(ui::EF_MOD3_DOWN, mod3_enabled_);
    } else if (overlay_) {
      overlay_->Show(false);
    }
  }
}

void StickyKeysController::SetModifiersEnabled(bool mod3_enabled,
                                               bool altgr_enabled) {
  mod3_enabled_ = mod3_enabled;
  altgr_enabled_ = altgr_enabled;
  if (overlay_) {
    overlay_->SetModifierVisible(ui::EF_ALTGR_DOWN, altgr_enabled_);
    overlay_->SetModifierVisible(ui::EF_MOD3_DOWN, mod3_enabled_);
  }
}

bool StickyKeysController::HandleKeyEvent(const ui::KeyEvent& event,
                                          ui::KeyboardCode key_code,
                                          int* mod_down_flags,
                                          bool* released) {
  return shift_sticky_key_->HandleKeyEvent(
             event, key_code, mod_down_flags, released) ||
         alt_sticky_key_->HandleKeyEvent(
             event, key_code, mod_down_flags, released) ||
         altgr_sticky_key_->HandleKeyEvent(
             event, key_code, mod_down_flags, released) ||
         ctrl_sticky_key_->HandleKeyEvent(
             event, key_code, mod_down_flags, released) ||
         mod3_sticky_key_->HandleKeyEvent(
             event, key_code, mod_down_flags, released);
}

bool StickyKeysController::HandleMouseEvent(const ui::MouseEvent& event,
                                            int* mod_down_flags,
                                            bool* released) {
  return shift_sticky_key_->HandleMouseEvent(
             event, mod_down_flags, released) ||
         alt_sticky_key_->HandleMouseEvent(
             event, mod_down_flags, released) ||
         altgr_sticky_key_->HandleMouseEvent(
             event, mod_down_flags, released) ||
         ctrl_sticky_key_->HandleMouseEvent(
             event, mod_down_flags, released) ||
         mod3_sticky_key_->HandleMouseEvent(
             event, mod_down_flags, released);
}

bool StickyKeysController::HandleScrollEvent(const ui::ScrollEvent& event,
                                             int* mod_down_flags,
                                             bool* released) {
  return shift_sticky_key_->HandleScrollEvent(
             event, mod_down_flags, released) ||
         alt_sticky_key_->HandleScrollEvent(
             event, mod_down_flags, released) ||
         altgr_sticky_key_->HandleScrollEvent(
             event, mod_down_flags, released) ||
         ctrl_sticky_key_->HandleScrollEvent(
             event, mod_down_flags, released) ||
         mod3_sticky_key_->HandleScrollEvent(
             event, mod_down_flags, released);
}

ui::EventRewriteStatus StickyKeysController::RewriteKeyEvent(
    const ui::KeyEvent& event,
    ui::KeyboardCode key_code,
    int* flags) {
  if (!enabled_)
    return ui::EVENT_REWRITE_CONTINUE;
  int mod_down_flags = 0;
  bool released = false;
  bool consumed = HandleKeyEvent(event, key_code, &mod_down_flags, &released);
  UpdateOverlay();
  return RewriteUpdate(consumed, released, mod_down_flags, flags);
}

ui::EventRewriteStatus StickyKeysController::RewriteMouseEvent(
    const ui::MouseEvent& event,
    int* flags) {
  if (!enabled_)
    return ui::EVENT_REWRITE_CONTINUE;
  int mod_down_flags = 0;
  bool released = false;
  bool consumed = HandleMouseEvent(event, &mod_down_flags, &released);
  UpdateOverlay();
  return RewriteUpdate(consumed, released, mod_down_flags, flags);
}

ui::EventRewriteStatus StickyKeysController::RewriteScrollEvent(
    const ui::ScrollEvent& event,
    int* flags) {
  if (!enabled_)
    return ui::EVENT_REWRITE_CONTINUE;
  int mod_down_flags = 0;
  bool released = false;
  bool consumed = HandleScrollEvent(event, &mod_down_flags, &released);
  UpdateOverlay();
  return RewriteUpdate(consumed, released, mod_down_flags, flags);
}

ui::EventRewriteStatus StickyKeysController::NextDispatchEvent(
    scoped_ptr<ui::Event>* new_event) {
  DCHECK(new_event);
  new_event->reset();
  int remaining = shift_sticky_key_->GetModifierUpEvent(new_event) +
                  alt_sticky_key_->GetModifierUpEvent(new_event) +
                  altgr_sticky_key_->GetModifierUpEvent(new_event) +
                  ctrl_sticky_key_->GetModifierUpEvent(new_event) +
                  mod3_sticky_key_->GetModifierUpEvent(new_event);
  if (!new_event)
    return ui::EVENT_REWRITE_CONTINUE;
  if (remaining)
    return ui::EVENT_REWRITE_DISPATCH_ANOTHER;
  return ui::EVENT_REWRITE_REWRITTEN;
}

void StickyKeysController::UpdateOverlay() {
  overlay_->SetModifierKeyState(
      ui::EF_SHIFT_DOWN, shift_sticky_key_->current_state());
  overlay_->SetModifierKeyState(
      ui::EF_CONTROL_DOWN, ctrl_sticky_key_->current_state());
  overlay_->SetModifierKeyState(
      ui::EF_ALT_DOWN, alt_sticky_key_->current_state());
  overlay_->SetModifierKeyState(
      ui::EF_ALTGR_DOWN, altgr_sticky_key_->current_state());
  overlay_->SetModifierKeyState(
      ui::EF_MOD3_DOWN, mod3_sticky_key_->current_state());

  bool key_in_use =
      shift_sticky_key_->current_state() != STICKY_KEY_STATE_DISABLED ||
      alt_sticky_key_->current_state() != STICKY_KEY_STATE_DISABLED ||
      altgr_sticky_key_->current_state() != STICKY_KEY_STATE_DISABLED ||
      ctrl_sticky_key_->current_state() != STICKY_KEY_STATE_DISABLED ||
      mod3_sticky_key_->current_state() != STICKY_KEY_STATE_DISABLED;

  overlay_->Show(enabled_ && key_in_use);
}

StickyKeysOverlay* StickyKeysController::GetOverlayForTest() {
  return overlay_.get();
}

///////////////////////////////////////////////////////////////////////////////
//  StickyKeysHandler
StickyKeysHandler::StickyKeysHandler(ui::EventFlags modifier_flag)
    : modifier_flag_(modifier_flag),
      current_state_(STICKY_KEY_STATE_DISABLED),
      preparing_to_enable_(false),
      scroll_delta_(0) {
}

StickyKeysHandler::~StickyKeysHandler() {
}

bool StickyKeysHandler::HandleKeyEvent(const ui::KeyEvent& event,
                                       ui::KeyboardCode key_code,
                                       int* mod_down_flags,
                                       bool* released) {
  switch (current_state_) {
    case STICKY_KEY_STATE_DISABLED:
      return HandleDisabledState(event, key_code);
    case STICKY_KEY_STATE_ENABLED:
      return HandleEnabledState(event, key_code, mod_down_flags, released);
    case STICKY_KEY_STATE_LOCKED:
      return HandleLockedState(event, key_code, mod_down_flags, released);
  }
  NOTREACHED();
  return false;
}

bool StickyKeysHandler::HandleMouseEvent(
    const ui::MouseEvent& event,
    int* mod_down_flags,
    bool* released) {
  if (ShouldModifyMouseEvent(event))
    preparing_to_enable_ = false;

  if (current_state_ == STICKY_KEY_STATE_DISABLED ||
      !ShouldModifyMouseEvent(event)) {
    return false;
  }
  DCHECK(current_state_ == STICKY_KEY_STATE_ENABLED ||
         current_state_ == STICKY_KEY_STATE_LOCKED);

  *mod_down_flags |= modifier_flag_;
  // Only disable on the mouse released event in normal, non-locked mode.
  if (current_state_ == STICKY_KEY_STATE_ENABLED &&
      event.type() != ui::ET_MOUSE_PRESSED) {
    current_state_ = STICKY_KEY_STATE_DISABLED;
    *released = true;
    return false;
  }

  return false;
}

bool StickyKeysHandler::HandleScrollEvent(
    const ui::ScrollEvent& event,
    int* mod_down_flags,
    bool* released) {
  preparing_to_enable_ = false;
  if (current_state_ == STICKY_KEY_STATE_DISABLED)
    return false;
  DCHECK(current_state_ == STICKY_KEY_STATE_ENABLED ||
         current_state_ == STICKY_KEY_STATE_LOCKED);

  // We detect a direction change if the current |scroll_delta_| is assigned
  // and the offset of the current scroll event has the opposing sign.
  bool direction_changed = false;
  if (current_state_ == STICKY_KEY_STATE_ENABLED &&
      event.type() == ui::ET_SCROLL) {
    int offset = event.y_offset();
    if (scroll_delta_)
      direction_changed = offset * scroll_delta_ <= 0;
    scroll_delta_ = offset;
  }

  if (!direction_changed)
    *mod_down_flags |= modifier_flag_;

  // We want to modify all the scroll events in the scroll sequence, which ends
  // with a fling start event. We also stop when the scroll sequence changes
  // direction.
  if (current_state_ == STICKY_KEY_STATE_ENABLED &&
      (event.type() == ui::ET_SCROLL_FLING_START || direction_changed)) {
    current_state_ = STICKY_KEY_STATE_DISABLED;
    scroll_delta_ = 0;
    *released = true;
    return false;
  }

  return false;
}

int StickyKeysHandler::GetModifierUpEvent(scoped_ptr<ui::Event>* new_event) {
  if (current_state_ != STICKY_KEY_STATE_DISABLED || !modifier_up_event_)
    return 0;
  DCHECK(new_event);
  if (*new_event)
    return 1;
  new_event->reset(modifier_up_event_.release());
  return 0;
}

StickyKeysHandler::KeyEventType StickyKeysHandler::TranslateKeyEvent(
    ui::EventType type,
    ui::KeyboardCode key_code) {
  bool is_target_key = false;
  if (key_code == ui::VKEY_SHIFT ||
      key_code == ui::VKEY_LSHIFT ||
      key_code == ui::VKEY_RSHIFT) {
    is_target_key = (modifier_flag_ == ui::EF_SHIFT_DOWN);
  } else if (key_code == ui::VKEY_CONTROL ||
      key_code == ui::VKEY_LCONTROL ||
      key_code == ui::VKEY_RCONTROL) {
    is_target_key = (modifier_flag_ == ui::EF_CONTROL_DOWN);
  } else if (key_code == ui::VKEY_MENU ||
      key_code == ui::VKEY_LMENU ||
      key_code == ui::VKEY_RMENU) {
    is_target_key = (modifier_flag_ == ui::EF_ALT_DOWN);
  } else if (key_code == ui::VKEY_ALTGR) {
    is_target_key = (modifier_flag_ == ui::EF_ALTGR_DOWN);
  } else if (key_code == ui::VKEY_OEM_8) {
    is_target_key = (modifier_flag_ == ui::EF_MOD3_DOWN);
  } else {
    return type == ui::ET_KEY_PRESSED ?
        NORMAL_KEY_DOWN : NORMAL_KEY_UP;
  }

  if (is_target_key) {
    return type == ui::ET_KEY_PRESSED ?
        TARGET_MODIFIER_DOWN : TARGET_MODIFIER_UP;
  }
  return type == ui::ET_KEY_PRESSED ?
      OTHER_MODIFIER_DOWN : OTHER_MODIFIER_UP;
}

bool StickyKeysHandler::HandleDisabledState(const ui::KeyEvent& event,
                                            ui::KeyboardCode key_code) {
  switch (TranslateKeyEvent(event.type(), key_code)) {
    case TARGET_MODIFIER_UP:
      if (preparing_to_enable_) {
        preparing_to_enable_ = false;
        scroll_delta_ = 0;
        current_state_ = STICKY_KEY_STATE_ENABLED;
        modifier_up_event_.reset(new ui::KeyEvent(event));
        return true;
      }
      return false;
    case TARGET_MODIFIER_DOWN:
      preparing_to_enable_ = true;
      return false;
    case NORMAL_KEY_DOWN:
      preparing_to_enable_ = false;
      return false;
    case NORMAL_KEY_UP:
    case OTHER_MODIFIER_DOWN:
    case OTHER_MODIFIER_UP:
      return false;
  }
  NOTREACHED();
  return false;
}

bool StickyKeysHandler::HandleEnabledState(const ui::KeyEvent& event,
                                           ui::KeyboardCode key_code,
                                           int* mod_down_flags,
                                           bool* released) {
  switch (TranslateKeyEvent(event.type(), key_code)) {
    case NORMAL_KEY_UP:
    case TARGET_MODIFIER_DOWN:
      return false;
    case TARGET_MODIFIER_UP:
      current_state_ = STICKY_KEY_STATE_LOCKED;
      modifier_up_event_.reset();
      return true;
    case NORMAL_KEY_DOWN: {
      current_state_ = STICKY_KEY_STATE_DISABLED;
      *mod_down_flags |= modifier_flag_;
      *released = true;
      return false;
    }
    case OTHER_MODIFIER_DOWN:
    case OTHER_MODIFIER_UP:
      return false;
  }
  NOTREACHED();
  return false;
}

bool StickyKeysHandler::HandleLockedState(const ui::KeyEvent& event,
                                          ui::KeyboardCode key_code,
                                          int* mod_down_flags,
                                          bool* released) {
  switch (TranslateKeyEvent(event.type(), key_code)) {
    case TARGET_MODIFIER_DOWN:
      return true;
    case TARGET_MODIFIER_UP:
      current_state_ = STICKY_KEY_STATE_DISABLED;
      return false;
    case NORMAL_KEY_DOWN:
    case NORMAL_KEY_UP:
      *mod_down_flags |= modifier_flag_;
      return false;
    case OTHER_MODIFIER_DOWN:
    case OTHER_MODIFIER_UP:
      return false;
  }
  NOTREACHED();
  return false;
}

}  // namespace ash
