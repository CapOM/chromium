// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_MEMORY_H_
#define MOJO_EDK_SYSTEM_MEMORY_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>  // For |memcpy()|.

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/edk/system/system_impl_export.h"
#include "mojo/public/c/system/macros.h"

namespace mojo {
namespace system {

namespace internal {

// Removes |const| from |T| (available as |remove_const<T>::type|):
// TODO(vtl): Remove these once we have the C++11 |remove_const|.
template <typename T>
struct remove_const {
  using type = T;
};
template <typename T>
struct remove_const<const T> {
  using type = T;
};

// Yields |(const) char| if |T| is |(const) void|, else |T|:
template <typename T>
struct VoidToChar {
  using type = T;
};
template <>
struct VoidToChar<void> {
  using type = char;
};
template <>
struct VoidToChar<const void> {
  using type = const char;
};

// Checks (insofar as appropriate/possible) that |pointer| is a valid pointer to
// a buffer of the given size and alignment (both in bytes).
template <size_t size, size_t alignment>
void MOJO_SYSTEM_IMPL_EXPORT CheckUserPointer(const void* pointer);

// Checks (insofar as appropriate/possible) that |pointer| is a valid pointer to
// a buffer of |count| elements of the given size and alignment (both in bytes).
template <size_t size, size_t alignment>
void MOJO_SYSTEM_IMPL_EXPORT
CheckUserPointerWithCount(const void* pointer, size_t count);

// Checks (insofar as appropriate/possible) that |pointer| is a valid pointer to
// a buffer of the given size and alignment (both in bytes).
template <size_t alignment>
void MOJO_SYSTEM_IMPL_EXPORT
CheckUserPointerWithSize(const void* pointer, size_t size);

}  // namespace internal

// Forward declarations so that they can be friended.
template <typename Type>
class UserPointerReader;
template <typename Type>
class UserPointerWriter;
template <typename Type>
class UserPointerReaderWriter;
template <class Options>
class UserOptionsReader;

// Provides a convenient way to implicitly get null |UserPointer<Type>|s.
struct NullUserPointer {};

// Represents a user pointer to a single |Type| (which must be POD), for Mojo
// primitive parameters.
//
// Use a const |Type| for in parameters, and non-const |Type|s for out and
// in-out parameters (in which case the |Put()| method is available).
template <typename Type>
class UserPointer {
 private:
  using NonVoidType = typename internal::VoidToChar<Type>::type;

 public:
  // Instead of explicitly using these constructors, you can often use
  // |MakeUserPointer()| (or |NullUserPointer()| for null pointers). (The common
  // exception is when you have, e.g., a |char*| and want to get a
  // |UserPointer<void>|.)
  UserPointer() : pointer_(nullptr) {}
  explicit UserPointer(Type* pointer) : pointer_(pointer) {}
  // Allow implicit conversion from the "null user pointer".
  UserPointer(NullUserPointer) : pointer_(nullptr) {}
  ~UserPointer() {}

  // Allow assignment from the "null user pointer".
  UserPointer<Type>& operator=(NullUserPointer) {
    pointer_ = nullptr;
    return *this;
  }

  // Allow conversion to a "non-const" |UserPointer|.
  operator UserPointer<const Type>() const {
    return UserPointer<const Type>(pointer_);
  }

  bool IsNull() const { return !pointer_; }

  // "Reinterpret casts" to a |UserPointer<ToType>|.
  template <typename ToType>
  UserPointer<ToType> ReinterpretCast() const {
    return UserPointer<ToType>(reinterpret_cast<ToType*>(pointer_));
  }

  // Checks that this pointer points to a valid |Type| in the same way as
  // |Get()| and |Put()|.
  // TODO(vtl): Logically, there should be separate read checks and write
  // checks.
  void Check() const {
    internal::CheckUserPointer<sizeof(NonVoidType), MOJO_ALIGNOF(NonVoidType)>(
        pointer_);
  }

  // Checks that this pointer points to a valid array (of type |Type|, or just a
  // buffer if |Type| is |void| or |const void|) of |count| elements (or bytes
  // if |Type| is |void| or |const void|) in the same way as |GetArray()| and
  // |PutArray()|.
  // TODO(vtl): Logically, there should be separate read checks and write
  // checks.
  // TODO(vtl): Switch more things to use this.
  void CheckArray(size_t count) const {
    internal::CheckUserPointerWithCount<sizeof(NonVoidType),
                                        MOJO_ALIGNOF(NonVoidType)>(pointer_,
                                                                   count);
  }

