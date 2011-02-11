// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_WIDGET_WIDGET_WIN_H_
#define VIEWS_WIDGET_WIDGET_WIN_H_
#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlmisc.h>

#include <string>
#include <vector>

#include "base/message_loop.h"
#include "base/scoped_ptr.h"
#include "base/scoped_vector.h"
#include "base/win/scoped_comptr.h"
#include "ui/base/win/window_impl.h"
#include "views/focus/focus_manager.h"
#include "views/layout/layout_manager.h"
#include "views/widget/widget.h"

namespace ui {
class ViewProp;
}

namespace gfx {
class CanvasSkia;
class Rect;
}

namespace views {

class CurrentMessageWatcher;
class DefaultThemeProvider;
class DropTargetWin;
class FocusSearch;
class RootView;
class TooltipManagerWin;
class Window;

RootView* GetRootViewForHWND(HWND hwnd);

// A Windows message reflected from other windows. This message is sent
// with the following arguments:
// hWnd - Target window
// uMsg - kReflectedMessage
// wParam - Should be 0
// lParam - Pointer to MSG struct containing the original message.
const int kReflectedMessage = WM_APP + 3;

// These two messages aren't defined in winuser.h, but they are sent to windows
// with captions. They appear to paint the window caption and frame.
// Unfortunately if you override the standard non-client rendering as we do
// with CustomFrameWindow, sometimes Windows (not deterministically
// reproducibly but definitely frequently) will send these messages to the
// window and paint the standard caption/title over the top of the custom one.
// So we need to handle these messages in CustomFrameWindow to prevent this
// from happening.
const int WM_NCUAHDRAWCAPTION = 0xAE;
const int WM_NCUAHDRAWFRAME = 0xAF;

///////////////////////////////////////////////////////////////////////////////
//
// WidgetWin
//  A Widget for a views hierarchy used to represent anything that can be
//  contained within an HWND, e.g. a control, a window, etc. Specializations
//  suitable for specific tasks, e.g. top level window, are derived from this.
//
//  This Widget contains a RootView which owns the hierarchy of views within it.
//  As long as views are part of this tree, they will be deleted automatically
//  when the RootView is destroyed. If you remove a view from the tree, you are
//  then responsible for cleaning up after it.
//
///////////////////////////////////////////////////////////////////////////////
class WidgetWin : public ui::WindowImpl,
                  public Widget,
                  public MessageLoopForUI::Observer,
                  public FocusTraversable {
 public:
  WidgetWin();
  virtual ~WidgetWin();

  // Returns the Widget associated with the specified HWND (if any).
  static WidgetWin* GetWidget(HWND hwnd);

  // Returns the root Widget associated with the specified HWND (if any).
  static WidgetWin* GetRootWidget(HWND hwnd);

  // Returns true if we are on Windows Vista or greater and composition is
  // enabled.
  static bool IsAeroGlassEnabled();

  void set_delete_on_destroy(bool delete_on_destroy) {
    delete_on_destroy_ = delete_on_destroy;
  }

  // See description of use_layered_buffer_ for details.
  void SetUseLayeredBuffer(bool use_layered_buffer);

  // Disable Layered Window updates by setting to false.
  void set_can_update_layered_window(bool can_update_layered_window) {
    can_update_layered_window_ = can_update_layered_window;
  }

  // Obtain the view event with the given MSAA child id.  Used in
  // ViewAccessibility::get_accChild to support requests for children of
  // windowless controls.  May return NULL (see ViewHierarchyChanged).
  View* GetAccessibilityViewEventAt(int id);

  // Add a view that has recently fired an accessibility event.  Returns a MSAA
  // child id which is generated by: -(index of view in vector + 1) which
  // guarantees a negative child id.  This distinguishes the view from
  // positive MSAA child id's which are direct leaf children of views that have
  // associated hWnd's (e.g. WidgetWin).
  int AddAccessibilityViewEvent(View* view);

  // Clear a view that has recently been removed on a hierarchy change.
  void ClearAccessibilityViewEvent(View* view);

  BEGIN_MSG_MAP_EX(WidgetWin)
    // Range handlers must go first!
    MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseRange)
    MESSAGE_RANGE_HANDLER_EX(WM_NCMOUSEMOVE, WM_NCMOUSEMOVE, OnMouseRange)

    // Reflected message handler
    MESSAGE_HANDLER_EX(kReflectedMessage, OnReflectedMessage)

