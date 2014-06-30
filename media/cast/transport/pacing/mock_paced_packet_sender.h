// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_TRANSPORT_PACING_MOCK_PACED_PACKET_SENDER_H_
#define MEDIA_CAST_TRANSPORT_PACING_MOCK_PACED_PACKET_SENDER_H_

#include "media/cast/transport/pacing/paced_sender.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {
namespace cast {
namespace transport {

class MockPacedPacketSender : public PacedPacketSender {
 public:
  MockPacedPacketSender();
  virtual ~MockPacedPacketSender();

  MOCK_METHOD1(SendPackets, bool(const SendPacketVector& packets));
  MOCK_METHOD2(ResendPackets, bool(const SendPacketVector& packets,
                                   base::TimeDelta dedupe_window));
  MOCK_METHOD2(SendRtcpPacket, bool(unsigned int ssrc, PacketRef packet));
  MOCK_METHOD1(CancelSendingPacket, void(const PacketKey& packet_key));
};

}  // namespace transport
}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_TRANSPORT_PACING_MOCK_PACED_PACKET_SENDER_H_
