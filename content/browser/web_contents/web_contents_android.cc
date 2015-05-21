// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_contents_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "content/browser/accessibility/browser_accessibility_android.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/android/interstitial_page_delegate_android.h"
#include "content/browser/frame_host/interstitial_page_impl.h"
#include "content/browser/media/android/browser_media_player_manager.h"
#include "content/browser/media/media_web_contents_observer.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/devtools_messages.h"
#include "content/common/frame_messages.h"
#include "content/common/input_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "jni/WebContentsImpl_jni.h"
#include "net/android/network_library.h"
#include "ui/accessibility/ax_node_data.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;
using base::android::ScopedJavaGlobalRef;
using base::android::ToJavaIntArray;

namespace content {

namespace {

void JavaScriptResultCallback(const ScopedJavaGlobalRef<jobject>& callback,
                              const base::Value* result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  std::string json;
  base::JSONWriter::Write(*result, &json);
  ScopedJavaLocalRef<jstring> j_json = ConvertUTF8ToJavaString(env, json);
  Java_WebContentsImpl_onEvaluateJavaScriptResult(
      env, j_json.obj(), callback.obj());
}

ScopedJavaLocalRef<jobject> WalkAXTreeDepthFirst(JNIEnv* env,
      BrowserAccessibilityAndroid* node) {

  ScopedJavaLocalRef<jstring> j_text =
      ConvertUTF16ToJavaString(env, node->GetText());
  ScopedJavaLocalRef<jstring> j_class =
      ConvertUTF8ToJavaString(env, node->GetClassName());
  const gfx::Rect& location = node->GetLocation();
  ScopedJavaLocalRef<jobject> j_node =
      Java_WebContentsImpl_createAccessibilitySnapshotNode(env,
          location.x(), location.y(), node->GetScrollX(),
          node->GetScrollY(), location.width(), location.height(),
          j_text.obj(), j_class.obj());

  for(uint32 i = 0; i < node->PlatformChildCount(); i++) {
    BrowserAccessibilityAndroid* child =
        static_cast<BrowserAccessibilityAndroid*>(
            node->PlatformGetChild(i));
    Java_WebContentsImpl_addAccessibilityNodeAsChild(env,
        j_node.obj(), WalkAXTreeDepthFirst(env, child).obj());
  }
  return j_node;
}

// Walks over the AXTreeUpdate and creates a light weight snapshot.
void AXTreeSnapshotCallback(const ScopedJavaGlobalRef<jobject>& callback,
                            const ui::AXTreeUpdate& result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (result.nodes.empty()) {
    Java_WebContentsImpl_onAccessibilitySnapshot(env, nullptr, callback.obj());
    return;
  }
  scoped_ptr<BrowserAccessibilityManager> manager(
      BrowserAccessibilityManager::Create(result, nullptr));
  BrowserAccessibilityAndroid* root =
      static_cast<BrowserAccessibilityAndroid*>(manager->GetRoot());
  ScopedJavaLocalRef<jobject> j_root = WalkAXTreeDepthFirst(env, root);
  Java_WebContentsImpl_onAccessibilitySnapshot(
      env, j_root.obj(), callback.obj());
}

void ReleaseAllMediaPlayers(WebContents* web_contents,
                            RenderFrameHost* render_frame_host) {
  BrowserMediaPlayerManager* manager =
      static_cast<WebContentsImpl*>(web_contents)->
          media_web_contents_observer()->GetMediaPlayerManager(
              render_frame_host);
  if (manager)
    manager->ReleaseAllMediaPlayers();
}

}  // namespace

// static
WebContents* WebContents::FromJavaWebContents(
    jobject jweb_contents_android) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!jweb_contents_android)
    return NULL;

  WebContentsAndroid* web_contents_android =
      reinterpret_cast<WebContentsAndroid*>(
          Java_WebContentsImpl_getNativePointer(AttachCurrentThread(),
                                                jweb_contents_android));
  if (!web_contents_android)
    return NULL;
  return web_contents_android->web_contents();
}

// static
static void DestroyWebContents(JNIEnv* env,
                               jclass clazz,
                               jlong jweb_contents_android_ptr) {
  WebContentsAndroid* web_contents_android =
      reinterpret_cast<WebContentsAndroid*>(jweb_contents_android_ptr);
  if (!web_contents_android)
    return;

  WebContents* web_contents = web_contents_android->web_contents();
  if (!web_contents)
    return;

  delete web_contents;
}

// static
bool WebContentsAndroid::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

