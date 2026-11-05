// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>

#include "common/common_types.h"

namespace VideoCommon {

/// Everything StorageBufferBinding's result depends on. The pointer and size words are the
/// guest memory it reads; map_generation covers the GPU page table (layout size, continuous
/// range and device address all come from it); flags are the descriptor's.
struct StorageBindingKey {
    u64 ssbo_addr{};      ///< GPU address of the pointer word, after any table indirection
    u64 holder{};         ///< The table entry the pointer was read through, 0 when direct
    u64 pointer_word{};   ///< The u64 at ssbo_addr
    u64 size_word{};      ///< The u64 at ssbo_addr + 8 when the binding reads it, else 0
    u64 map_generation{}; ///< Tegra::MemoryManager::MapGeneration() when the words were read
    u32 flags{};          ///< is_written | native_pointer << 1 | table_pointer << 2

    bool operator==(const StorageBindingKey&) const = default;
};

/// EDEN_NX2_SSBO_MEMO=1 (default off): a storage buffer binding is recomputed only when one of
/// its inputs changed since the last draw that bound the slot. StorageBufferBinding otherwise
/// takes the memory manager's lock and walks the page table about five times per binding per
/// draw, for a result that only changes with the pointer word or a map.
inline bool StorageBindingMemoEnabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("EDEN_NX2_SSBO_MEMO");
        return value != nullptr && value[0] != '0';
    }();
    return enabled;
}

/// The last binding computed for one storage buffer slot, with the key it was computed from.
template <typename Binding>
class StorageBindingMemo {
public:
    /// The stored binding when `key` matches the stored key, else nullptr.
    [[nodiscard]] const Binding* Find(const StorageBindingKey& key) const noexcept {
        return valid && stored_key == key ? &binding : nullptr;
    }

    void Store(const StorageBindingKey& key, const Binding& value) noexcept {
        stored_key = key;
        binding = value;
        valid = true;
    }

    void Clear() noexcept {
        valid = false;
    }

private:
    StorageBindingKey stored_key{};
    Binding binding{};
    bool valid{};
};

} // namespace VideoCommon
