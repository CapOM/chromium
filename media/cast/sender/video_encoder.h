// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_SENDER_VIDEO_ENCODER_H_
#define MEDIA_CAST_SENDER_VIDEO_ENCODER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "media/base/video_frame.h"
#include "media/cast/cast_config.h"
#include "media/cast/cast_environment.h"

namespace media {
namespace cast {

class VideoFrameFactory;

// All these functions are called from the main cast thread.
class VideoEncoder {
 public:
  typedef base::Callback<void(scoped_ptr<EncodedFrame>)> FrameEncodedCallback;

  virtual ~VideoEncoder() {}

  // Returns true if the size of video frames passed in successive calls to
  // EncodedVideoFrame() can vary.
  virtual bool CanEncodeVariedFrameSizes() const = 0;

  // If true is returned, the Encoder has accepted the request and will process
  // it asynchronously, running |frame_encoded_callback| on the MAIN
  // CastEnvironment thread with the result.  If false is returned, nothing
  // happens and the callback will not be run.
  virtual bool EncodeVideoFrame(
      const scoped_refptr<media::VideoFrame>& video_frame,
      const base::TimeTicks& reference_time,
      const FrameEncodedCallback& frame_encoded_callback) = 0;

  // Inform the encoder about the new target bit rate.
  virtual void SetBitRate(int new_bit_rate) = 0;

  // Inform the encoder to encode the next frame as a key frame.
  virtual void GenerateKeyFrame() = 0;

  // Inform the encoder to only reference frames older or equal to frame_id;
  virtual void LatestFrameIdToReference(uint32 frame_id) = 0;

  // Creates a |VideoFrameFactory| object to vend |VideoFrame| object with
  // encoder affinity (defined as offering some sort of performance benefit).
  // This is an optional capability and by default returns null.
  virtual scoped_ptr<VideoFrameFactory> CreateVideoFrameFactory();

  // Instructs the encoder to finish and emit all frames that have been
  // submitted for encoding. An encoder may hold a certain number of frames for
  // analysis. Under certain network conditions, particularly when there is
  // network congestion, it is necessary to flush out of the encoder all
  // submitted frames so that eventually new frames may be encoded. Like
  // EncodeVideoFrame(), the encoder will process this request asynchronously.
  virtual void EmitFrames();
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_SENDER_VIDEO_ENCODER_H_
