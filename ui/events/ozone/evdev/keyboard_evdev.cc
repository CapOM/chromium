// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/keyboard_evdev.h"

#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/dom4/keycode_converter.h"
#include "ui/events/ozone/evdev/event_modifiers_evdev.h"
#include "ui/events/ozone/layout/keyboard_layout_engine.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/events/ozone/layout/layout_util.h"

namespace ui {

namespace {

const int kXkbKeycodeOffset = 8;

const int kRepeatDelayMs = 500;
const int kRepeatIntervalMs = 50;

int EventFlagToEvdevModifier(int flag) {
  switch (flag) {
    case EF_CAPS_LOCK_DOWN:
      return EVDEV_MODIFIER_CAPS_LOCK;
    case EF_SHIFT_DOWN:
      return EVDEV_MODIFIER_SHIFT;
    case EF_CONTROL_DOWN:
      return EVDEV_MODIFIER_CONTROL;
    case EF_ALT_DOWN:
      return EVDEV_MODIFIER_ALT;
    case EF_ALTGR_DOWN:
      return EVDEV_MODIFIER_ALTGR;
    case EF_LEFT_MOUSE_BUTTON:
      return EVDEV_MODIFIER_LEFT_MOUSE_BUTTON;
    case EF_MIDDLE_MOUSE_BUTTON:
      return EVDEV_MODIFIER_MIDDLE_MOUSE_BUTTON;
    case EF_RIGHT_MOUSE_BUTTON:
      return EVDEV_MODIFIER_RIGHT_MOUSE_BUTTON;
    case EF_COMMAND_DOWN:
      return EVDEV_MODIFIER_COMMAND;
    default:
      return EVDEV_MODIFIER_NONE;
  }
}

bool IsModifierLock(int evdev_modifier) {
  return evdev_modifier == EVDEV_MODIFIER_CAPS_LOCK;
}

}  // namespace

KeyboardEvdev::KeyboardEvdev(EventModifiersEvdev* modifiers,
                             KeyboardLayoutEngine* keyboard_layout_engine,
                             const EventDispatchCallback& callback)
    : callback_(callback),
      modifiers_(modifiers),
      keyboard_layout_engine_(keyboard_layout_engine),
      repeat_enabled_(true),
      repeat_key_(KEY_RESERVED) {
  repeat_delay_ = base::TimeDelta::FromMilliseconds(kRepeatDelayMs);
  repeat_interval_ = base::TimeDelta::FromMilliseconds(kRepeatIntervalMs);
}

KeyboardEvdev::~KeyboardEvdev() {
}

void KeyboardEvdev::OnKeyChange(unsigned int key, bool down) {
  if (key > KEY_MAX)
    return;

  if (down == key_state_.test(key))
    return;

  // State transition: !(down) -> (down)
  if (down)
    key_state_.set(key);
  else
    key_state_.reset(key);

  UpdateKeyRepeat(key, down);
  DispatchKey(key, down, false /* repeat */);
}

void KeyboardEvdev::SetCapsLockEnabled(bool enabled) {
  modifiers_->SetModifierLock(EVDEV_MODIFIER_CAPS_LOCK, enabled);
}

bool KeyboardEvdev::IsCapsLockEnabled() {
  return (modifiers_->GetModifierFlags() & EF_CAPS_LOCK_DOWN) != 0;
}

bool KeyboardEvdev::IsAutoRepeatEnabled() {
  return repeat_enabled_;
}

void KeyboardEvdev::SetAutoRepeatEnabled(bool enabled) {
  repeat_enabled_ = enabled;
}

void KeyboardEvdev::SetAutoRepeatRate(const base::TimeDelta& delay,
                                      const base::TimeDelta& interval) {
  repeat_delay_ = delay;
  repeat_interval_ = interval;
}

void KeyboardEvdev::GetAutoRepeatRate(base::TimeDelta* delay,
                                      base::TimeDelta* interval) {
  *delay = repeat_delay_;
  *interval = repeat_interval_;
}

void KeyboardEvdev::UpdateModifier(int modifier_flag, bool down) {
  if (modifier_flag == EF_NONE)
    return;

  int modifier = EventFlagToEvdevModifier(modifier_flag);
  if (modifier == EVDEV_MODIFIER_NONE)
    return;

  if (IsModifierLock(modifier))
    modifiers_->UpdateModifierLock(modifier, down);
  else
    modifiers_->UpdateModifier(modifier, down);
}

void KeyboardEvdev::UpdateKeyRepeat(unsigned int key, bool down) {
  if (!repeat_enabled_)
    StopKeyRepeat();
  else if (key != repeat_key_ && down)
    StartKeyRepeat(key);
  else if (key == repeat_key_ && !down)
    StopKeyRepeat();
}

void KeyboardEvdev::StartKeyRepeat(unsigned int key) {
  repeat_key_ = key;
  repeat_delay_timer_.Start(
      FROM_HERE, repeat_delay_,
      base::Bind(&KeyboardEvdev::OnRepeatDelayTimeout, base::Unretained(this)));
  repeat_interval_timer_.Stop();
}

void KeyboardEvdev::StopKeyRepeat() {
  repeat_key_ = KEY_RESERVED;
  repeat_delay_timer_.Stop();
  repeat_interval_timer_.Stop();
}

void KeyboardEvdev::OnRepeatDelayTimeout() {
  DispatchKey(repeat_key_, true /* down */, true /* repeat */);

  repeat_interval_timer_.Start(
      FROM_HERE, repeat_interval_,
      base::Bind(&KeyboardEvdev::OnRepeatIntervalTimeout,
                 base::Unretained(this)));
}

void KeyboardEvdev::OnRepeatIntervalTimeout() {
  DispatchKey(repeat_key_, true /* down */, true /* repeat */);
}

void KeyboardEvdev::DispatchKey(unsigned int key, bool down, bool repeat) {
  DomCode dom_code =
      KeycodeConverter::NativeKeycodeToDomCode(EvdevCodeToNativeCode(key));
  // DomCode constants are not included here because of conflicts with
  // evdev preprocessor macros.
  if (!static_cast<int>(dom_code))
    return;
  int flags = modifiers_->GetModifierFlags();
  DomKey dom_key;
  KeyboardCode key_code;
  uint16 character;
  uint32 platform_keycode = 0;
  if (!keyboard_layout_engine_->Lookup(dom_code, flags, &dom_key, &character,
                                       &key_code, &platform_keycode)) {
    return;
  }
  if (!repeat)
    UpdateModifier(ModifierDomKeyToEventFlag(dom_key), down);

  KeyEvent* event =
      new KeyEvent(down ? ET_KEY_PRESSED : ET_KEY_RELEASED, key_code, dom_code,
                   modifiers_->GetModifierFlags(), dom_key, character);
  if (platform_keycode)
    event->set_platform_keycode(platform_keycode);
  callback_.Run(make_scoped_ptr(event));
}

// static
int KeyboardEvdev::NativeCodeToEvdevCode(int native_code) {
  if (native_code == KeycodeConverter::InvalidNativeKeycode()) {
    return KEY_RESERVED;
  }
  return native_code - kXkbKeycodeOffset;
}

// static
int KeyboardEvdev::EvdevCodeToNativeCode(int evdev_code) {
  if (evdev_code == KEY_RESERVED)
    return KeycodeConverter::InvalidNativeKeycode();
  return evdev_code + kXkbKeycodeOffset;
}

}  // namespace ui
