// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/common/stub_overlay_manager.h"

namespace ui {

StubOverlayManager::StubOverlayManager() {
}

StubOverlayManager::~StubOverlayManager() {
}

OverlayCandidatesOzone* StubOverlayManager::GetOverlayCandidates(
    gfx::AcceleratedWidget w) {
  return nullptr;
}

bool StubOverlayManager::CanShowPrimaryPlaneAsOverlay() {
  return false;
}

}  // namespace ui
