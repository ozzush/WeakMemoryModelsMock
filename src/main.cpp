#include "Executor.h"
#include "Parser.h"
#include "Program.h"
#include "SequentialConsistencyStorageManager.h"
#include "StorageManager.h"
#include "TotalStoreOrderStorageManager.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace wmm;

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
    InternalUpdateManagerPtr internalUpdateManager(new RandomTSOInternalUpdateManager(seedGen()));
    StorageManagerPtr storageManager(new TotalStoreOrderStorageManager(10, programs.size(), std::move(internalUpdateManager)));
    RandomExecutor executor(programs, storageManager, 10, seedGen());

    while (executor.execute());
    executor.writeState(std::cout);
}
