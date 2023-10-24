#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Executor.h"
#include "Parser.h"
#include "Program.h"
#include "SequentialConsistencyStorageManager.h"
#include "StorageManager.h"
#include "TotalStoreOrderStorageManager.h"
#include "PartialStoreOrderStorageManager.h"

using namespace wmm::execution;
using namespace wmm::program;
using namespace wmm::storage;
using namespace wmm::storage::PSO;

int main(int argc, char *argv[]) {
    std::vector<Program> programs;
    for (int i = 1; i < argc; ++i) {
        std::ifstream filestream(argv[i], std::ifstream::in);
        if (!filestream.is_open()) {
            throw std::runtime_error(std::string("Couldn't open file ") + argv[i]);
        }
        programs.push_back(Parser::parseFromStream(filestream));
    }

    std::random_device seedGen;
    InternalUpdateManagerPtr internalUpdateManager(new InteractiveInternalUpdateManager());
    LoggerPtr logger(new StorageLoggerImpl(std::cout));
    StorageManagerPtr storageManager(new PartialStoreOrderStorageManager(10, programs.size(), std::move(internalUpdateManager), std::move(logger)));
    InteractiveExecutor executor(programs, storageManager, 10);

    while (executor.execute()) {}
    executor.writeState(std::cout);
}
