//
// Created by veronika on 22.10.23.
//

#include "SequentialConsistencyStorageManager.h"
#include "doctest.h"

using namespace wmm::storage;

TEST_SUITE("Sequential Consistency") {
    using SC::SequentialConsistencyStorageManager;
    TEST_CASE("Load and Store") {
        SequentialConsistencyStorageManager storageManager(10);

        SUBCASE("Load from uninitialized storage should return 0") {
            int32_t result = storageManager.load(0, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 0);
        }
        SUBCASE("Stored value is immediately available to other threads") {
            storageManager.store(0, 0, 42, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 42);
        }
    }

    TEST_CASE("CompareAndSwap") {
        SequentialConsistencyStorageManager storageManager(10);

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
        SequentialConsistencyStorageManager storageManager(10);

        SUBCASE("FetchAndIncrement test") {
            storageManager.store(0, 0, 10, MemoryAccessMode::Relaxed);
            storageManager.fetchAndIncrement(0, 0, 42, MemoryAccessMode::Relaxed);
            int32_t result = storageManager.load(1, 0, MemoryAccessMode::Relaxed);
            CHECK_EQ(result, 52);
        }
    }
}