  // Gets the value (of type |Type|, or a |char| if |Type| is |void|) pointed to
  // by this user pointer. Use this when you'd use the rvalue |*user_pointer|,
  // but be aware that this may be costly -- so if the value will be used
  // multiple times, you should save it.
  //
  // (We want to force a copy here, so return |Type| not |const Type&|.)
  NonVoidType Get() const {
    Check();
    internal::CheckUserPointer<sizeof(NonVoidType), MOJO_ALIGNOF(NonVoidType)>(
        pointer_);
    return *pointer_;
  }

  // Gets an array (of type |Type|, or just a buffer if |Type| is |void| or
  // |const void|) of |count| elements (or bytes if |Type| is |void| or |const
  // void|) from the location pointed to by this user pointer. Use this when
  // you'd do something like |memcpy(destination, user_pointer, count *
  // sizeof(Type)|.
  void GetArray(typename internal::remove_const<Type>::type* destination,
                size_t count) const {
    CheckArray(count);
    memcpy(destination, pointer_, count * sizeof(NonVoidType));
  }

  // Puts a value (of type |Type|, or of type |char| if |Type| is |void|) to the
  // location pointed to by this user pointer. Use this when you'd use the
  // lvalue |*user_pointer|. Since this may be costly, you should avoid using
  // this (for the same user pointer) more than once.
  //
  // Note: This |Put()| method is not valid when |T| is const, e.g., |const
  // uint32_t|, but it's okay to include them so long as this template is only
  // implicitly instantiated (see 14.7.1 of the C++11 standard) and not
  // explicitly instantiated. (On implicit instantiation, only the declarations
  // need be valid, not the definitions.)
  //
  // In C++11, we could do something like:
  //   template <typename _Type = Type>
  //   typename enable_if<!is_const<_Type>::value &&
  //                      !is_void<_Type>::value>::type Put(
  //       const _Type& value) { ... }
  // (which obviously be correct), but C++03 doesn't allow default function
  // template arguments.
  void Put(const NonVoidType& value) {
    Check();
    *pointer_ = value;
  }

  // Puts an array (of type |Type|, or just a buffer if |Type| is |void|) with
  // |count| elements (or bytes |Type| is |void|) to the location pointed to by
  // this user pointer. Use this when you'd do something like
  // |memcpy(user_pointer, source, count * sizeof(Type))|.
  //
  // Note: The same comments about the validity of |Put()| (except for the part
  // about |void|) apply here.
  void PutArray(const Type* source, size_t count) {
    CheckArray(count);
    memcpy(pointer_, source, count * sizeof(NonVoidType));
  }

  // Gets a |UserPointer| at offset |i| (in |Type|s) relative to this.
  UserPointer At(size_t i) const {
    return UserPointer(
        static_cast<Type*>(static_cast<NonVoidType*>(pointer_) + i));
  }

  // Gets the value of the |UserPointer| as a |uintptr_t|. This should not be
  // casted back to a pointer (and dereferenced), but may be used as a key for
  // lookup or passed back to the user.
  uintptr_t GetPointerValue() const {
    return reinterpret_cast<uintptr_t>(pointer_);
  }

  // These provides safe (read-only/write-only/read-and-write) access to a
  // |UserPointer<Type>| (probably pointing to an array) using just an ordinary
  // pointer (obtained via |GetPointer()|).
  //
  // The memory returned by |GetPointer()| may be a copy of the original user
  // memory, but should be modified only if the user is intended to eventually
  // see the change.) If any changes are made, |Commit()| should be called to
  // guarantee that the changes are written back to user memory (it may be
  // called multiple times).
  //
  // Note: These classes are designed to allow fast, unsafe implementations (in
  // which |GetPointer()| just returns the user pointer) if desired. Thus if
  // |Commit()| is *not* called, changes may or may not be made visible to the
  // user.
  //
  // Use these classes in the following way:
  //
  //   MojoResult Core::PutFoos(UserPointer<const uint32_t> foos,
  //                            uint32_t num_foos) {
  //     UserPointer<const uint32_t>::Reader foos_reader(foos, num_foos);
  //     return PutFoosImpl(foos_reader.GetPointer(), num_foos);
  //   }
  //
  //   MojoResult Core::GetFoos(UserPointer<uint32_t> foos,
  //                            uint32_t num_foos) {
  //     UserPointer<uint32_t>::Writer foos_writer(foos, num_foos);
  //     MojoResult rv = GetFoosImpl(foos.GetPointer(), num_foos);
  //     foos_writer.Commit();
  //     return rv;
  //   }
  //
  // TODO(vtl): Possibly, since we're not really being safe, we should just not
  // copy for Release builds.
  using Reader = UserPointerReader<Type>;
  using Writer = UserPointerWriter<Type>;
  using ReaderWriter = UserPointerReaderWriter<Type>;

