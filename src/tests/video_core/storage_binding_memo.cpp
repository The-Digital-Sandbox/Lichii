// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/buffer_cache/storage_binding_memo.h"

namespace {

struct TestBinding {
    u64 device_addr{};
    u64 gpu_addr{};
    u32 size{};
    bool operator==(const TestBinding&) const = default;
};

VideoCommon::StorageBindingKey BaseKey() {
    return VideoCommon::StorageBindingKey{
        .ssbo_addr = 0x1000,
        .holder = 0,
        .pointer_word = 0x8000'0000,
        .size_word = 0x100,
        .map_generation = 7,
        .flags = 1,
    };
}

} // Anonymous namespace

TEST_CASE("Storage binding memo misses until stored", "[video_core][storage_bindings]") {
    VideoCommon::StorageBindingMemo<TestBinding> memo;
    REQUIRE(memo.Find(BaseKey()) == nullptr);
}

TEST_CASE("Storage binding memo returns the binding for the same key",
          "[video_core][storage_bindings]") {
    VideoCommon::StorageBindingMemo<TestBinding> memo;
    const TestBinding binding{.device_addr = 0x4000, .gpu_addr = 0x8000'0000, .size = 0x100};
    memo.Store(BaseKey(), binding);

    const TestBinding* found = memo.Find(BaseKey());
    REQUIRE(found != nullptr);
    REQUIRE(*found == binding);
}

TEST_CASE("Storage binding memo misses when any input changed", "[video_core][storage_bindings]") {
    VideoCommon::StorageBindingMemo<TestBinding> memo;
    memo.Store(BaseKey(), TestBinding{.size = 1});

    SECTION("pointer word") {
        auto key = BaseKey();
        key.pointer_word += 0x1000;
        REQUIRE(memo.Find(key) == nullptr);
    }
    SECTION("size word") {
        auto key = BaseKey();
        key.size_word = 0x200;
        REQUIRE(memo.Find(key) == nullptr);
    }
    SECTION("table entry") {
        auto key = BaseKey();
        key.holder = 0x2000;
        REQUIRE(memo.Find(key) == nullptr);
    }
    SECTION("pointer address") {
        auto key = BaseKey();
        key.ssbo_addr += 8;
        REQUIRE(memo.Find(key) == nullptr);
    }
    SECTION("GPU page table") {
        auto key = BaseKey();
        key.map_generation += 1;
        REQUIRE(memo.Find(key) == nullptr);
    }
    SECTION("descriptor flags") {
        auto key = BaseKey();
        key.flags = 0;
        REQUIRE(memo.Find(key) == nullptr);
    }
}

TEST_CASE("Storage binding memo keeps only the latest binding", "[video_core][storage_bindings]") {
    VideoCommon::StorageBindingMemo<TestBinding> memo;
    memo.Store(BaseKey(), TestBinding{.size = 1});

    auto other = BaseKey();
    other.pointer_word = 0x9000'0000;
    memo.Store(other, TestBinding{.size = 2});

    REQUIRE(memo.Find(BaseKey()) == nullptr);
    const TestBinding* found = memo.Find(other);
    REQUIRE(found != nullptr);
    REQUIRE(found->size == 2);

    memo.Clear();
    REQUIRE(memo.Find(other) == nullptr);
}
