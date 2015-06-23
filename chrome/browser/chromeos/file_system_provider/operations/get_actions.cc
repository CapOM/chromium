// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/operations/get_actions.h"

#include <algorithm>
#include <string>

#include "chrome/common/extensions/api/file_system_provider.h"
#include "chrome/common/extensions/api/file_system_provider_internal.h"

namespace chromeos {
namespace file_system_provider {
namespace operations {
namespace {

// Convert the request |value| into a list of actions.
Actions ConvertRequestValueToActions(scoped_ptr<RequestValue> value) {
  using extensions::api::file_system_provider_internal::
      GetActionsRequestedSuccess::Params;

  const Params* params = value->get_actions_success_params();
  DCHECK(params);

  Actions result;
  for (const auto& idl_action : params->actions) {
    Action action;
    action.id = idl_action->id;
    action.title = idl_action->title.get() ? *idl_action->title : std::string();
    result.push_back(action);
  }

  return result;
}

}  // namespace

GetActions::GetActions(
    extensions::EventRouter* event_router,
    const ProvidedFileSystemInfo& file_system_info,
    const base::FilePath& entry_path,
    const ProvidedFileSystemInterface::GetActionsCallback& callback)
    : Operation(event_router, file_system_info),
      entry_path_(entry_path),
      callback_(callback) {
}

GetActions::~GetActions() {
}

bool GetActions::Execute(int request_id) {
  using extensions::api::file_system_provider::GetActionsRequestedOptions;

  GetActionsRequestedOptions options;
  options.file_system_id = file_system_info_.file_system_id();
  options.request_id = request_id;
  options.entry_path = entry_path_.AsUTF8Unsafe();

  return SendEvent(
      request_id,
      extensions::api::file_system_provider::OnGetActionsRequested::kEventName,
      extensions::api::file_system_provider::OnGetActionsRequested::Create(
          options));
}

void GetActions::OnSuccess(int /* request_id */,
                           scoped_ptr<RequestValue> result,
                           bool has_more) {
  callback_.Run(ConvertRequestValueToActions(result.Pass()),
                base::File::FILE_OK);
}

void GetActions::OnError(int /* request_id */,
                         scoped_ptr<RequestValue> /* result */,
                         base::File::Error error) {
  callback_.Run(Actions(), error);
}

}  // namespace operations
}  // namespace file_system_provider
}  // namespace chromeos