WebContentsAndroid::WebContentsAndroid(WebContents* web_contents)
    : web_contents_(web_contents),
      navigation_controller_(&(web_contents->GetController())),
      weak_factory_(this) {
  JNIEnv* env = AttachCurrentThread();
  obj_.Reset(env,
             Java_WebContentsImpl_create(
                 env,
                 reinterpret_cast<intptr_t>(this),
                 navigation_controller_.GetJavaObject().obj()).obj());
  RendererPreferences* prefs = web_contents_->GetMutableRendererPrefs();
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  prefs->network_contry_iso =
      command_line->HasSwitch(switches::kNetworkCountryIso) ?
          command_line->GetSwitchValueASCII(switches::kNetworkCountryIso)
          : net::android::GetTelephonyNetworkCountryIso();
}

WebContentsAndroid::~WebContentsAndroid() {
  Java_WebContentsImpl_clearNativePtr(AttachCurrentThread(), obj_.obj());
}

base::android::ScopedJavaLocalRef<jobject>
WebContentsAndroid::GetJavaObject() {
  return base::android::ScopedJavaLocalRef<jobject>(obj_);
}

ScopedJavaLocalRef<jstring> WebContentsAndroid::GetTitle(
    JNIEnv* env, jobject obj) const {
  return base::android::ConvertUTF16ToJavaString(env,
                                                 web_contents_->GetTitle());
}

ScopedJavaLocalRef<jstring> WebContentsAndroid::GetVisibleURL(
    JNIEnv* env, jobject obj) const {
  return base::android::ConvertUTF8ToJavaString(
      env, web_contents_->GetVisibleURL().spec());
}

bool WebContentsAndroid::IsLoading(JNIEnv* env, jobject obj) const {
  return web_contents_->IsLoading();
}

bool WebContentsAndroid::IsLoadingToDifferentDocument(JNIEnv* env,
                                                      jobject obj) const {
  return web_contents_->IsLoadingToDifferentDocument();
}

void WebContentsAndroid::Stop(JNIEnv* env, jobject obj) {
  web_contents_->Stop();
}

void WebContentsAndroid::InsertCSS(
    JNIEnv* env, jobject jobj, jstring jcss) {
  web_contents_->InsertCSS(base::android::ConvertJavaStringToUTF8(env, jcss));
}

RenderWidgetHostViewAndroid*
    WebContentsAndroid::GetRenderWidgetHostViewAndroid() {
  RenderWidgetHostView* rwhv = NULL;
  rwhv = web_contents_->GetRenderWidgetHostView();
  if (web_contents_->ShowingInterstitialPage()) {
    rwhv = web_contents_->GetInterstitialPage()
               ->GetMainFrame()
               ->GetRenderViewHost()
               ->GetView();
  }
  return static_cast<RenderWidgetHostViewAndroid*>(rwhv);
}

jint WebContentsAndroid::GetBackgroundColor(JNIEnv* env, jobject obj) {
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (!rwhva)
    return SK_ColorWHITE;
  return rwhva->GetCachedBackgroundColor();
}

ScopedJavaLocalRef<jstring> WebContentsAndroid::GetURL(JNIEnv* env,
                                                       jobject obj) const {
  return ConvertUTF8ToJavaString(env, web_contents_->GetURL().spec());
}

ScopedJavaLocalRef<jstring> WebContentsAndroid::GetLastCommittedURL(
    JNIEnv* env,
    jobject) const {
  return ConvertUTF8ToJavaString(env,
                                 web_contents_->GetLastCommittedURL().spec());
}


jboolean WebContentsAndroid::IsIncognito(JNIEnv* env, jobject obj) {
  return web_contents_->GetBrowserContext()->IsOffTheRecord();
}

void WebContentsAndroid::ResumeLoadingCreatedWebContents(JNIEnv* env,
                                                         jobject obj) {
  web_contents_->ResumeLoadingCreatedWebContents();
}

void WebContentsAndroid::OnHide(JNIEnv* env, jobject obj) {
  web_contents_->WasHidden();
}

void WebContentsAndroid::OnShow(JNIEnv* env, jobject obj) {
  web_contents_->WasShown();
}

void WebContentsAndroid::ReleaseMediaPlayers(JNIEnv* env, jobject jobj) {
#if defined(ENABLE_BROWSER_CDMS)
  web_contents_->ForEachFrame(
      base::Bind(&ReleaseAllMediaPlayers, base::Unretained(web_contents_)));
#endif // defined(ENABLE_BROWSER_CDMS)
}

void WebContentsAndroid::ShowInterstitialPage(
    JNIEnv* env,
    jobject obj,
    jstring jurl,
    jlong delegate_ptr) {
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));
  InterstitialPageDelegateAndroid* delegate =
      reinterpret_cast<InterstitialPageDelegateAndroid*>(delegate_ptr);
  InterstitialPage* interstitial = InterstitialPage::Create(
      web_contents_, false, url, delegate);
  delegate->set_interstitial_page(interstitial);
  interstitial->Show();
}

