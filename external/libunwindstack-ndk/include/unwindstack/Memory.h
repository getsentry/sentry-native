/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIBUNWINDSTACK_MEMORY_H
#define _LIBUNWINDSTACK_MEMORY_H

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace unwindstack {

class Memory {
 public:
  Memory() = default;
  virtual ~Memory() = default;

  static std::shared_ptr<Memory> CreateProcessMemory(pid_t pid);

  virtual bool ReadString(uint64_t addr, std::string* string, uint64_t max_read = UINT64_MAX);

  virtual size_t Read(uint64_t addr, void* dst, size_t size) = 0;

  bool ReadFully(uint64_t addr, void* dst, size_t size);

  inline bool ReadField(uint64_t addr, void* start, void* field, size_t size) {
    if (reinterpret_cast<uintptr_t>(field) < reinterpret_cast<uintptr_t>(start)) {
      return false;
    }
    uint64_t offset = reinterpret_cast<uintptr_t>(field) - reinterpret_cast<uintptr_t>(start);
    if (__builtin_add_overflow(addr, offset, &offset)) {
      return false;
    }
    // The read will check if offset + size overflows.
    return ReadFully(offset, field, size);
  }

  inline bool Read32(uint64_t addr, uint32_t* dst) {
    return ReadFully(addr, dst, sizeof(uint32_t));
  }

  inline bool Read64(uint64_t addr, uint64_t* dst) {
    return ReadFully(addr, dst, sizeof(uint64_t));
  }
};

class MemoryBuffer : public Memory {
 public:
  MemoryBuffer() = default;
  virtual ~MemoryBuffer() = default;

  size_t Read(uint64_t addr, void* dst, size_t size) override;

  uint8_t* GetPtr(size_t offset);

  void Resize(size_t size) { raw_.resize(size); }

  uint64_t Size() { return raw_.size(); }

 private:
  std::vector<uint8_t> raw_;
};

class MemoryFileAtOffset : public Memory {
 public:
  MemoryFileAtOffset() = default;
  virtual ~MemoryFileAtOffset();

  bool Init(const std::string& file, uint64_t offset, uint64_t size = UINT64_MAX);

  size_t Read(uint64_t addr, void* dst, size_t size) override;

  size_t Size() { return size_; }

  void Clear();

 protected:
  size_t size_ = 0;
  size_t offset_ = 0;
  uint8_t* data_ = nullptr;
};

class MemoryRemote : public Memory {
 public:
  MemoryRemote(pid_t pid) : pid_(pid), read_redirect_func_(0) {}
  virtual ~MemoryRemote() = default;

  size_t Read(uint64_t addr, void* dst, size_t size) override;

  pid_t pid() { return pid_; }

 private:
  pid_t pid_;
  std::atomic_uintptr_t read_redirect_func_;
};

class MemoryLocal : public Memory {
 public:
  MemoryLocal() = default;
  virtual ~MemoryLocal() = default;

  size_t Read(uint64_t addr, void* dst, size_t size) override;
};

// MemoryRange maps one address range onto another.
// The range [src_begin, src_begin + length) in the underlying Memory is mapped onto offset,
// such that range.read(offset) is equivalent to underlying.read(src_begin).
class MemoryRange : public Memory {
 public:
  MemoryRange(const std::shared_ptr<Memory>& memory, uint64_t begin, uint64_t length,
              uint64_t offset);
  virtual ~MemoryRange() = default;

  size_t Read(uint64_t addr, void* dst, size_t size) override;

 private:
  std::shared_ptr<Memory> memory_;
  uint64_t begin_;
  uint64_t length_;
  uint64_t offset_;
};

class MemoryOffline : public Memory {
 public:
  MemoryOffline() = default;
  virtual ~MemoryOffline() = default;

  bool Init(const std::string& file, uint64_t offset);

  size_t Read(uint64_t addr, void* dst, size_t size) override;

 private:
  std::unique_ptr<MemoryRange> memory_;
};

class MemoryOfflineParts : public Memory {
 public:
  MemoryOfflineParts() = default;
  virtual ~MemoryOfflineParts();

  void Add(MemoryOffline* memory) { memories_.push_back(memory); }

  size_t Read(uint64_t addr, void* dst, size_t size) override;

 private:
  std::vector<MemoryOffline*> memories_;
};

}  // namespace unwindstack

#endif  // _LIBUNWINDSTACK_MEMORY_H
