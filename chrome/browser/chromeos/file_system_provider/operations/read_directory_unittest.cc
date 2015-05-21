// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/operations/read_directory.h"

#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/values.h"
#include "chrome/browser/chromeos/file_system_provider/operations/get_metadata.h"
#include "chrome/browser/chromeos/file_system_provider/operations/test_util.h"
#include "chrome/common/extensions/api/file_system_provider.h"
#include "chrome/common/extensions/api/file_system_provider_capabilities/file_system_provider_capabilities_handler.h"
#include "chrome/common/extensions/api/file_system_provider_internal.h"
#include "extensions/browser/event_router.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace file_system_provider {
namespace operations {
namespace {

const char kExtensionId[] = "mbflcebpggnecokmikipoihdbecnjfoj";
const char kFileSystemId[] = "testing-file-system";
const int kRequestId = 2;
const base::FilePath::CharType kDirectoryPath[] =
    FILE_PATH_LITERAL("/directory");

// Callback invocation logger. Acts as a fileapi end-point.
class CallbackLogger {
 public:
  class Event {
   public:
    Event(base::File::Error result,
          const storage::AsyncFileUtil::EntryList& entry_list,
          bool has_more)
        : result_(result), entry_list_(entry_list), has_more_(has_more) {}
    virtual ~Event() {}

    base::File::Error result() { return result_; }
    const storage::AsyncFileUtil::EntryList& entry_list() {
      return entry_list_;
    }
    bool has_more() { return has_more_; }

   private:
    base::File::Error result_;
    storage::AsyncFileUtil::EntryList entry_list_;
    bool has_more_;

    DISALLOW_COPY_AND_ASSIGN(Event);
  };

  CallbackLogger() {}
  virtual ~CallbackLogger() {}

  void OnReadDirectory(base::File::Error result,
                       const storage::AsyncFileUtil::EntryList& entry_list,
                       bool has_more) {
    events_.push_back(new Event(result, entry_list, has_more));
  }

  ScopedVector<Event>& events() { return events_; }

 private:
  ScopedVector<Event> events_;

  DISALLOW_COPY_AND_ASSIGN(CallbackLogger);
};

// Returns the request value as |result| in case of successful parse.
void CreateRequestValueFromJSON(const std::string& json,
                                scoped_ptr<RequestValue>* result) {
  using extensions::api::file_system_provider_internal::
      ReadDirectoryRequestedSuccess::Params;

  int json_error_code;
  std::string json_error_msg;
  scoped_ptr<base::Value> value(base::JSONReader::ReadAndReturnError(
      json, base::JSON_PARSE_RFC, &json_error_code, &json_error_msg));
  ASSERT_TRUE(value.get()) << json_error_msg;

  base::ListValue* value_as_list;
  ASSERT_TRUE(value->GetAsList(&value_as_list));
  scoped_ptr<Params> params(Params::Create(*value_as_list));
  ASSERT_TRUE(params.get());
  *result = RequestValue::CreateForReadDirectorySuccess(params.Pass());
  ASSERT_TRUE(result->get());
}

}  // namespace

class FileSystemProviderOperationsReadDirectoryTest : public testing::Test {
 protected:
  FileSystemProviderOperationsReadDirectoryTest() {}
  ~FileSystemProviderOperationsReadDirectoryTest() override {}

  void SetUp() override {
    file_system_info_ = ProvidedFileSystemInfo(
        kExtensionId, MountOptions(kFileSystemId, "" /* display_name */),
        base::FilePath(), false /* configurable */, extensions::SOURCE_FILE);
  }

  ProvidedFileSystemInfo file_system_info_;
};

TEST_F(FileSystemProviderOperationsReadDirectoryTest, Execute) {
  using extensions::api::file_system_provider::ReadDirectoryRequestedOptions;

  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  CallbackLogger callback_logger;

  ReadDirectory read_directory(NULL, file_system_info_,
                               base::FilePath(kDirectoryPath),
                               base::Bind(&CallbackLogger::OnReadDirectory,
                                          base::Unretained(&callback_logger)));
  read_directory.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(read_directory.Execute(kRequestId));

  ASSERT_EQ(1u, dispatcher.events().size());
  extensions::Event* event = dispatcher.events()[0];
  EXPECT_EQ(extensions::api::file_system_provider::OnReadDirectoryRequested::
                kEventName,
            event->event_name);
  base::ListValue* event_args = event->event_args.get();
  ASSERT_EQ(1u, event_args->GetSize());

  const base::DictionaryValue* options_as_value = NULL;
  ASSERT_TRUE(event_args->GetDictionary(0, &options_as_value));

  ReadDirectoryRequestedOptions options;
  ASSERT_TRUE(
      ReadDirectoryRequestedOptions::Populate(*options_as_value, &options));
  EXPECT_EQ(kFileSystemId, options.file_system_id);
  EXPECT_EQ(kRequestId, options.request_id);
  EXPECT_EQ(kDirectoryPath, options.directory_path);
}

TEST_F(FileSystemProviderOperationsReadDirectoryTest, Execute_NoListener) {
  util::LoggingDispatchEventImpl dispatcher(false /* dispatch_reply */);
  CallbackLogger callback_logger;

  ReadDirectory read_directory(NULL, file_system_info_,
                               base::FilePath(kDirectoryPath),
                               base::Bind(&CallbackLogger::OnReadDirectory,
                                          base::Unretained(&callback_logger)));
  read_directory.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_FALSE(read_directory.Execute(kRequestId));
}

TEST_F(FileSystemProviderOperationsReadDirectoryTest, OnSuccess) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  CallbackLogger callback_logger;