jboolean WebContentsAndroid::IsShowingInterstitialPage(JNIEnv* env,
                                                        jobject obj) {
  return web_contents_->ShowingInterstitialPage();
}

jboolean WebContentsAndroid::IsRenderWidgetHostViewReady(
    JNIEnv* env,
    jobject obj) {
  RenderWidgetHostViewAndroid* view = GetRenderWidgetHostViewAndroid();
  return view && view->HasValidFrame();
}

void WebContentsAndroid::ExitFullscreen(JNIEnv* env, jobject obj) {
  web_contents_->ExitFullscreen();
}

void WebContentsAndroid::UpdateTopControlsState(
    JNIEnv* env,
    jobject obj,
    bool enable_hiding,
    bool enable_showing,
    bool animate) {
  RenderViewHost* host = web_contents_->GetRenderViewHost();
  if (!host)
    return;
  host->Send(new ViewMsg_UpdateTopControlsState(host->GetRoutingID(),
                                                enable_hiding,
                                                enable_showing,
                                                animate));
}

void WebContentsAndroid::ShowImeIfNeeded(JNIEnv* env, jobject obj) {
  RenderViewHost* host = web_contents_->GetRenderViewHost();
  if (!host)
    return;
  host->Send(new ViewMsg_ShowImeIfNeeded(host->GetRoutingID()));
}

void WebContentsAndroid::ScrollFocusedEditableNodeIntoView(
    JNIEnv* env,
    jobject obj) {
  RenderViewHost* host = web_contents_->GetRenderViewHost();
  if (!host)
    return;
  host->Send(new InputMsg_ScrollFocusedEditableNodeIntoRect(
      host->GetRoutingID(), gfx::Rect()));
}

void WebContentsAndroid::SelectWordAroundCaret(JNIEnv* env, jobject obj) {
  RenderViewHost* host = web_contents_->GetRenderViewHost();
  if (!host)
    return;
  host->SelectWordAroundCaret();
}

void WebContentsAndroid::EvaluateJavaScript(JNIEnv* env,
                                            jobject obj,
                                            jstring script,
                                            jobject callback) {
  RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  DCHECK(rvh);

  if (!rvh->IsRenderViewLive()) {
    if (!static_cast<WebContentsImpl*>(web_contents_)->
        CreateRenderViewForInitialEmptyDocument()) {
      LOG(ERROR) << "Failed to create RenderView in EvaluateJavaScript";
      return;
    }
  }

  if (!callback) {
    // No callback requested.
    web_contents_->GetMainFrame()->ExecuteJavaScript(
        ConvertJavaStringToUTF16(env, script));
    return;
  }

  // Secure the Java callback in a scoped object and give ownership of it to the
  // base::Callback.
  ScopedJavaGlobalRef<jobject> j_callback;
  j_callback.Reset(env, callback);
  RenderFrameHost::JavaScriptResultCallback js_callback =
      base::Bind(&JavaScriptResultCallback, j_callback);

  web_contents_->GetMainFrame()->ExecuteJavaScript(
      ConvertJavaStringToUTF16(env, script), js_callback);
}

void WebContentsAndroid::AddMessageToDevToolsConsole(JNIEnv* env,
                                                     jobject jobj,
                                                     jint level,
                                                     jstring message) {
  DCHECK_GE(level, 0);
  DCHECK_LE(level, CONSOLE_MESSAGE_LEVEL_LAST);

  web_contents_->GetMainFrame()->Send(new DevToolsAgentMsg_AddMessageToConsole(
      web_contents_->GetMainFrame()->GetRoutingID(),
      static_cast<ConsoleMessageLevel>(level),
      ConvertJavaStringToUTF8(env, message)));
}

jboolean WebContentsAndroid::HasAccessedInitialDocument(
    JNIEnv* env,
    jobject jobj) {
  return static_cast<WebContentsImpl*>(web_contents_)->
      HasAccessedInitialDocument();
}

jint WebContentsAndroid::GetThemeColor(JNIEnv* env, jobject obj) {
  return web_contents_->GetThemeColor();
}

void WebContentsAndroid::RequestAccessibilitySnapshot(JNIEnv* env,
                                                      jobject obj,
                                                      jobject callback) {
  // Secure the Java callback in a scoped object and give ownership of it to the
  // base::Callback.
  ScopedJavaGlobalRef<jobject> j_callback;
  j_callback.Reset(env, callback);
  WebContentsImpl::AXTreeSnapshotCallback snapshot_callback =
      base::Bind(&AXTreeSnapshotCallback, j_callback);

  static_cast<WebContentsImpl*>(web_contents_)->RequestAXTreeSnapshot(
      snapshot_callback);
}

}  // namespace content
