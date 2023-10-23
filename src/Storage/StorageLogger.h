//
// Created by veronika on 22.10.23.
//

#pragma once

#include <memory>
#include <ostream>

#include "StorageMemoryAccessMode.h"

namespace wmm::storage {

class StorageManagerInterface;

enum class LogLevel : int { INFO = 0, WARNING, ERROR };

static inline bool operator<=(LogLevel lhs, LogLevel rhs) {
    return static_cast<int>(lhs) <= static_cast<int>(rhs);
}

class StorageLogger {
public:
    virtual void info(const std::string &log) = 0;
    virtual void warning(const std::string &log) = 0;
    virtual void error(const std::string &log) = 0;
    virtual void storage(const StorageManagerInterface &storage) = 0;

    void load(size_t threadId, size_t address, MemoryAccessMode accessMode,
              int32_t result);

    void store(size_t threadId, size_t address, int32_t value,
               MemoryAccessMode accessMode);
    void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue,
                        int32_t realValue, int32_t newValue,
                        MemoryAccessMode accessMode);
    void fetchAndIncrement(size_t threadId, size_t address, int32_t increment,
                           MemoryAccessMode accessMode);

    void fence(size_t threadId, MemoryAccessMode accessMode);

    virtual ~StorageLogger() = default;
};

using LoggerPtr = std::unique_ptr<StorageLogger>;

class StorageLoggerImpl : public StorageLogger {
    std::reference_wrapper<std::ostream> m_outputStream;
    const LogLevel m_logLevel;

public:
    explicit StorageLoggerImpl(std::ostream &outputStream,
                               LogLevel logLevel = LogLevel::INFO)
        : m_outputStream(outputStream), m_logLevel(logLevel) {}

    void info(const std::string &log) override;
    void warning(const std::string &log) override;
    void error(const std::string &log) override;
    void storage(const StorageManagerInterface &storage) override;
};

class FakeStorageLogger : public StorageLogger {
public:
    FakeStorageLogger() = default;

    void info(const std::string &log) override {}
    void warning(const std::string &log) override {}
    void error(const std::string &log) override {}
    void storage(const StorageManagerInterface &storage) override {}
};

} // namespace wmm::storage
