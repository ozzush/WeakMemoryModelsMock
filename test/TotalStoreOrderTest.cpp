//
// Created by veronika on 22.10.23.
//

#include "TotalStoreOrderStorageManager.h"
#include "doctest.h"

using namespace wmm::storage;

TEST_SUITE("Total Store Order") {
    // TODO: add test that differentiates TSO and PSO
    using TSO::InternalUpdateManagerPtr;
    using TSO::SequentialInternalUpdateManager;
    using TSO::TotalStoreOrderStorageManager;

    TEST_CASE("Load and Store") {
        InternalUpdateManagerPtr internalUpdateManager(new SequentialInternalUpdateManager());
        TotalStoreOrderStorageManager storageManager(10, 2, std::move(internalUpdateManager));

        SUBCASE("Load from uninitialized storage should return 0") {
            int32_t result = storageManager.load(0, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 0);
        }
        SUBCASE("Load from a previously stored value should return the stored value") {
            storageManager.store(0, 0, 41, MemoryAccessMode::Relaxed);
            storageManager.store(0, 0, 42, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(0, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 42);
        }
        SUBCASE("Load by another thread doesn't see the value before buffer is flushed") {
            storageManager.store(0, 0, 42, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            REQUIRE_EQ(result, 0);
            storageManager.internalUpdate();
            result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 42);
        }
    }

    TEST_CASE("CompareAndSwap") {
        InternalUpdateManagerPtr internalUpdateManager(new SequentialInternalUpdateManager());
        TotalStoreOrderStorageManager storageManager(10, 2, std::move(internalUpdateManager));

        storageManager.store(0, 0, 42, MemoryAccessMode::Relaxed);
        SUBCASE("CompareAndSwap should succeed with the expected value") {
            storageManager.compareAndSwap(0, 0, 42, 43, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 43);
        }
        SUBCASE("CompareAndSwap should fail with the wrong expected value") {
            storageManager.compareAndSwap(0, 0, 0, 43, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 42);
        }
    }

    TEST_CASE("FetchAndIncrement") {
        InternalUpdateManagerPtr internalUpdateManager(new SequentialInternalUpdateManager());
        TotalStoreOrderStorageManager storageManager(10, 2, std::move(internalUpdateManager));

        SUBCASE("FetchAndIncrement doesn't ignore buffer") {
            storageManager.store(0, 0, 10, MemoryAccessMode::Relaxed);
            storageManager.fetchAndIncrement(0, 0, 42, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 52);
        }
        SUBCASE("FetchAndIncrement writes through buffer") {
            storageManager.fetchAndIncrement(0, 0, 42, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 42);
        }
    }

    TEST_CASE("Fence") {
        InternalUpdateManagerPtr internalUpdateManager(new SequentialInternalUpdateManager());
        TotalStoreOrderStorageManager storageManager(10, 2, std::move(internalUpdateManager));

        SUBCASE("Fence flushes buffer") {
            storageManager.store(0, 0, 10, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            REQUIRE_EQ(result, 0);
            storageManager.fence(0, MemoryAccessMode::Relaxed);
            result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 10);
        }
        SUBCASE("Fence only flushes one buffer") {
            storageManager.store(0, 0, 10, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            REQUIRE_EQ(result, 0);
            storageManager.fence(1, MemoryAccessMode::Relaxed);
            result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 0);
        }
    }
}
