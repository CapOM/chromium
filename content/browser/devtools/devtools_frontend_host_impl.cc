// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_frontend_host_impl.h"

#include "content/common/devtools_messages.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

// static
DevToolsFrontendHost* DevToolsFrontendHost::Create(
    WebContents* frontend_web_contents,
    DevToolsFrontendHost::Delegate* delegate) {
  return new DevToolsFrontendHostImpl(frontend_web_contents, delegate);
}

DevToolsFrontendHostImpl::DevToolsFrontendHostImpl(
    WebContents* frontend_web_contents,
    DevToolsFrontendHost::Delegate* delegate)
    : WebContentsObserver(frontend_web_contents), delegate_(delegate) {
  RenderFrameHost* main_frame_host = web_contents()->GetMainFrame();
  main_frame_host->Send(
      new DevToolsMsg_SetupDevToolsClient(main_frame_host->GetRoutingID()));
}

DevToolsFrontendHostImpl::~DevToolsFrontendHostImpl() {
}

bool DevToolsFrontendHostImpl::OnMessageReceived(
    const IPC::Message& message,
    RenderFrameHost* render_frame_host) {
  if (render_frame_host != web_contents()->GetMainFrame())
    return false;
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DevToolsFrontendHostImpl, message)
    IPC_MESSAGE_HANDLER(DevToolsAgentMsg_DispatchOnInspectorBackend,
                        OnDispatchOnInspectorBackend)
    IPC_MESSAGE_HANDLER(DevToolsHostMsg_DispatchOnEmbedder,
                        OnDispatchOnEmbedder)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void DevToolsFrontendHostImpl::OnDispatchOnInspectorBackend(
    const std::string& message) {
  delegate_->HandleMessageFromDevToolsFrontendToBackend(message);
}

void DevToolsFrontendHostImpl::OnDispatchOnEmbedder(
    const std::string& message) {
  delegate_->HandleMessageFromDevToolsFrontend(message);
}

}  // namespace content
