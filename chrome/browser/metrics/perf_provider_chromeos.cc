// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/metrics/histogram.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/metrics/perf_provider_chromeos.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"

namespace {

// Partition time since login into successive intervals of this size. In each
// interval, pick a random time to collect a profile.
// This interval is twenty-four hours.
const size_t kPerfProfilingIntervalMs = 24 * 60 * 60 * 1000;

// Default time in seconds perf is run for.
const size_t kPerfCommandDurationDefaultSeconds = 2;

// Limit the total size of protobufs that can be cached, so they don't take up
// too much memory. If the size of cached protobufs exceeds this value, stop
// collecting further perf data. The current value is 4 MB.
const size_t kCachedPerfDataProtobufSizeThreshold = 4 * 1024 * 1024;

// Enumeration representing success and various failure modes for collecting and
// sending perf data.
enum GetPerfDataOutcome {
  SUCCESS,
  NOT_READY_TO_UPLOAD,
  NOT_READY_TO_COLLECT,
  INCOGNITO_ACTIVE,
  INCOGNITO_LAUNCHED,
  PROTOBUF_NOT_PARSED,
  NUM_OUTCOMES
};

// Name of the histogram that represents the success and various failure modes
// for collecting and sending perf data.
const char kGetPerfDataOutcomeHistogram[] = "UMA.Perf.GetData";

void AddToPerfHistogram(GetPerfDataOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION(kGetPerfDataOutcomeHistogram,
                            outcome,
                            NUM_OUTCOMES);
}

// Returns true if a normal user is logged in. Returns false if logged in as an
// guest or as a kiosk app.
bool IsNormalUserLoggedIn() {
  chromeos::LoginState* login_state = chromeos::LoginState::Get();
  return (login_state->IsUserLoggedIn() && !login_state->IsGuestUser() &&
          !login_state->IsKioskApp());
}

}  // namespace


namespace metrics {

// This class must be created and used on the UI thread. It watches for any
// incognito window being opened from the time it is instantiated to the time it
// is destroyed.
class WindowedIncognitoObserver : public chrome::BrowserListObserver {
 public:
  WindowedIncognitoObserver() : incognito_launched_(false) {
    BrowserList::AddObserver(this);
  }

  virtual ~WindowedIncognitoObserver() {
    BrowserList::RemoveObserver(this);
  }

  // This method can be checked to see whether any incognito window has been
  // opened since the time this object was created.
  bool incognito_launched() {
    return incognito_launched_;
  }

 private:
  // chrome::BrowserListObserver implementation.
  virtual void OnBrowserAdded(Browser* browser) OVERRIDE {
    if (browser->profile()->IsOffTheRecord())
      incognito_launched_ = true;
  }