 private:
  friend class UserPointerReader<Type>;
  friend class UserPointerReader<const Type>;
  friend class UserPointerWriter<Type>;
  friend class UserPointerReaderWriter<Type>;
  template <class Options>
  friend class UserOptionsReader;

  Type* pointer_;
  // Allow copy and assignment.
};

// Provides a convenient way to make a |UserPointer<Type>|.
template <typename Type>
inline UserPointer<Type> MakeUserPointer(Type* pointer) {
  return UserPointer<Type>(pointer);
}

// Implementation of |UserPointer<Type>::Reader|.
template <typename Type>
class UserPointerReader {
 private:
  using TypeNoConst = typename internal::remove_const<Type>::type;

 public:
  // Note: If |count| is zero, |GetPointer()| will always return null.
  UserPointerReader(UserPointer<const Type> user_pointer, size_t count) {
    Init(user_pointer.pointer_, count, true);
  }
  UserPointerReader(UserPointer<TypeNoConst> user_pointer, size_t count) {
    Init(user_pointer.pointer_, count, true);
  }

  const Type* GetPointer() const { return buffer_.get(); }

 private:
  template <class Options>
  friend class UserOptionsReader;

  struct NoCheck {};
  UserPointerReader(NoCheck,
                    UserPointer<const Type> user_pointer,
                    size_t count) {
    Init(user_pointer.pointer_, count, false);
  }

  void Init(const Type* user_pointer, size_t count, bool check) {
    if (count == 0)
      return;

    if (check) {
      internal::CheckUserPointerWithCount<sizeof(Type), MOJO_ALIGNOF(Type)>(
          user_pointer, count);
    }
    buffer_.reset(new TypeNoConst[count]);
    memcpy(buffer_.get(), user_pointer, count * sizeof(Type));
  }

  scoped_ptr<TypeNoConst[]> buffer_;

  DISALLOW_COPY_AND_ASSIGN(UserPointerReader);
};

// Implementation of |UserPointer<Type>::Writer|.
template <typename Type>
class UserPointerWriter {
 public:
  // Note: If |count| is zero, |GetPointer()| will always return null.
  UserPointerWriter(UserPointer<Type> user_pointer, size_t count)
      : user_pointer_(user_pointer), count_(count) {
    if (count_ > 0) {
      buffer_.reset(new Type[count_]);
      memset(buffer_.get(), 0, count_ * sizeof(Type));
    }
  }

  Type* GetPointer() const { return buffer_.get(); }

  void Commit() {
    internal::CheckUserPointerWithCount<sizeof(Type), MOJO_ALIGNOF(Type)>(
        user_pointer_.pointer_, count_);
    memcpy(user_pointer_.pointer_, buffer_.get(), count_ * sizeof(Type));
  }

 private:
  UserPointer<Type> user_pointer_;
  size_t count_;
  scoped_ptr<Type[]> buffer_;

  DISALLOW_COPY_AND_ASSIGN(UserPointerWriter);
};

// Implementation of |UserPointer<Type>::ReaderWriter|.
template <typename Type>
class UserPointerReaderWriter {
 public:
  // Note: If |count| is zero, |GetPointer()| will always return null.
  UserPointerReaderWriter(UserPointer<Type> user_pointer, size_t count)
      : user_pointer_(user_pointer), count_(count) {
    if (count_ > 0) {
      internal::CheckUserPointerWithCount<sizeof(Type), MOJO_ALIGNOF(Type)>(
          user_pointer_.pointer_, count_);
      buffer_.reset(new Type[count]);
      memcpy(buffer_.get(), user_pointer.pointer_, count * sizeof(Type));
    }
  }

  Type* GetPointer() const { return buffer_.get(); }
  size_t GetCount() const { return count_; }

  void Commit() {
    internal::CheckUserPointerWithCount<sizeof(Type), MOJO_ALIGNOF(Type)>(
        user_pointer_.pointer_, count_);
    memcpy(user_pointer_.pointer_, buffer_.get(), count_ * sizeof(Type));
  }

 private:
  UserPointer<Type> user_pointer_;
  size_t count_;
  scoped_ptr<Type[]> buffer_;

  DISALLOW_COPY_AND_ASSIGN(UserPointerReaderWriter);
};

}  // namespace system
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_MEMORY_H_
