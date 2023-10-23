//
// Created by veronika on 22.10.23.
//

#include "Logger.h"
#include "StorageManager.h"

namespace wmm {
void storage::Logger::storage(const storage::StorageManagerInterface &storage) {
    storage.writeStorage(m_outputStream.get());
}
} // namespace wmm