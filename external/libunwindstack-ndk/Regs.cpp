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

#include <linux/elf.h>
#include <stdint.h>
#include <sys/ptrace.h>
#define PTRACE_GETREGSET 0x4204
#define PTRACE_GETREGS 12
#include <sys/uio.h>

#include <vector>

#include <unwindstack/Elf.h>
#include <unwindstack/MapInfo.h>
#include <unwindstack/Regs.h>
#include <unwindstack/RegsArm.h>
#include <unwindstack/RegsArm64.h>
#include <unwindstack/RegsX86.h>
#include <unwindstack/RegsX86_64.h>
#include <unwindstack/UserArm.h>
#include <unwindstack/UserArm64.h>
#include <unwindstack/UserX86.h>
#include <unwindstack/UserX86_64.h>

namespace unwindstack {

// The largest user structure.
constexpr size_t MAX_USER_REGS_SIZE = sizeof(arm64_user_regs) + 10;

// This function assumes that reg_data is already aligned to a 64 bit value.
// If not this could crash with an unaligned access.
Regs* Regs::RemoteGet(pid_t pid) {
  // Make the buffer large enough to contain the largest registers type.
  std::vector<uint64_t> buffer(MAX_USER_REGS_SIZE / sizeof(uint64_t));
  struct iovec io;
  io.iov_base = buffer.data();
  io.iov_len = buffer.size() * sizeof(uint64_t);

  if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, reinterpret_cast<void*>(&io)) == -1) {
    // Falling back to PTRACE_GETREGS.
    if (ptrace(PTRACE_GETREGS, pid, 0, io.iov_base) == -1) {
      return nullptr;
    }
  }
  // Assuming we can't unwind an architecture other than current.
  switch (Regs::CurrentArch()) {
  case ARCH_X86:
    return RegsX86::Read(buffer.data());
  case ARCH_X86_64:
    return RegsX86_64::Read(buffer.data());
  case ARCH_ARM:
    return RegsArm::Read(buffer.data());
  case ARCH_ARM64:
    return RegsArm64::Read(buffer.data());
  default:
    return nullptr;
  }
}

Regs* Regs::CreateFromUcontext(ArchEnum arch, void* ucontext) {
  switch (arch) {
    case ARCH_X86:
      return RegsX86::CreateFromUcontext(ucontext);
    case ARCH_X86_64:
      return RegsX86_64::CreateFromUcontext(ucontext);
    case ARCH_ARM:
      return RegsArm::CreateFromUcontext(ucontext);
    case ARCH_ARM64:
      return RegsArm64::CreateFromUcontext(ucontext);
    case ARCH_UNKNOWN:
    default:
      return nullptr;
  }
}

ArchEnum Regs::CurrentArch() {
#if defined(__arm__)
  return ARCH_ARM;
#elif defined(__aarch64__)
  return ARCH_ARM64;
#elif defined(__i386__)
  return ARCH_X86;
#elif defined(__x86_64__)
  return ARCH_X86_64;
#else
  abort();
#endif
}

Regs* Regs::CreateFromLocal() {
  Regs* regs;
#if defined(__arm__)
  regs = new RegsArm();
#elif defined(__aarch64__)
  regs = new RegsArm64();
#elif defined(__i386__)
  regs = new RegsX86();
#elif defined(__x86_64__)
  regs = new RegsX86_64();
#else
  abort();
#endif
  return regs;
}

}  // namespace unwindstack
