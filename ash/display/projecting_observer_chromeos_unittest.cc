// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/projecting_observer_chromeos.h"

#include "base/memory/scoped_vector.h"
#include "chromeos/dbus/fake_dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/chromeos/test/test_display_snapshot.h"

namespace ash {
namespace {

ui::TestDisplaySnapshot* CreateInternalSnapshot() {
  ui::TestDisplaySnapshot* output = new ui::TestDisplaySnapshot();
  output->set_type(ui::OUTPUT_TYPE_INTERNAL);
  return output;
}

ui::TestDisplaySnapshot* CreateVGASnapshot() {
  ui::TestDisplaySnapshot* output = new ui::TestDisplaySnapshot();
  output->set_type(ui::OUTPUT_TYPE_VGA);
  return output;
}

ui::OutputConfigurator::DisplayStateList CreateOutputs(
    const ScopedVector<ui::TestDisplaySnapshot>& displays) {
  ui::OutputConfigurator::DisplayStateList outputs;
  for (size_t i = 0; i < displays.size(); ++i) {
    ui::OutputConfigurator::DisplayState state;
    state.display = displays[i];
    outputs.push_back(state);
  }

  return outputs;
}

class ProjectingObserverTest : public testing::Test {
 public:
  ProjectingObserverTest() : observer_(new ProjectingObserver()) {
    chromeos::FakeDBusThreadManager* dbus_manager =
        new chromeos::FakeDBusThreadManager();
    fake_power_client_ = new chromeos::FakePowerManagerClient();

    dbus_manager->SetPowerManagerClient(
        scoped_ptr<chromeos::PowerManagerClient>(fake_power_client_));

    // Takes ownership of |dbus_manager|.
    chromeos::DBusThreadManager::InitializeForTesting(dbus_manager);
  }

  virtual ~ProjectingObserverTest() {
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  scoped_ptr<ProjectingObserver> observer_;
  chromeos::FakePowerManagerClient* fake_power_client_;  //  Not owned.

  DISALLOW_COPY_AND_ASSIGN(ProjectingObserverTest);
};

}  // namespace

TEST_F(ProjectingObserverTest, CheckNoDisplay) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  EXPECT_EQ(1, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithoutInternalDisplay) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateVGASnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  EXPECT_EQ(1, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithInternalDisplay) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateInternalSnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  EXPECT_EQ(1, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithTwoVGADisplays) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateVGASnapshot());
  displays.push_back(CreateVGASnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  EXPECT_EQ(1, fake_power_client_->num_set_is_projecting_calls());
  // We need at least 1 internal display to set projecting to on.
  EXPECT_FALSE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithInternalAndVGADisplays) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateInternalSnapshot());
  displays.push_back(CreateVGASnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  EXPECT_EQ(1, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithVGADisplayAndOneCastingSession) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateVGASnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  observer_->OnCastingSessionStartedOrStopped(true);

  EXPECT_EQ(2, fake_power_client_->num_set_is_projecting_calls());
  // Need at least one internal display to set projecting state to |true|.
  EXPECT_FALSE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithInternalDisplayAndOneCastingSession) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateInternalSnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  observer_->OnCastingSessionStartedOrStopped(true);

  EXPECT_EQ(2, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest, CheckProjectingAfterClosingACastingSession) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateInternalSnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  observer_->OnCastingSessionStartedOrStopped(true);
  observer_->OnCastingSessionStartedOrStopped(true);

  EXPECT_EQ(3, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_->is_projecting());

  observer_->OnCastingSessionStartedOrStopped(false);

  EXPECT_EQ(4, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest,
       CheckStopProjectingAfterClosingAllCastingSessions) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateInternalSnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  observer_->OnCastingSessionStartedOrStopped(true);
  observer_->OnCastingSessionStartedOrStopped(false);

  EXPECT_EQ(3, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_->is_projecting());
}

TEST_F(ProjectingObserverTest,
       CheckStopProjectingAfterDisconnectingSecondOutput) {
  ScopedVector<ui::TestDisplaySnapshot> displays;
  displays.push_back(CreateInternalSnapshot());
  displays.push_back(CreateVGASnapshot());
  ui::OutputConfigurator::DisplayStateList outputs = CreateOutputs(displays);
  observer_->OnDisplayModeChanged(outputs);

  // Remove VGA output.
  outputs.erase(outputs.begin() + 1);
  observer_->OnDisplayModeChanged(outputs);

  EXPECT_EQ(2, fake_power_client_->num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_->is_projecting());
}

}  // namespace ash
