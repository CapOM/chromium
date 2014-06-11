// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_WINDOW_SELECTOR_H_
#define ASH_WM_OVERVIEW_WINDOW_SELECTOR_H_

#include <set>
#include <vector>

#include "ash/ash_export.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/time/time.h"
#include "ui/aura/window_observer.h"
#include "ui/aura/window_tracker.h"
#include "ui/events/event_handler.h"
#include "ui/gfx/display_observer.h"
#include "ui/wm/public/activation_change_observer.h"

namespace aura {
class RootWindow;
class Window;
}

namespace gfx {
class Rect;
}

namespace ui {
class LocatedEvent;
}

namespace ash {
class WindowSelectorDelegate;
class WindowSelectorItem;
class WindowSelectorTest;
class WindowGrid;

// The WindowSelector shows a grid of all of your windows, allowing to select
// one by clicking or tapping on it.
class ASH_EXPORT WindowSelector
    : public ui::EventHandler,
      public gfx::DisplayObserver,
      public aura::WindowObserver,
      public aura::client::ActivationChangeObserver {
 public:
  enum Direction {
    LEFT,
    UP,
    RIGHT,
    DOWN
  };

  typedef std::vector<aura::Window*> WindowList;
  typedef ScopedVector<WindowSelectorItem> WindowSelectorItemList;

  WindowSelector(const WindowList& windows,
                 WindowSelectorDelegate* delegate);
  virtual ~WindowSelector();

  // Cancels window selection.
  void CancelSelection();

  // Called when the last window selector item from a grid is deleted.
  void OnGridEmpty(WindowGrid* grid);

  // ui::EventHandler:
  virtual void OnKeyEvent(ui::KeyEvent* event) OVERRIDE;

  // gfx::DisplayObserver:
  virtual void OnDisplayAdded(const gfx::Display& display) OVERRIDE;
  virtual void OnDisplayRemoved(const gfx::Display& display) OVERRIDE;
  virtual void OnDisplayMetricsChanged(const gfx::Display& display,
                                       uint32_t metrics) OVERRIDE;

  // aura::WindowObserver:
  virtual void OnWindowAdded(aura::Window* new_window) OVERRIDE;
  virtual void OnWindowDestroying(aura::Window* window) OVERRIDE;

  // aura::client::ActivationChangeObserver:
  virtual void OnWindowActivated(aura::Window* gained_active,
                                 aura::Window* lost_active) OVERRIDE;
  virtual void OnAttemptToReactivateWindow(
      aura::Window* request_active,
      aura::Window* actual_active) OVERRIDE;

 private:
  friend class WindowSelectorTest;

  // Begins positioning windows such that all windows are visible on the screen.
  void StartOverview();

  // Position all of the windows in the overview.
  void PositionWindows(bool animate);

  // Hide and track all hidden windows not in the overview item list.
  void HideAndTrackNonOverviewWindows();

  // |focus|, restores focus to the stored window.
  void ResetFocusRestoreWindow(bool focus);

  // Helper function that moves the selection widget to |direction| on the
  // corresponding window grid.
  void Move(Direction direction);

  // Tracks observed windows.
  std::set<aura::Window*> observed_windows_;

  // Weak pointer to the selector delegate which will be called when a
  // selection is made.
  WindowSelectorDelegate* delegate_;

  // A weak pointer to the window which was focused on beginning window
  // selection. If window selection is canceled the focus should be restored to
  // this window.
  aura::Window* restore_focus_window_;

  // True when performing operations that may cause window activations. This is
  // used to prevent handling the resulting expected activation.
  bool ignore_activations_;

  // List of all the window overview grids, one for each root window.
  ScopedVector<WindowGrid> grid_list_;

  // The time when overview was started.
  base::Time overview_start_time_;

  // Tracks windows which were hidden because they were not part of the
  // overview.
  aura::WindowTracker hidden_windows_;

  // Tracks the index of the root window the selection widget is in.
  size_t selected_grid_index_;

  DISALLOW_COPY_AND_ASSIGN(WindowSelector);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_WINDOW_SELECTOR_H_
