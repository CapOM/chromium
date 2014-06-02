// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MOJO_MOJO_APPLICATION_HOST_H_
#define CONTENT_BROWSER_MOJO_MOJO_APPLICATION_HOST_H_

#include "base/process/process_handle.h"
#include "mojo/common/channel_init.h"
#include "mojo/embedder/scoped_platform_handle.h"
#include "mojo/public/interfaces/service_provider/service_provider.mojom.h"

namespace IPC {
class Sender;
}

namespace content {

// MojoApplicationHost represents the code needed on the browser side to setup
// a child process as a Mojo application via Chrome IPC. The child process
// should use MojoApplication to handle messages generated by an instance of
// MojoApplicationHost. MojoApplicationHost makes the mojo::ShellClient
// interface available so that child-provided services can be invoked.
class MojoApplicationHost {
 public:
  MojoApplicationHost();
  virtual ~MojoApplicationHost();

  // Two-phase initialization:
  //  1- Init makes the shell_client() available synchronously.
  //  2- Activate establishes the actual connection to the peer process.
  bool Init();
  bool Activate(IPC::Sender* sender, base::ProcessHandle process_handle);

  bool did_activate() const { return did_activate_; }

  mojo::ServiceProvider* service_provider() {
    DCHECK(child_service_provider_.get());
    return child_service_provider_->client();
  }

 private:
  class ServiceProviderImpl
      : public mojo::InterfaceImpl<mojo::ServiceProvider> {
   public:
    virtual void OnConnectionError() OVERRIDE {
      // TODO(darin): How should we handle this error?
    }

    // mojo::ServiceProvider methods:
    virtual void ConnectToService(
        const mojo::String& url,
        const mojo::String& name,
        mojo::ScopedMessagePipeHandle handle) OVERRIDE;
  };

  mojo::common::ChannelInit channel_init_;
  mojo::embedder::ScopedPlatformHandle client_handle_;

  scoped_ptr<ServiceProviderImpl> child_service_provider_;

  bool did_activate_;

  DISALLOW_COPY_AND_ASSIGN(MojoApplicationHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MOJO_MOJO_APPLICATION_HOST_H_
