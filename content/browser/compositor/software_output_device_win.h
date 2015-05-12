// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_SOFTWARE_OUTPUT_DEVICE_WIN_H_
#define CONTENT_BROWSER_COMPOSITOR_SOFTWARE_OUTPUT_DEVICE_WIN_H_

#include "base/memory/scoped_ptr.h"
#include "cc/output/software_output_device.h"

#include <windows.h>

namespace gfx {
class Canvas;
}

namespace ui {
class Compositor;
}

namespace content {

class SoftwareOutputDeviceWin : public cc::SoftwareOutputDevice {
 public:
  explicit SoftwareOutputDeviceWin(ui::Compositor* compositor);
  ~SoftwareOutputDeviceWin() override;

  void Resize(const gfx::Size& viewport_pixel_size,
              float scale_factor) override;
  SkCanvas* BeginPaint(const gfx::Rect& damage_rect) override;
  void EndPaint(cc::SoftwareFrameData* frame_data) override;

 private:
  HWND hwnd_;
  BITMAPINFO bitmap_info_;
  scoped_ptr<gfx::Canvas> contents_;
  bool is_hwnd_composited_;

  DISALLOW_COPY_AND_ASSIGN(SoftwareOutputDeviceWin);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_SOFTWARE_OUTPUT_DEVICE_WIN_H_
