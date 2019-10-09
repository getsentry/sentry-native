/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define _GNU_SOURCE 1
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <unwindstack/Elf.h>
#include <unwindstack/JitDebug.h>
#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>
#include <unwindstack/Unwinder.h>

#include <android-base/stringprintf.h>

struct map_info_t {
  uint64_t start;
  uint64_t end;
  uint64_t offset;
  std::string name;
};

static bool Attach(pid_t pid) {
  if (ptrace(PTRACE_ATTACH, pid, 0, 0) == -1) {
    return false;
  }

  // Allow at least 1 second to attach properly.
  for (size_t i = 0; i < 1000; i++) {
    siginfo_t si;
    if (ptrace(PTRACE_GETSIGINFO, pid, 0, &si) == 0) {
      return true;
    }
    usleep(1000);
  }
  printf("%d: Failed to stop.\n", pid);
  return false;
}

bool SaveRegs(unwindstack::Regs* regs) {
  std::unique_ptr<FILE, decltype(&fclose)> fp(fopen("regs.txt", "w+"), &fclose);
  if (fp == nullptr) {
    printf("Failed to create file regs.txt.\n");
    return false;
  }
  regs->IterateRegisters([&fp](const char* name, uint64_t value) {
    fprintf(fp.get(), "%s: %" PRIx64 "\n", name, value);
  });

  return true;
}

bool SaveStack(pid_t pid, uint64_t sp_start, uint64_t sp_end) {
  std::unique_ptr<FILE, decltype(&fclose)> fp(fopen("stack.data", "w+"), &fclose);
  if (fp == nullptr) {
    printf("Failed to create stack.data.\n");
    return false;
  }

  size_t bytes = fwrite(&sp_start, 1, sizeof(sp_start), fp.get());
  if (bytes != sizeof(sp_start)) {
    perror("Failed to write all data.");
    return false;
  }

  std::vector<uint8_t> buffer(sp_end - sp_start);
  auto process_memory = unwindstack::Memory::CreateProcessMemory(pid);
  if (!process_memory->Read(sp_start, buffer.data(), buffer.size())) {
    printf("Unable to read stack data.\n");
    return false;
  }

  bytes = fwrite(buffer.data(), 1, buffer.size(), fp.get());
  if (bytes != buffer.size()) {
    printf("Failed to write all stack data: stack size %zu, written %zu\n", buffer.size(), bytes);
    return 1;
  }

  return true;
}

bool CreateElfFromMemory(std::shared_ptr<unwindstack::Memory>& memory, map_info_t* info) {
  std::string cur_name;
  if (info->name.empty()) {
    cur_name = android::base::StringPrintf("anonymous:%" PRIx64, info->start);
  } else {
    cur_name = basename(info->name.c_str());
    cur_name = android::base::StringPrintf("%s:%" PRIx64, basename(info->name.c_str()), info->start);
  }

  std::unique_ptr<FILE, decltype(&fclose)> output(fopen(cur_name.c_str(), "w+"), &fclose);
  if (output == nullptr) {
    printf("Cannot create %s\n", cur_name.c_str());
    return false;
  }
  std::vector<uint8_t> buffer(info->end - info->start);
  // If this is a mapped in file, it might not be possible to read the entire
  // map, so read all that is readable.
  size_t bytes = memory->Read(info->start, buffer.data(), buffer.size());
  if (bytes == 0) {
    printf("Cannot read data from address %" PRIx64 " length %zu\n", info->start, buffer.size());
    return false;
  }
  size_t bytes_written = fwrite(buffer.data(), 1, bytes, output.get());
  if (bytes_written != bytes) {
    printf("Failed to write all data to file: bytes read %zu, written %zu\n", bytes, bytes_written);
    return false;
  }

  // Replace the name with the new name.
  info->name = cur_name;

  return true;
}

