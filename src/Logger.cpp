//
// Created by veronika on 22.10.23.
//

#include <format>

#include "Logger.h"
#include "StorageManager.h"

namespace wmm::storage {
void Logger::storage(const storage::StorageManagerInterface &storage) {
    storage.writeStorage(m_outputStream.get());
}

void Logger::error(const std::string &log)  {
    if (m_logLevel <= LogLevel::ERROR) {
        m_outputStream.get() << "ERROR: " << log << std::endl;
    }
}

void Logger::warning(const std::string &log) {
    if (m_logLevel <= LogLevel::WARNING) {
        m_outputStream.get() << "WARN: " << log << std::endl;
    }
}

void Logger::info(const std::string &log)  {
    if (m_logLevel <= LogLevel::INFO) {
        m_outputStream.get() << "INFO: " << log << std::endl;
    }
}

void Logger::load(size_t threadId, size_t address,
                     MemoryAccessMode accessMode) {
    info(std::format("t{} at #{}: {} load", threadId,
                     address, toString(accessMode)));
};

void Logger::store(size_t threadId, size_t address, int32_t value,
                      MemoryAccessMode accessMode) {
    info(std::format("t{} at #{}: {} store {}", threadId,
                     address, toString(accessMode),
                     value));
}

void Logger::compareAndSwap(size_t threadId, size_t address,
                               int32_t expectedValue, int32_t newValue,
                               MemoryAccessMode accessMode) {
    info(
            std::format("t{} at #{}: {} if {} store {}", threadId, address,
                        toString(accessMode), expectedValue, newValue));
}

void Logger::fetchAndIncrement(size_t threadId, size_t address,
                                  int32_t increment,
                                  MemoryAccessMode accessMode) {
    info(std::format("t{} at #{}: {} +{}", threadId,
                     address, toString(accessMode),
                     increment));
}
void Logger::fence(size_t threadId, MemoryAccessMode accessMode) {
    info(
            std::format("t{}: {} fence", threadId, toString(accessMode)));
}

} // namespace wmm