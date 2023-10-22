////
//// Created by veronika on 21.10.23.
////
//
//#include "TotalStoreOrderStorageManager.h"
//#include <ostream>
//
//namespace wmm {
//
//int32_t TotalStoreOrderStorageManager::load(size_t threadId, size_t address,
//                                            MemoryAccessMode accessMode) {
//    auto valueFromBuffer = m_threadBuffers.at(threadId).find(address);
//    if (valueFromBuffer) { return valueFromBuffer.value(); }
//    return m_storage.load(address);
//}
//
//void TotalStoreOrderStorageManager::store(size_t threadId, size_t address,
//                                          int32_t value,
//                                          MemoryAccessMode accessMode) {
//    m_threadBuffers.at(threadId).push({address, value});
//}
//
//void TotalStoreOrderStorageManager::compareAndSwap(
//        size_t threadId, size_t address, int32_t expectedValue,
//        int32_t newValue, MemoryAccessMode accessMode) {
//    flushBuffer(threadId);
//    auto value = m_storage.load(address);
//    if (value == expectedValue) { m_storage.store(address, newValue); }
//}
//
//void TotalStoreOrderStorageManager::flushBuffer(size_t threadId) {
//    while (propagate(threadId));
//}
//
//void TotalStoreOrderStorageManager::fetchAndIncrement(
//        size_t threadId, size_t address, int32_t increment,
//        MemoryAccessMode accessMode) {
//    flushBuffer(threadId);
//    auto value = m_storage.load(address);
//    m_storage.store(address, value + increment);
//}
//
//void TotalStoreOrderStorageManager::fence(size_t threadId,
//                                          MemoryAccessMode accessMode) {
//    flushBuffer(threadId);
//}
//
//void TotalStoreOrderStorageManager::writeStorage(
//        std::ostream &outputStream) const {
//    outputStream << "Shared storage: " << m_storage << '\n';
//    outputStream << "Thread buffers:\n";
//    for (size_t i = 0; i < m_threadBuffers.size(); ++i) {
//        outputStream << 't' << i << ": " << m_threadBuffers[i] << '\n';
//    }
//}
//
//bool TotalStoreOrderStorageManager::propagate(size_t threadId) {
//    auto instruction = m_threadBuffers.at(threadId).pop();
//    if (instruction) {
//        m_storage.store(instruction->address, instruction->value);
//        return true;
//    }
//    return false;
//}
//
//} // namespace wmm