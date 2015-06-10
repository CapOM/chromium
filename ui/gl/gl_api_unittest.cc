// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_switches.h"

namespace gfx {

class GLApiTest : public testing::Test {
 public:
  void SetUp() override {
    fake_extension_string_ = "";
    num_fake_extension_strings_ = 0;
    fake_extension_strings_ = nullptr;

    driver_.reset(new DriverGL());
    api_.reset(new RealGLApi());

    driver_->fn.glGetStringFn = &FakeGetString;
    driver_->fn.glGetStringiFn = &FakeGetStringi;
    driver_->fn.glGetIntegervFn = &FakeGetIntegervFn;
  }

  void TearDown() override {
    api_.reset(nullptr);
    driver_.reset(nullptr);

    SetGLImplementation(kGLImplementationNone);
    fake_extension_string_ = "";
    num_fake_extension_strings_ = 0;
    fake_extension_strings_ = nullptr;
  }

  void InitializeAPI(base::CommandLine* command_line) {
    api_.reset(new RealGLApi());
    if (command_line)
      api_->InitializeWithCommandLine(driver_.get(), command_line);
    else
      api_->Initialize(driver_.get());
    api_->InitializeWithContext();
  }

  void SetFakeExtensionString(const char* fake_string) {
    SetGLImplementation(kGLImplementationDesktopGL);
    fake_extension_string_ = fake_string;
  }

  void SetFakeExtensionStrings(const char** fake_strings, uint32_t count) {
    SetGLImplementation(kGLImplementationDesktopGLCoreProfile);
    num_fake_extension_strings_ = count;
    fake_extension_strings_ = fake_strings;
  }

  static const GLubyte* GL_BINDING_CALL FakeGetString(GLenum name) {
    return reinterpret_cast<const GLubyte*>(fake_extension_string_);
  }

  static void GL_BINDING_CALL FakeGetIntegervFn(GLenum pname, GLint* params) {
    *params = num_fake_extension_strings_;
  }

  static const GLubyte* GL_BINDING_CALL FakeGetStringi(GLenum name,
                                                       GLuint index) {
    return (index < num_fake_extension_strings_) ?
           reinterpret_cast<const GLubyte*>(fake_extension_strings_[index]) :
           nullptr;
  }

  const char* GetExtensions() {
    return reinterpret_cast<const char*>(api_->glGetStringFn(GL_EXTENSIONS));
  }

  uint32_t GetNumExtensions() {
    GLint num_extensions = 0;
    api_->glGetIntegervFn(GL_NUM_EXTENSIONS, &num_extensions);
    return static_cast<uint32_t>(num_extensions);
  }

  const char* GetExtensioni(uint32_t index) {
    return reinterpret_cast<const char*>(api_->glGetStringiFn(GL_EXTENSIONS,
                                                              index));
  }

 protected:
  static const char* fake_extension_string_;

  static uint32_t num_fake_extension_strings_;
  static const char** fake_extension_strings_;

  scoped_ptr<DriverGL> driver_;
  scoped_ptr<RealGLApi> api_;
};

const char* GLApiTest::fake_extension_string_ = "";

uint32_t GLApiTest::num_fake_extension_strings_ = 0;
const char** GLApiTest::fake_extension_strings_ = nullptr;

TEST_F(GLApiTest, DisabledExtensionStringTest) {
  static const char* kFakeExtensions = "GL_EXT_1 GL_EXT_2 GL_EXT_3 GL_EXT_4";
  static const char* kFakeDisabledExtensions = "GL_EXT_1,GL_EXT_2,GL_FAKE";
  static const char* kFilteredExtensions = "GL_EXT_3 GL_EXT_4";

  SetFakeExtensionString(kFakeExtensions);
  InitializeAPI(nullptr);

  EXPECT_STREQ(kFakeExtensions, GetExtensions());

  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kDisableGLExtensions,
                                 kFakeDisabledExtensions);
  InitializeAPI(&command_line);

  EXPECT_STREQ(kFilteredExtensions, GetExtensions());
}

TEST_F(GLApiTest, DisabledExtensionStringIndexTest) {
  static const char* kFakeExtensions[] = {
    "GL_EXT_1",
    "GL_EXT_2",
    "GL_EXT_3",
    "GL_EXT_4"
  };
  static const char* kFakeDisabledExtensions = "GL_EXT_1,GL_EXT_2,GL_FAKE";
  static const char* kFilteredExtensions[] = {
    "GL_EXT_3",
    "GL_EXT_4"
  };

  SetFakeExtensionStrings(kFakeExtensions, arraysize(kFakeExtensions));
  InitializeAPI(nullptr);

  EXPECT_EQ(arraysize(kFakeExtensions), GetNumExtensions());
  for (uint32_t i = 0; i < arraysize(kFakeExtensions); ++i) {
    EXPECT_STREQ(kFakeExtensions[i], GetExtensioni(i));
  }

  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kDisableGLExtensions,
                                 kFakeDisabledExtensions);
  InitializeAPI(&command_line);

  EXPECT_EQ(arraysize(kFilteredExtensions), GetNumExtensions());
  for (uint32_t i = 0; i < arraysize(kFilteredExtensions); ++i) {
    EXPECT_STREQ(kFilteredExtensions[i], GetExtensioni(i));
  }
}

}  // namespace gfx
