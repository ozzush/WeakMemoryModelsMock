//
// Created by veronika on 22.10.23.
//

#include <format>

#include "StorageLogger.h"
#include "StorageManager.h"

namespace wmm::storage {
void StorageLoggerImpl::storage(
        const storage::StorageManagerInterface &storage) {
    storage.writeStorage(m_outputStream.get());
}

void StorageLoggerImpl::error(const std::string &log) {
    if (m_logLevel >= LogLevel::ERROR) {
        m_outputStream.get() << "ERROR: " << log << std::endl;
    }
}

void StorageLoggerImpl::warning(const std::string &log) {
    if (m_logLevel >= LogLevel::WARNING) {
        m_outputStream.get() << "WARN: " << log << std::endl;
    }
}

void StorageLoggerImpl::info(const std::string &log) {
    if (m_logLevel >= LogLevel::INFO) {
        m_outputStream.get() << log << std::endl;
    }
}

void StorageLogger::load(size_t threadId, size_t address,
                         MemoryAccessMode accessMode, int32_t result) {
    info(std::format("ACTION: t{}#{}: {} load ({})", threadId, address,
                     toString(accessMode), result));
};

void StorageLogger::store(size_t threadId, size_t address, int32_t value,
                          MemoryAccessMode accessMode) {
    info(std::format("ACTION: t{}#{}: {} store {}", threadId, address,
                     toString(accessMode), value));
}

void StorageLogger::compareAndSwap(size_t threadId, size_t address,
                                   int32_t expectedValue, int32_t realValue,
                                   int32_t newValue,
                                   MemoryAccessMode accessMode) {
    info(std::format("ACTION: t{}#{}: {} if ({})=={} store {}", threadId,
                     address, toString(accessMode), realValue, expectedValue,
                     newValue));
}

void StorageLogger::fetchAndIncrement(size_t threadId, size_t address,
                                      int32_t increment,
                                      MemoryAccessMode accessMode) {
    info(std::format("ACTION: t{}#{}: {} +{}", threadId, address,
                     toString(accessMode), increment));
}
void StorageLogger::fence(size_t threadId, MemoryAccessMode accessMode) {
    info(std::format("ACTION: t{}: {} fence", threadId, toString(accessMode)));
}

} // namespace wmm::storage