    // CustomFrameWindow hacks
    MESSAGE_HANDLER_EX(WM_NCUAHDRAWCAPTION, OnNCUAHDrawCaption)
    MESSAGE_HANDLER_EX(WM_NCUAHDRAWFRAME, OnNCUAHDrawFrame)

    // Vista and newer
    MESSAGE_HANDLER_EX(WM_DWMCOMPOSITIONCHANGED, OnDwmCompositionChanged)

    // Non-atlcrack.h handlers
    MESSAGE_HANDLER_EX(WM_GETOBJECT, OnGetObject)
    MESSAGE_HANDLER_EX(WM_NCMOUSELEAVE, OnNCMouseLeave)
    MESSAGE_HANDLER_EX(WM_MOUSELEAVE, OnMouseLeave)
    MESSAGE_HANDLER_EX(WM_MOUSEWHEEL, OnMouseWheel)

    // This list is in _ALPHABETICAL_ order! OR I WILL HURT YOU.
    MSG_WM_ACTIVATE(OnActivate)
    MSG_WM_ACTIVATEAPP(OnActivateApp)
    MSG_WM_APPCOMMAND(OnAppCommand)
    MSG_WM_CANCELMODE(OnCancelMode)
    MSG_WM_CAPTURECHANGED(OnCaptureChanged)
    MSG_WM_CLOSE(OnClose)
    MSG_WM_COMMAND(OnCommand)
    MSG_WM_CREATE(OnCreate)
    MSG_WM_DESTROY(OnDestroy)
    MSG_WM_DISPLAYCHANGE(OnDisplayChange)
    MSG_WM_ERASEBKGND(OnEraseBkgnd)
    MSG_WM_ENDSESSION(OnEndSession)
    MSG_WM_ENTERSIZEMOVE(OnEnterSizeMove)
    MSG_WM_EXITMENULOOP(OnExitMenuLoop)
    MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
    MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
    MSG_WM_HSCROLL(OnHScroll)
    MSG_WM_INITMENU(OnInitMenu)
    MSG_WM_INITMENUPOPUP(OnInitMenuPopup)
    MSG_WM_KEYDOWN(OnKeyDown)
    MSG_WM_KEYUP(OnKeyUp)
    MSG_WM_KILLFOCUS(OnKillFocus)
    MSG_WM_SYSKEYDOWN(OnKeyDown)
    MSG_WM_SYSKEYUP(OnKeyUp)
    MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
    MSG_WM_LBUTTONDOWN(OnLButtonDown)
    MSG_WM_LBUTTONUP(OnLButtonUp)
    MSG_WM_MBUTTONDOWN(OnMButtonDown)
    MSG_WM_MBUTTONUP(OnMButtonUp)
    MSG_WM_MBUTTONDBLCLK(OnMButtonDblClk)
    MSG_WM_MOUSEACTIVATE(OnMouseActivate)
    MSG_WM_MOUSEMOVE(OnMouseMove)
    MSG_WM_MOVE(OnMove)
    MSG_WM_MOVING(OnMoving)
    MSG_WM_NCACTIVATE(OnNCActivate)
    MSG_WM_NCCALCSIZE(OnNCCalcSize)
    MSG_WM_NCHITTEST(OnNCHitTest)
    MSG_WM_NCMOUSEMOVE(OnNCMouseMove)
    MSG_WM_NCLBUTTONDBLCLK(OnNCLButtonDblClk)
    MSG_WM_NCLBUTTONDOWN(OnNCLButtonDown)
    MSG_WM_NCLBUTTONUP(OnNCLButtonUp)
    MSG_WM_NCMBUTTONDBLCLK(OnNCMButtonDblClk)
    MSG_WM_NCMBUTTONDOWN(OnNCMButtonDown)
    MSG_WM_NCMBUTTONUP(OnNCMButtonUp)
    MSG_WM_NCPAINT(OnNCPaint)
    MSG_WM_NCRBUTTONDBLCLK(OnNCRButtonDblClk)
    MSG_WM_NCRBUTTONDOWN(OnNCRButtonDown)
    MSG_WM_NCRBUTTONUP(OnNCRButtonUp)
    MSG_WM_NOTIFY(OnNotify)
    MSG_WM_PAINT(OnPaint)
    MSG_WM_POWERBROADCAST(OnPowerBroadcast)
    MSG_WM_RBUTTONDBLCLK(OnRButtonDblClk)
    MSG_WM_RBUTTONDOWN(OnRButtonDown)
    MSG_WM_RBUTTONUP(OnRButtonUp)
    MSG_WM_SETFOCUS(OnSetFocus)
    MSG_WM_SETICON(OnSetIcon)
    MSG_WM_SETTEXT(OnSetText)
    MSG_WM_SETTINGCHANGE(OnSettingChange)
    MSG_WM_SIZE(OnSize)
    MSG_WM_SYSCOMMAND(OnSysCommand)
    MSG_WM_THEMECHANGED(OnThemeChanged)
    MSG_WM_VSCROLL(OnVScroll)
    MSG_WM_WINDOWPOSCHANGING(OnWindowPosChanging)
    MSG_WM_WINDOWPOSCHANGED(OnWindowPosChanged)
  END_MSG_MAP()