  bool incognito_launched_;
};

PerfProvider::PerfProvider()
      : login_observer_(this),
        next_profiling_interval_start_(base::TimeTicks::Now()),
        weak_factory_(this) {
  // Register the login observer with LoginState.
  chromeos::LoginState::Get()->AddObserver(&login_observer_);

  // Check the login state. At the time of writing, this class is instantiated
  // before login. A subsequent login would activate the profiling. However,
  // that behavior may change in the future so that the user is already logged
  // when this class is instantiated. By calling LoggedInStateChanged() here,
  // PerfProvider will recognize that the system is already logged in.
  login_observer_.LoggedInStateChanged();
}

PerfProvider::~PerfProvider() {
  chromeos::LoginState::Get()->RemoveObserver(&login_observer_);
}

bool PerfProvider::GetSampledProfiles(
    std::vector<SampledProfile>* sampled_profiles) {
  DCHECK(CalledOnValidThread());
  if (cached_perf_data_.empty()) {
    AddToPerfHistogram(NOT_READY_TO_UPLOAD);
    return false;
  }

  sampled_profiles->swap(cached_perf_data_);
  cached_perf_data_.clear();

  AddToPerfHistogram(SUCCESS);
  return true;
}

PerfProvider::LoginObserver::LoginObserver(PerfProvider* perf_provider)
    : perf_provider_(perf_provider) {}

void PerfProvider::LoginObserver::LoggedInStateChanged() {
  if (IsNormalUserLoggedIn())
    perf_provider_->OnUserLoggedIn();
  else
    perf_provider_->Deactivate();
}

void PerfProvider::OnUserLoggedIn() {
  login_time_ = base::TimeTicks::Now();
  ScheduleCollection();
}

void PerfProvider::Deactivate() {
  // Stop the timer, but leave |cached_perf_data_| intact.
  timer_.Stop();
}

void PerfProvider::ScheduleCollection() {
  DCHECK(CalledOnValidThread());
  if (timer_.IsRunning())
    return;

  // Pick a random time in the current interval.
  base::TimeTicks scheduled_time =
      next_profiling_interval_start_ +
      base::TimeDelta::FromMilliseconds(
          base::RandGenerator(kPerfProfilingIntervalMs));

  // If the scheduled time has already passed in the time it took to make the
  // above calculations, trigger the collection event immediately.
  base::TimeTicks now = base::TimeTicks::Now();
  if (scheduled_time < now)
    scheduled_time = now;

  timer_.Start(FROM_HERE, scheduled_time - now, this,
               &PerfProvider::DoPeriodicCollection);

  // Update the profiling interval tracker to the start of the next interval.
  next_profiling_interval_start_ +=
      base::TimeDelta::FromMilliseconds(kPerfProfilingIntervalMs);
}

void PerfProvider::CollectIfNecessary(
    SampledProfile::TriggerEvent trigger_event) {
  DCHECK(CalledOnValidThread());

  // Do not collect further data if we've already collected a substantial amount
  // of data, as indicated by |kCachedPerfDataProtobufSizeThreshold|.
  size_t cached_perf_data_size = 0;
  for (size_t i = 0; i < cached_perf_data_.size(); ++i) {
    cached_perf_data_size += cached_perf_data_[i].ByteSize();
  }
  if (cached_perf_data_size >= kCachedPerfDataProtobufSizeThreshold) {
    AddToPerfHistogram(NOT_READY_TO_COLLECT);
    return;
  }

  // For privacy reasons, Chrome should only collect perf data if there is no
  // incognito session active (or gets spawned during the collection).
  if (BrowserList::IsOffTheRecordSessionActive()) {
    AddToPerfHistogram(INCOGNITO_ACTIVE);
    return;
  }

  scoped_ptr<WindowedIncognitoObserver> incognito_observer(
      new WindowedIncognitoObserver);

  chromeos::DebugDaemonClient* client =
      chromeos::DBusThreadManager::Get()->GetDebugDaemonClient();

  base::TimeDelta collection_duration = base::TimeDelta::FromSeconds(
      kPerfCommandDurationDefaultSeconds);

  client->GetPerfData(collection_duration.InSeconds(),
                      base::Bind(&PerfProvider::ParseProtoIfValid,
                                 weak_factory_.GetWeakPtr(),
                                 base::Passed(&incognito_observer),
                                 trigger_event));
}

void PerfProvider::DoPeriodicCollection() {
  CollectIfNecessary(SampledProfile::PERIODIC_COLLECTION);
  ScheduleCollection();
}

void PerfProvider::ParseProtoIfValid(
    scoped_ptr<WindowedIncognitoObserver> incognito_observer,
    SampledProfile::TriggerEvent trigger_event,
    const std::vector<uint8>& data) {
  DCHECK(CalledOnValidThread());

  if (incognito_observer->incognito_launched()) {
    AddToPerfHistogram(INCOGNITO_LAUNCHED);
    return;
  }

  PerfDataProto perf_data_proto;
  if (!perf_data_proto.ParseFromArray(data.data(), data.size())) {
    AddToPerfHistogram(PROTOBUF_NOT_PARSED);
    return;
  }

  // Populate a profile collection protobuf with the collected perf data and
  // extra metadata.
  cached_perf_data_.resize(cached_perf_data_.size() + 1);
  SampledProfile& collection_data = cached_perf_data_.back();
  collection_data.set_trigger_event(trigger_event);
  collection_data.set_ms_after_boot(
      perf_data_proto.timestamp_sec() * base::Time::kMillisecondsPerSecond);

  DCHECK(!login_time_.is_null());
  collection_data.
      set_ms_after_login((base::TimeTicks::Now() - login_time_)
          .InMilliseconds());
  *collection_data.mutable_perf_data() = perf_data_proto;
}

}  // namespace metrics
