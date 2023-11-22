//
// Created by veronika on 14.11.23.
//

#pragma once

#include "StorageManager.h"
#include <unordered_map>

namespace wmm::storage::RA {

class MessageView {
private:
    std::unordered_map<size_t, size_t> timestamps;
public:
    MessageView &operator|=(const MessageView &other);

    friend MessageView operator|(const MessageView &lhs, const MessageView &rhs);

    MessageView &operator|=(std::initializer_list<std::pair<size_t, size_t>> &&pairs);

    friend MessageView operator|(const MessageView &lhs, std::pair<size_t, size_t> &&pair);

    size_t getTimestamp(size_t location) const;
};

struct Message {
    const size_t location;
    const size_t value;
    const size_t timestamp;
    
};

class StrongReleaseAcquireStorageManager : public StorageManagerInterface {
private:
    std::vector<MessageView> m_messageViews;

public:
    explicit StrongReleaseAcquireStorageManager(
            size_t storageSize, size_t nOfThreads,
            storage::LoggerPtr &&logger = std::make_unique<FakeStorageLogger>())
        : StorageManagerInterface(std::move(logger)), m_messageViews(nOfThreads) {}

    int32_t load(size_t threadId, size_t address,
                 MemoryAccessMode accessMode) override;

    void store(size_t threadId, size_t address, int32_t value,
               MemoryAccessMode accessMode) override;

    void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue,
                        int32_t newValue, MemoryAccessMode accessMode) override;

    void fetchAndIncrement(size_t threadId, size_t address, int32_t increment,
                           MemoryAccessMode accessMode) override;

    void fence(size_t threadId, MemoryAccessMode accessMode) override;

    void writeStorage(std::ostream &outputStream) const override;

};

} // namespace wmm::storage::RA
