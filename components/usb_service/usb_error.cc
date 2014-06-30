// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/usb_service/usb_error.h"

#include "third_party/libusb/src/libusb/libusb.h"

namespace usb_service {

std::string ConvertErrorToString(int errcode) {
  return libusb_strerror(static_cast<libusb_error>(errcode));
}

}  // namespace usb_service;
