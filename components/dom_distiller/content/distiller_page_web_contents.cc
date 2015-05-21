// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/content/distiller_page_web_contents.h"

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "components/dom_distiller/content/web_contents_main_frame_observer.h"
#include "components/dom_distiller/core/distiller_page.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/gfx/screen.h"
#include "url/gurl.h"

namespace dom_distiller {

SourcePageHandleWebContents::SourcePageHandleWebContents(
    content::WebContents* web_contents,
    bool owned)
    : web_contents_(web_contents), owned_(owned) {
}

SourcePageHandleWebContents::~SourcePageHandleWebContents() {
  if (owned_) {
    delete web_contents_;
  }
}

scoped_ptr<DistillerPage> DistillerPageWebContentsFactory::CreateDistillerPage(
    const gfx::Size& render_view_size) const {
  DCHECK(browser_context_);
  return scoped_ptr<DistillerPage>(new DistillerPageWebContents(
      browser_context_, render_view_size,
      scoped_ptr<SourcePageHandleWebContents>()));
}

scoped_ptr<DistillerPage>
DistillerPageWebContentsFactory::CreateDistillerPageWithHandle(
    scoped_ptr<SourcePageHandle> handle) const {
  DCHECK(browser_context_);
  scoped_ptr<SourcePageHandleWebContents> web_contents_handle =
      scoped_ptr<SourcePageHandleWebContents>(
          static_cast<SourcePageHandleWebContents*>(handle.release()));
  return scoped_ptr<DistillerPage>(new DistillerPageWebContents(
      browser_context_, gfx::Size(), web_contents_handle.Pass()));
}

DistillerPageWebContents::DistillerPageWebContents(
    content::BrowserContext* browser_context,
    const gfx::Size& render_view_size,
    scoped_ptr<SourcePageHandleWebContents> optional_web_contents_handle)
    : state_(IDLE),
      source_page_handle_(nullptr),
      browser_context_(browser_context),
      render_view_size_(render_view_size) {
  if (optional_web_contents_handle) {
    source_page_handle_ = optional_web_contents_handle.Pass();
    if (render_view_size.IsEmpty())
      render_view_size_ =
          source_page_handle_->web_contents()->GetContainerBounds().size();
  }
}

DistillerPageWebContents::~DistillerPageWebContents() {
}

bool DistillerPageWebContents::StringifyOutput() {
 return false;
}

bool DistillerPageWebContents::CreateNewContext() {
 return true;
}

void DistillerPageWebContents::DistillPageImpl(const GURL& url,
                                               const std::string& script) {
  DCHECK(browser_context_);
  DCHECK(state_ == IDLE);
  state_ = LOADING_PAGE;
  script_ = script;

  if (source_page_handle_ && source_page_handle_->web_contents() &&
      source_page_handle_->web_contents()->GetLastCommittedURL() == url) {
    WebContentsMainFrameObserver* main_frame_observer =
        WebContentsMainFrameObserver::FromWebContents(
            source_page_handle_->web_contents());
    if (main_frame_observer && main_frame_observer->is_initialized()) {
      if (main_frame_observer->is_document_loaded_in_main_frame()) {
        // Main frame has already loaded for the current WebContents, so execute
        // JavaScript immediately.
        ExecuteJavaScript();
      } else {
        // Main frame document has not loaded yet, so wait until it has before
        // executing JavaScript. It will trigger after DocumentLoadedInFrame is
        // called for the main frame.
        content::WebContentsObserver::Observe(
            source_page_handle_->web_contents());
      }
    } else {
      // The WebContentsMainFrameObserver has not been correctly initialized,
      // so fall back to creating a new WebContents.
      CreateNewWebContents(url);
    }
  } else {
    CreateNewWebContents(url);
  }
}

void DistillerPageWebContents::CreateNewWebContents(const GURL& url) {
  // Create new WebContents to use for distilling the content.
  content::WebContents::CreateParams create_params(browser_context_);
  create_params.initially_hidden = true;
  content::WebContents* web_contents =
      content::WebContents::Create(create_params);
  DCHECK(web_contents);

  web_contents->SetDelegate(this);

  // Start observing WebContents and load the requested URL.
  content::WebContentsObserver::Observe(web_contents);
  content::NavigationController::LoadURLParams params(url);
  web_contents->GetController().LoadURLWithParams(params);

  source_page_handle_.reset(
      new SourcePageHandleWebContents(web_contents, true));
}

gfx::Size DistillerPageWebContents::GetSizeForNewRenderView(
    content::WebContents* web_contents) const {
  gfx::Size size(render_view_size_);
  if (size.IsEmpty())
    size = web_contents->GetContainerBounds().size();
  // If size is still empty, set it to fullscreen so that document.offsetWidth
  // in the executed domdistiller.js won't be 0.
  if (size.IsEmpty()) {
    DVLOG(1) << "Using fullscreen as default RenderView size";
    size = gfx::Screen::GetNativeScreen()->GetPrimaryDisplay().size();
  }
  return size;
}

void DistillerPageWebContents::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host ==
      source_page_handle_->web_contents()->GetMainFrame()) {
    ExecuteJavaScript();
  }
}

void DistillerPageWebContents::DidFailLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description) {
  if (!render_frame_host->GetParent()) {
    content::WebContentsObserver::Observe(NULL);
    DCHECK(state_ == LOADING_PAGE || state_ == EXECUTING_JAVASCRIPT);
    state_ = PAGELOAD_FAILED;
    scoped_ptr<base::Value> empty = base::Value::CreateNullValue();
    OnWebContentsDistillationDone(GURL(), empty.get());
  }
}

void DistillerPageWebContents::ExecuteJavaScript() {
  content::RenderFrameHost* frame =
      source_page_handle_->web_contents()->GetMainFrame();
  DCHECK(frame);
  DCHECK_EQ(LOADING_PAGE, state_);
  state_ = EXECUTING_JAVASCRIPT;
  content::WebContentsObserver::Observe(NULL);
  // Stop any pending navigation since the intent is to distill the current
  // page.
  source_page_handle_->web_contents()->Stop();
  DVLOG(1) << "Beginning distillation";
  frame->ExecuteJavaScript(
      base::UTF8ToUTF16(script_),
      base::Bind(&DistillerPageWebContents::OnWebContentsDistillationDone,
                 base::Unretained(this),
                 source_page_handle_->web_contents()->GetLastCommittedURL()));
}

void DistillerPageWebContents::OnWebContentsDistillationDone(
    const GURL& page_url,
    const base::Value* value) {
  DCHECK(state_ == PAGELOAD_FAILED || state_ == EXECUTING_JAVASCRIPT);
  state_ = IDLE;
  DistillerPage::OnDistillationDone(page_url, value);
}

}  // namespace dom_distiller