  // Overridden from Widget:
  virtual void Init(gfx::NativeView parent, const gfx::Rect& bounds);
  virtual void InitWithWidget(Widget* parent, const gfx::Rect& bounds);
  virtual WidgetDelegate* GetWidgetDelegate();
  virtual void SetWidgetDelegate(WidgetDelegate* delegate);
  virtual void SetContentsView(View* view);
  virtual void GetBounds(gfx::Rect* out, bool including_frame) const;
  virtual void SetBounds(const gfx::Rect& bounds);
  virtual void MoveAbove(Widget* other);
  virtual void SetShape(gfx::NativeRegion region);
  virtual void Close();
  virtual void CloseNow();
  virtual void Show();
  virtual void Hide();
  virtual gfx::NativeView GetNativeView() const;
  virtual void PaintNow(const gfx::Rect& update_rect);
  virtual void SetOpacity(unsigned char opacity);
  virtual void SetAlwaysOnTop(bool on_top);
  virtual RootView* GetRootView();
  virtual Widget* GetRootWidget() const;
  virtual bool IsVisible() const;
  virtual bool IsActive() const;
  virtual bool IsAccessibleWidget() const;
  virtual TooltipManager* GetTooltipManager();
  virtual void GenerateMousePressedForView(View* view,
                                           const gfx::Point& point);
  virtual bool GetAccelerator(int cmd_id, ui::Accelerator* accelerator);
  virtual Window* GetWindow();
  virtual const Window* GetWindow() const;
  virtual void SetNativeWindowProperty(const char* name, void* value);
  virtual void* GetNativeWindowProperty(const char* name);
  virtual ThemeProvider* GetThemeProvider() const;
  virtual ThemeProvider* GetDefaultThemeProvider() const;
  virtual FocusManager* GetFocusManager();
  virtual void ViewHierarchyChanged(bool is_add, View *parent,
                                    View *child);
  virtual bool ContainsNativeView(gfx::NativeView native_view);

  // Overridden from MessageLoop::Observer:
  void WillProcessMessage(const MSG& msg);
  virtual void DidProcessMessage(const MSG& msg);

  // Overridden from FocusTraversable:
  virtual FocusSearch* GetFocusSearch();
  virtual FocusTraversable* GetFocusTraversableParent();
  virtual View* GetFocusTraversableParentView();

  void SetFocusTraversableParent(FocusTraversable* parent);
  void SetFocusTraversableParentView(View* parent_view);

  BOOL IsWindow() const {
    return ::IsWindow(GetNativeView());
  }