bool CopyElfFromFile(map_info_t* info) {
  std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(info->name.c_str(), "r"), &fclose);
  if (fp == nullptr) {
    return false;
  }

  std::string cur_name = basename(info->name.c_str());
  std::unique_ptr<FILE, decltype(&fclose)> output(fopen(cur_name.c_str(), "w+"), &fclose);
  if (output == nullptr) {
    printf("Cannot create file %s\n", cur_name.c_str());
    return false;
  }
  std::vector<uint8_t> buffer(10000);
  size_t bytes;
  while ((bytes = fread(buffer.data(), 1, buffer.size(), fp.get())) > 0) {
    size_t bytes_written = fwrite(buffer.data(), 1, bytes, output.get());
    if (bytes_written != bytes) {
      printf("Bytes written doesn't match bytes read: read %zu, written %zu\n", bytes,
             bytes_written);
      return false;
    }
  }

  // Replace the name with the new name.
  info->name = cur_name;

  return true;
}

int SaveData(pid_t pid) {
  unwindstack::Regs* regs = unwindstack::Regs::RemoteGet(pid);
  if (regs == nullptr) {
    printf("Unable to get remote reg data.\n");
    return 1;
  }

  unwindstack::RemoteMaps maps(pid);
  if (!maps.Parse()) {
    printf("Unable to parse maps.\n");
    return 1;
  }

  // Save the current state of the registers.
  if (!SaveRegs(regs)) {
    return 1;
  }

  // Do an unwind so we know how much of the stack to save, and what
  // elf files are involved.
  uint64_t sp = regs->sp();
  auto process_memory = unwindstack::Memory::CreateProcessMemory(pid);
  unwindstack::JitDebug jit_debug(process_memory);
  unwindstack::Unwinder unwinder(1024, &maps, regs, process_memory);
  unwinder.SetJitDebug(&jit_debug, regs->Arch());
  unwinder.Unwind();

  std::unordered_map<uint64_t, map_info_t> maps_by_start;
  uint64_t last_sp;
  for (auto frame : unwinder.frames()) {
    last_sp = frame.sp;
    if (maps_by_start.count(frame.map_start) == 0) {
      auto info = &maps_by_start[frame.map_start];
      info->start = frame.map_start;
      info->end = frame.map_end;
      info->offset = frame.map_offset;
      info->name = frame.map_name;
      if (!CopyElfFromFile(info)) {
        // Try to create the elf from memory, this will handle cases where
        // the data only exists in memory such as vdso data on x86.
        if (!CreateElfFromMemory(process_memory, info)) {
          return 1;
        }
      }
    }
  }

  for (size_t i = 0; i < unwinder.NumFrames(); i++) {
    printf("%s\n", unwinder.FormatFrame(i).c_str());
  }

  if (!SaveStack(pid, sp, last_sp)) {
    return 1;
  }

  std::vector<std::pair<uint64_t, map_info_t>> sorted_maps(maps_by_start.begin(),
                                                           maps_by_start.end());
  std::sort(sorted_maps.begin(), sorted_maps.end(),
            [](auto& a, auto& b) { return a.first < b.first; });

  std::unique_ptr<FILE, decltype(&fclose)> fp(fopen("maps.txt", "w+"), &fclose);
  if (fp == nullptr) {
    printf("Failed to create maps.txt.\n");
    return false;
  }

  for (auto& element : sorted_maps) {
    map_info_t& map = element.second;
    fprintf(fp.get(), "%" PRIx64 "-%" PRIx64 " r-xp %" PRIx64 " 00:00 0", map.start, map.end,
            map.offset);
    if (!map.name.empty()) {
      fprintf(fp.get(), "   %s", map.name.c_str());
    }
    fprintf(fp.get(), "\n");
  }

  return 0;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: unwind_for_offline <PID>\n");
    return 1;
  }

  pid_t pid = atoi(argv[1]);
  if (!Attach(pid)) {
    printf("Failed to attach to pid %d: %s\n", pid, strerror(errno));
    return 1;
  }

  int return_code = SaveData(pid);

  ptrace(PTRACE_DETACH, pid, 0, 0);

  return return_code;
}
