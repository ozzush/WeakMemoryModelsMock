//
// Created by veronika on 22.10.23.
//

#include "Logger.h"
#include "StorageManager.h"

namespace wmm {
void storage::Logger::storage(const storage::StorageManagerInterface &storage) {
    storage.writeStorage(m_outputStream.get());
}

void storage::Logger::error(const std::string &log)  {
    if (m_logLevel <= LogLevel::ERROR) {
        m_outputStream.get() << "ERROR: " << log << std::endl;
    }
}

void storage::Logger::warning(const std::string &log) {
    if (m_logLevel <= LogLevel::WARNING) {
        m_outputStream.get() << "WARN: " << log << std::endl;
    }
}

void storage::Logger::info(const std::string &log)  {
    if (m_logLevel <= LogLevel::INFO) {
        m_outputStream.get() << "INFO: " << log << std::endl;
    }
}

} // namespace wmm