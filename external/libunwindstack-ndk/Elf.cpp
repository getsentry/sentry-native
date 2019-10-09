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
#include <string.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#define LOG_TAG "unwind"
#include <android-base/log_main.h>
#include <linux/elf-em.h>

#include <unwindstack/Elf.h>
#include <unwindstack/ElfInterface.h>
#include <unwindstack/MapInfo.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>

#include "ElfInterfaceArm.h"
#include "Symbols.h"

namespace unwindstack {

bool Elf::cache_enabled_;
std::unordered_map<std::string, std::pair<std::shared_ptr<Elf>, bool>>* Elf::cache_;
std::mutex* Elf::cache_lock_;

bool Elf::Init(bool init_gnu_debugdata) {
  load_bias_ = 0;
  if (!memory_) {
    return false;
  }

  interface_.reset(CreateInterfaceFromMemory(memory_.get()));
  if (!interface_) {
    return false;
  }

  valid_ = interface_->Init(&load_bias_);
  if (valid_) {
    interface_->InitHeaders();
    gnu_debugdata_interface_.reset(nullptr);
  } else {
    interface_.reset(nullptr);
  }
  return valid_;
}

uint64_t Elf::GetRelPc(uint64_t pc, const MapInfo* map_info) {
  return pc - map_info->start + load_bias_ + map_info->elf_offset;
}

bool Elf::GetFunctionName(uint64_t addr, std::string* name, uint64_t* func_offset) {
  std::lock_guard<std::mutex> guard(lock_);
  return valid_ && (interface_->GetFunctionName(addr, load_bias_, name, func_offset) ||
                    (gnu_debugdata_interface_ && gnu_debugdata_interface_->GetFunctionName(
                                                     addr, load_bias_, name, func_offset)));
}

// The relative pc is always relative to the start of the map from which it comes.
bool Elf::Step(uint64_t rel_pc, uint64_t adjusted_rel_pc, uint64_t elf_offset, Regs* regs,
               Memory* process_memory, bool* finished) {
  if (!valid_) {
    return false;
  }

  // The relative pc expectd by StepIfSignalHandler is relative to the start of the elf.
  if (regs->StepIfSignalHandler(rel_pc + elf_offset, this, process_memory)) {
    *finished = false;
    return true;
  }

  // Lock during the step which can update information in the object.
  std::lock_guard<std::mutex> guard(lock_);
  return interface_->Step(adjusted_rel_pc, load_bias_, regs, process_memory, finished);
}

bool Elf::IsValidElf(Memory* memory) {
  if (memory == nullptr) {
    return false;
  }

  // Verify that this is a valid elf file.
  uint8_t e_ident[SELFMAG + 1];
  if (!memory->ReadFully(0, e_ident, SELFMAG)) {
    return false;
  }

  if (memcmp(e_ident, ELFMAG, SELFMAG) != 0) {
    return false;
  }
  return true;
}

void Elf::GetInfo(Memory* memory, bool* valid, uint64_t* size) {
  if (!IsValidElf(memory)) {
    *valid = false;
    return;
  }
  *size = 0;
  *valid = true;

  // Now read the section header information.
  uint8_t class_type;
  if (!memory->ReadFully(EI_CLASS, &class_type, 1)) {
    return;
  }
  if (class_type == ELFCLASS32) {
    ElfInterface32::GetMaxSize(memory, size);
  } else if (class_type == ELFCLASS64) {
    ElfInterface64::GetMaxSize(memory, size);
  } else {
    *valid = false;
  }
}

ElfInterface* Elf::CreateInterfaceFromMemory(Memory* memory) {
  if (!IsValidElf(memory)) {
    return nullptr;
  }

  std::unique_ptr<ElfInterface> interface;
  if (!memory->ReadFully(EI_CLASS, &class_type_, 1)) {
    return nullptr;
  }
  if (class_type_ == ELFCLASS32) {
    Elf32_Half e_machine;
    if (!memory->ReadFully(EI_NIDENT + sizeof(Elf32_Half), &e_machine, sizeof(e_machine))) {
      return nullptr;
    }

    machine_type_ = e_machine;
    if (e_machine == EM_ARM) {
      arch_ = ARCH_ARM;
      interface.reset(new ElfInterfaceArm(memory));
    } else if (e_machine == EM_386) {
      arch_ = ARCH_X86;
      interface.reset(new ElfInterface32(memory));
    } else {
      // Unsupported.
      ALOGI("32 bit elf that is neither arm nor x86 nor mips: e_machine = %d\n", e_machine);
      return nullptr;
    }
  } else if (class_type_ == ELFCLASS64) {
    Elf64_Half e_machine;
    if (!memory->ReadFully(EI_NIDENT + sizeof(Elf64_Half), &e_machine, sizeof(e_machine))) {
      return nullptr;
    }

    machine_type_ = e_machine;
    if (e_machine == EM_AARCH64) {
      arch_ = ARCH_ARM64;
    } else if (e_machine == EM_X86_64) {
      arch_ = ARCH_X86_64;
    } else {
      // Unsupported.
      ALOGI("64 bit elf that is neither aarch64 nor x86_64 nor mips64: e_machine = %d\n",
            e_machine);
      return nullptr;
    }
    interface.reset(new ElfInterface64(memory));
  }

  return interface.release();
}

uint64_t Elf::GetLoadBias(Memory* memory) {
  if (!IsValidElf(memory)) {
    return 0;
  }

  uint8_t class_type;
  if (!memory->Read(EI_CLASS, &class_type, 1)) {
    return 0;
  }

  if (class_type == ELFCLASS32) {
    return ElfInterface::GetLoadBias<Elf32_Ehdr, Elf32_Phdr>(memory);
  } else if (class_type == ELFCLASS64) {
    return ElfInterface::GetLoadBias<Elf64_Ehdr, Elf64_Phdr>(memory);
  }
  return 0;
}

void Elf::CacheLock() {
  cache_lock_->lock();
}

void Elf::CacheUnlock() {
  cache_lock_->unlock();
}

void Elf::CacheAdd(MapInfo* info) {
  // If elf_offset != 0, then cache both name:offset and name.
  // The cached name is used to do lookups if multiple maps for the same
  // named elf file exist.
  // For example, if there are two maps boot.odex:1000 and boot.odex:2000
  // where each reference the entire boot.odex, the cache will properly
  // use the same cached elf object.

  if (info->offset == 0 || info->elf_offset != 0) {
    (*cache_)[info->name] = std::make_pair(info->elf, true);
  }

  if (info->offset != 0) {
    // The second element in the pair indicates whether elf_offset should
    // be set to offset when getting out of the cache.
    (*cache_)[info->name + ':' + std::to_string(info->offset)] =
        std::make_pair(info->elf, info->elf_offset != 0);
  }
}

bool Elf::CacheAfterCreateMemory(MapInfo* info) {
  if (info->name.empty() || info->offset == 0 || info->elf_offset == 0) {
    return false;
  }

  auto entry = cache_->find(info->name);
  if (entry == cache_->end()) {
    return false;
  }

  // In this case, the whole file is the elf, and the name has already
  // been cached. Add an entry at name:offset to get this directly out
  // of the cache next time.
  info->elf = entry->second.first;
  (*cache_)[info->name + ':' + std::to_string(info->offset)] = std::make_pair(info->elf, true);
  return true;
}

bool Elf::CacheGet(MapInfo* info) {
  std::string name(info->name);
  if (info->offset != 0) {
    name += ':' + std::to_string(info->offset);
  }
  auto entry = cache_->find(name);
  if (entry != cache_->end()) {
    info->elf = entry->second.first;
    if (entry->second.second) {
      info->elf_offset = info->offset;
    }
    return true;
  }
  return false;
}

}  // namespace unwindstack