  ReadDirectory read_directory(NULL, file_system_info_,
                               base::FilePath(kDirectoryPath),
                               base::Bind(&CallbackLogger::OnReadDirectory,
                                          base::Unretained(&callback_logger)));
  read_directory.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(read_directory.Execute(kRequestId));

  // Sample input as JSON. Keep in sync with file_system_provider_api.idl.
  // As for now, it is impossible to create *::Params class directly, not from
  // base::Value.
  const std::string input =
      "[\n"
      "  \"testing-file-system\",\n"  // kFileSystemId
      "  2,\n"                        // kRequestId
      "  [\n"
      "    {\n"
      "      \"isDirectory\": false,\n"
      "      \"name\": \"blueberries.txt\",\n"
      "      \"size\": 4096,\n"
      "      \"modificationTime\": {\n"
      "        \"value\": \"Thu Apr 24 00:46:52 UTC 2014\"\n"
      "      }\n"
      "    }\n"
      "  ],\n"
      "  false,\n"  // has_more
      "  0\n"       // execution_time
      "]\n";
  scoped_ptr<RequestValue> request_value;
  ASSERT_NO_FATAL_FAILURE(CreateRequestValueFromJSON(input, &request_value));

  const bool has_more = false;
  read_directory.OnSuccess(kRequestId, request_value.Pass(), has_more);

  ASSERT_EQ(1u, callback_logger.events().size());
  CallbackLogger::Event* event = callback_logger.events()[0];
  EXPECT_EQ(base::File::FILE_OK, event->result());

  ASSERT_EQ(1u, event->entry_list().size());
  const storage::DirectoryEntry entry = event->entry_list()[0];
  EXPECT_FALSE(entry.is_directory);
  EXPECT_EQ("blueberries.txt", entry.name);
  EXPECT_EQ(4096, entry.size);
  base::Time expected_time;
  EXPECT_TRUE(
      base::Time::FromString("Thu Apr 24 00:46:52 UTC 2014", &expected_time));
  EXPECT_EQ(expected_time, entry.last_modified_time);
}

TEST_F(FileSystemProviderOperationsReadDirectoryTest,
       OnSuccess_InvalidMetadata) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  CallbackLogger callback_logger;

  ReadDirectory read_directory(NULL, file_system_info_,
                               base::FilePath(kDirectoryPath),
                               base::Bind(&CallbackLogger::OnReadDirectory,
                                          base::Unretained(&callback_logger)));
  read_directory.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(read_directory.Execute(kRequestId));

  // Sample input as JSON. Keep in sync with file_system_provider_api.idl.
  // As for now, it is impossible to create *::Params class directly, not from
  // base::Value.
  const std::string input =
      "[\n"
      "  \"testing-file-system\",\n"  // kFileSystemId
      "  2,\n"                        // kRequestId
      "  [\n"
      "    {\n"
      "      \"isDirectory\": false,\n"
      "      \"name\": \"blue/berries.txt\",\n"
      "      \"size\": 4096,\n"
      "      \"modificationTime\": {\n"
      "        \"value\": \"Thu Apr 24 00:46:52 UTC 2014\"\n"
      "      }\n"
      "    }\n"
      "  ],\n"
      "  false,\n"  // has_more
      "  0\n"       // execution_time
      "]\n";
  scoped_ptr<RequestValue> request_value;
  ASSERT_NO_FATAL_FAILURE(CreateRequestValueFromJSON(input, &request_value));

  const bool has_more = false;
  read_directory.OnSuccess(kRequestId, request_value.Pass(), has_more);

  ASSERT_EQ(1u, callback_logger.events().size());
  CallbackLogger::Event* event = callback_logger.events()[0];
  EXPECT_EQ(base::File::FILE_ERROR_IO, event->result());

  EXPECT_EQ(0u, event->entry_list().size());
}

TEST_F(FileSystemProviderOperationsReadDirectoryTest, OnError) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  CallbackLogger callback_logger;

  ReadDirectory read_directory(NULL, file_system_info_,
                               base::FilePath(kDirectoryPath),
                               base::Bind(&CallbackLogger::OnReadDirectory,
                                          base::Unretained(&callback_logger)));
  read_directory.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(read_directory.Execute(kRequestId));

  read_directory.OnError(kRequestId,
                         scoped_ptr<RequestValue>(new RequestValue()),
                         base::File::FILE_ERROR_TOO_MANY_OPENED);

  ASSERT_EQ(1u, callback_logger.events().size());
  CallbackLogger::Event* event = callback_logger.events()[0];
  EXPECT_EQ(base::File::FILE_ERROR_TOO_MANY_OPENED, event->result());
  ASSERT_EQ(0u, event->entry_list().size());
}

}  // namespace operations
}  // namespace file_system_provider
}  // namespace chromeos
