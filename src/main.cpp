#include "Parser.h"
#include "Program.h"
#include "StorageManager.h"
#include "ThreadManager.h"
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

    StorageManagerPtr storageManager(new FakeStorageManager());
    ThreadManager threadManager(programs, storageManager, 10);

    while (true) {
        bool successAny = false;
        for (size_t i = 0; i < programs.size(); ++i) {
            successAny |= threadManager.evaluateThread(i);
        }
        if (!successAny) {
            std::cout << (threadManager.allThreadsCompleted()
                                  ? "All threads completed successfully\n"
                                  : "All threads are blocked\n");
            auto localStorages = threadManager.getThreadLocalStorages();
            std::cout << "Thread-local storages:\n";
            for (size_t i = 0; i < localStorages.size(); ++i) {
                std::cout << "t" << i << ": ";
                auto storage = localStorages[i].getStorage();
                for (auto elm : storage) {
                    std::cout << elm << ' ';
                }
                std::cout << '\n';
            }
            return 0;
        }
    }
}