  BOOL ShowWindow(int command) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::ShowWindow(GetNativeView(), command);
  }

  HWND SetCapture() {
    DCHECK(::IsWindow(GetNativeView()));
    return ::SetCapture(GetNativeView());
  }

  HWND GetParent() const {
    return ::GetParent(GetNativeView());
  }

  LONG GetWindowLong(int index) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::GetWindowLong(GetNativeView(), index);
  }

  BOOL GetWindowRect(RECT* rect) const {
    return ::GetWindowRect(GetNativeView(), rect);
  }

  LONG SetWindowLong(int index, LONG new_long) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::SetWindowLong(GetNativeView(), index, new_long);
  }

  BOOL SetWindowPos(HWND hwnd_after, int x, int y, int cx, int cy, UINT flags) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::SetWindowPos(GetNativeView(), hwnd_after, x, y, cx, cy, flags);
  }

  BOOL IsZoomed() const {
    DCHECK(::IsWindow(GetNativeView()));
    return ::IsZoomed(GetNativeView());
  }

  BOOL MoveWindow(int x, int y, int width, int height) {
    return MoveWindow(x, y, width, height, TRUE);
  }

  BOOL MoveWindow(int x, int y, int width, int height, BOOL repaint) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::MoveWindow(GetNativeView(), x, y, width, height, repaint);
  }

  int SetWindowRgn(HRGN region, BOOL redraw) {
    DCHECK(::IsWindow(GetNativeView()));
    return ::SetWindowRgn(GetNativeView(), region, redraw);
  }

  BOOL GetClientRect(RECT* rect) const {
    DCHECK(::IsWindow(GetNativeView()));
    return ::GetClientRect(GetNativeView(), rect);
  }

  // Resets the last move flag so that we can go around the optimization
  // that disregards duplicate mouse moves when ending animation requires
  // a new hit-test to do some highlighting as in TabStrip::RemoveTabAnimation
  // to cause the close button to highlight.
  void ResetLastMouseMoveFlag() {
    last_mouse_event_was_move_ = false;
  }

 protected:
  // Overridden from WindowImpl:
  virtual HICON GetDefaultWindowIcon() const;
  virtual LRESULT OnWndProc(UINT message, WPARAM w_param, LPARAM l_param);

  // Message Handlers
  // These are all virtual so that specialized Widgets can modify or augment
  // processing.
  // This list is in _ALPHABETICAL_ order!
  // Note: in the base class these functions must do nothing but convert point
  //       coordinates to client coordinates (if necessary) and forward the
  //       handling to the appropriate Process* function. This is so that
  //       subclasses can easily override these methods to do different things
  //       and have a convenient function to call to get the default behavior.
  virtual void OnActivate(UINT action, BOOL minimized, HWND window);
  virtual void OnActivateApp(BOOL active, DWORD thread_id);
  virtual LRESULT OnAppCommand(HWND window, short app_command, WORD device,
                               int keystate);
  virtual void OnCancelMode();
  virtual void OnCaptureChanged(HWND hwnd);
  virtual void OnClose();
  virtual void OnCommand(UINT notification_code, int command_id, HWND window);
  virtual LRESULT OnCreate(CREATESTRUCT* create_struct);
  // WARNING: If you override this be sure and invoke super, otherwise we'll
  // leak a few things.
  virtual void OnDestroy();
  virtual void OnDisplayChange(UINT bits_per_pixel, CSize screen_size);
  virtual LRESULT OnDwmCompositionChanged(UINT msg,
                                          WPARAM w_param,
                                          LPARAM l_param);
  virtual void OnEndSession(BOOL ending, UINT logoff);
  virtual void OnEnterSizeMove();
  virtual LRESULT OnEraseBkgnd(HDC dc);
  virtual void OnExitMenuLoop(BOOL is_track_popup_menu);
  virtual void OnExitSizeMove();
  virtual LRESULT OnGetObject(UINT uMsg, WPARAM w_param, LPARAM l_param);
  virtual void OnGetMinMaxInfo(MINMAXINFO* minmax_info);
  virtual void OnHScroll(int scroll_type, short position, HWND scrollbar);
  virtual void OnInitMenu(HMENU menu);
  virtual void OnInitMenuPopup(HMENU menu, UINT position, BOOL is_system_menu);
  virtual void OnKeyDown(TCHAR c, UINT rep_cnt, UINT flags);
  virtual void OnKeyUp(TCHAR c, UINT rep_cnt, UINT flags);
  virtual void OnKillFocus(HWND focused_window);
  virtual void OnLButtonDblClk(UINT flags, const CPoint& point);
  virtual void OnLButtonDown(UINT flags, const CPoint& point);
  virtual void OnLButtonUp(UINT flags, const CPoint& point);
  virtual void OnMButtonDblClk(UINT flags, const CPoint& point);
  virtual void OnMButtonDown(UINT flags, const CPoint& point);
  virtual void OnMButtonUp(UINT flags, const CPoint& point);
  virtual LRESULT OnMouseActivate(HWND window, UINT hittest_code, UINT message);
  virtual void OnMouseMove(UINT flags, const CPoint& point);
  virtual LRESULT OnMouseLeave(UINT message, WPARAM w_param, LPARAM l_param);
  virtual LRESULT OnMouseWheel(UINT message, WPARAM w_param, LPARAM l_param);
  virtual void OnMove(const CPoint& point);
  virtual void OnMoving(UINT param, LPRECT new_bounds);
  virtual LRESULT OnMouseRange(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual LRESULT OnNCActivate(BOOL active);
  virtual LRESULT OnNCCalcSize(BOOL w_param, LPARAM l_param);
  virtual LRESULT OnNCHitTest(const CPoint& pt);
  virtual void OnNCLButtonDblClk(UINT flags, const CPoint& point);
  virtual void OnNCLButtonDown(UINT flags, const CPoint& point);
  virtual void OnNCLButtonUp(UINT flags, const CPoint& point);
  virtual void OnNCMButtonDblClk(UINT flags, const CPoint& point);
  virtual void OnNCMButtonDown(UINT flags, const CPoint& point);
  virtual void OnNCMButtonUp(UINT flags, const CPoint& point);
  virtual LRESULT OnNCMouseLeave(UINT uMsg, WPARAM w_param, LPARAM l_param);
  virtual LRESULT OnNCMouseMove(UINT flags, const CPoint& point);
  virtual void OnNCPaint(HRGN rgn);
  virtual void OnNCRButtonDblClk(UINT flags, const CPoint& point);
  virtual void OnNCRButtonDown(UINT flags, const CPoint& point);
  virtual void OnNCRButtonUp(UINT flags, const CPoint& point);
  virtual LRESULT OnNCUAHDrawCaption(UINT msg,
                                     WPARAM w_param,
                                     LPARAM l_param);
  virtual LRESULT OnNCUAHDrawFrame(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual LRESULT OnNotify(int w_param, NMHDR* l_param);
  virtual void OnPaint(HDC dc);
  virtual LRESULT OnPowerBroadcast(DWORD power_event, DWORD data);
  virtual void OnRButtonDblClk(UINT flags, const CPoint& point);
  virtual void OnRButtonDown(UINT flags, const CPoint& point);
  virtual void OnRButtonUp(UINT flags, const CPoint& point);
  virtual LRESULT OnReflectedMessage(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual void OnSetFocus(HWND focused_window);
  virtual LRESULT OnSetIcon(UINT size_type, HICON new_icon);
  virtual LRESULT OnSetText(const wchar_t* text);
  virtual void OnSettingChange(UINT flags, const wchar_t* section);
  virtual void OnSize(UINT param, const CSize& size);
  virtual void OnSysCommand(UINT notification_code, CPoint click);
  virtual void OnThemeChanged();
  virtual void OnVScroll(int scroll_type, short position, HWND scrollbar);
  virtual void OnWindowPosChanging(WINDOWPOS* window_pos);
  virtual void OnWindowPosChanged(WINDOWPOS* window_pos);

  // deletes this window as it is destroyed, override to provide different
  // behavior.
  virtual void OnFinalMessage(HWND window);

  // Returns the size that the RootView should be set to in LayoutRootView().
  virtual gfx::Size GetRootViewSize() const;

  // Start tracking all mouse events so that this window gets sent mouse leave
  // messages too.
  void TrackMouseEvents(DWORD mouse_tracking_flags);

  // Actually handle mouse events. These functions are called by subclasses who
  // override the message handlers above to do the actual real work of handling
  // the event in the View system.
  bool ProcessMousePressed(const CPoint& point,
                           UINT flags,
                           bool dbl_click,
                           bool non_client);
  void ProcessMouseDragged(const CPoint& point, UINT flags);
  void ProcessMouseReleased(const CPoint& point, UINT flags);
  void ProcessMouseMoved(const CPoint& point, UINT flags, bool is_nonclient);
  void ProcessMouseExited();

  // Lays out the root view to fit the appropriate area within the widget.
  // Called when the window size or non client metrics change.
  void LayoutRootView();

  // Called when a MSAA screen reader cleint is detected.
  virtual void OnScreenReaderDetected();

  // Returns whether capture should be released on mouse release. The default
  // is true.
  virtual bool ReleaseCaptureOnMouseReleased();

  // Creates the RootView to be used within this Widget. Can be overridden to
  // create specialized RootView implementations.
  virtual RootView* CreateRootView();

  // Returns true if this WidgetWin is opaque.
  bool opaque() const { return opaque_; }

  // The TooltipManager.
  // WARNING: RootView's destructor calls into the TooltipManager. As such, this
  // must be destroyed AFTER root_view_.
  scoped_ptr<TooltipManagerWin> tooltip_manager_;

  scoped_refptr<DropTargetWin> drop_target_;

  // The focus manager keeping track of focus for this Widget and any of its
  // children.  NULL for non top-level widgets.
  // WARNING: RootView's destructor calls into the FocusManager. As such, this
  // must be destroyed AFTER root_view_.
  scoped_ptr<FocusManager> focus_manager_;

  // The root of the View hierarchy attached to this window.
  // WARNING: see warning in tooltip_manager_ for ordering dependencies with
  // this and tooltip_manager_.
  scoped_ptr<RootView> root_view_;

  // Whether or not we have capture the mouse.
  bool has_capture_;

  // If true, the mouse is currently down.
  bool is_mouse_down_;

  // Are a subclass of WindowWin?
  bool is_window_;

 private:
  typedef ScopedVector<ui::ViewProp> ViewProps;

  // Implementation of GetWindow. Ascends the parents of |hwnd| returning the
  // first ancestor that is a Window.
  static Window* GetWindowImpl(HWND hwnd);

  // Resize the bitmap used to contain the contents of the layered window. This
  // recreates the entire bitmap.
  void SizeContents(const gfx::Size& window_size);

  // Paint into a DIB and then update the layered window with its contents.
  void PaintLayeredWindow();

  // In layered mode, update the layered window. |dib_dc| represents a handle
  // to a device context that contains the contents of the window.
  void UpdateWindowFromContents(HDC dib_dc);

  // Invoked from WM_DESTROY. Does appropriate cleanup and invokes OnDestroy
  // so that subclasses can do any cleanup they need to.
  // void OnDestroyImpl();

  // Returns the RootView that contains the focused view, or NULL if there is no
  // focused view.
  RootView* GetFocusedViewRootView();

  // Called after the WM_ACTIVATE message has been processed by the default
  // windows procedure.
  static void PostProcessActivateMessage(WidgetWin* widget,
                                         int activation_state);

  // The following factory is used for calls to close the WidgetWin
  // instance.
  ScopedRunnableMethodFactory<WidgetWin> close_widget_factory_;

  // The flags currently being used with TrackMouseEvent to track mouse
  // messages. 0 if there is no active tracking. The value of this member is
  // used when tracking is canceled.
  DWORD active_mouse_tracking_flags_;

  bool opaque_;

  // Should we keep an offscreen buffer? This is initially true and if the
  // window has WS_EX_LAYERED then it remains true. You can set this to false
  // at any time to ditch the buffer, and similarly set back to true to force
  // creation of the buffer.
  //
  // NOTE: this is intended to be used with a layered window (a window with an
  // extended window style of WS_EX_LAYERED). If you are using a layered window
  // and NOT changing the layered alpha or anything else, then leave this value
  // alone. OTOH if you are invoking SetLayeredWindowAttributes then you'll
  // must likely want to set this to false, or after changing the alpha toggle
  // the extended style bit to false than back to true. See MSDN for more
  // details.
  bool use_layered_buffer_;

  // The default alpha to be applied to the layered window.
  BYTE layered_alpha_;

  // A canvas that contains the window contents in the case of a layered
  // window.
  scoped_ptr<gfx::CanvasSkia> contents_;

  // Whether or not the window should delete itself when it is destroyed.
  // Set this to false via its setter for stack allocated instances.
  bool delete_on_destroy_;

  // True if we are allowed to update the layered window from the DIB backing
  // store if necessary.
  bool can_update_layered_window_;

  // The following are used to detect duplicate mouse move events and not
  // deliver them. Displaying a window may result in the system generating
  // duplicate move events even though the mouse hasn't moved.

  // If true, the last event was a mouse move event.
  bool last_mouse_event_was_move_;

  // Coordinates of the last mouse move event, in screen coordinates.
  int last_mouse_move_x_;
  int last_mouse_move_y_;

  // Whether the focus should be restored next time we get enabled.  Needed to
  // restore focus correctly when Windows modal dialogs are displayed.
  bool restore_focus_when_enabled_;

  // Instance of accessibility information and handling for MSAA root
  base::win::ScopedComPtr<IAccessible> accessibility_root_;

  scoped_ptr<DefaultThemeProvider> default_theme_provider_;

  // Non owned pointer to optional delegate.  May be NULL if no delegate is
  // being used.
  WidgetDelegate* delegate_;

  // Value determines whether the Widget is customized for accessibility.
  static bool screen_reader_active_;

  // The maximum number of view events in our vector below.
  static const int kMaxAccessibilityViewEvents = 20;

  // A vector used to access views for which we have sent notifications to
  // accessibility clients.  It is used as a circular queue.
  std::vector<View*> accessibility_view_events_;

  // The current position of the view events vector.  When incrementing,
  // we always mod this value with the max view events above .
  int accessibility_view_events_index_;

  ViewProps props_;

  // Keeps track of the current message.
  static CurrentMessageWatcher* message_watcher_;

  DISALLOW_COPY_AND_ASSIGN(WidgetWin);
};

}  // namespace views

#endif  // VIEWS_WIDGET_WIDGET_WIN_H_
