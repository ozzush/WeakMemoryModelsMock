#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Executor.h"
#include "Parser.h"
#include "PartialStoreOrderStorageManager.h"
#include "Program.h"
#include "SequentialConsistencyStorageManager.h"
#include "TotalStoreOrderStorageManager.h"

using namespace wmm::execution;
using namespace wmm::program;
using namespace wmm::storage;

#define INIT_INTERNAL_UPDATE_MANAGER(var_name, namespace_name)                 \
    {                                                                          \
        using namespace namespace_name;                                        \
        switch (mode) {                                                        \
            case ExecutionMode::Random: {                                      \
                std::random_device seedGen;                                    \
                (var_name) = std::make_unique<RandomInternalUpdateManager>(    \
                        seedGen());                                            \
                break;                                                         \
            }                                                                  \
            case ExecutionMode::Interactive:                                   \
                (var_name) =                                                   \
                        std::make_unique<InteractiveInternalUpdateManager>();  \
                break;                                                         \
            case ExecutionMode::Enumerate:                                     \
                throw std::runtime_error(                                      \
                        "Enumerate execution is not supported");               \
                break;                                                         \
        }                                                                      \
    }

std::ifstream openFile(const std::string &filename) {
    std::ifstream filestream(filename, std::ifstream::in);
    if (!filestream.is_open()) {
        throw std::runtime_error(std::format("Couldn't open file ", filename));
    }
    return filestream;
}

enum class MemoryModel { TSO, PSO, SC, RA, SRA };

MemoryModel parseMemoryModel(const std::string &model) {
    if (model == "tso") {
        return MemoryModel::TSO;
    } else if (model == "pso") {
        return MemoryModel::PSO;
    } else if (model == "sc") {
        return MemoryModel::SC;
    } else if (model == "ra") {
        return MemoryModel::RA;
    } else if (model == "sra") {
        return MemoryModel::SRA;
    } else {
        throw std::runtime_error("Unknown memory model: " + model);
    }
}

enum class ExecutionMode { Random, Interactive, Enumerate };

ExecutionMode parseExecutionMode(const std::string &mode) {
    if (mode == "rand") {
        return ExecutionMode::Random;
    } else if (mode == "interact") {
        return ExecutionMode::Interactive;
    } else if (mode == "enum") {
        return ExecutionMode::Enumerate;
    } else {
        throw std::runtime_error("Unknown execution mode: " + mode);
    }
}


int main(int argc, char *argv[]) {
    std::ifstream filestream = openFile(argv[1]);
    std::vector<Program> programs = Parser::parseFromStream(filestream);
    MemoryModel model = parseMemoryModel(argv[2]);
    ExecutionMode mode = parseExecutionMode(argv[3]);
    LogLevel log = static_cast<LogLevel>(std::stoi(argv[4]));
    LoggerPtr logger(new StorageLoggerImpl(std::cout, log));

    StorageManagerPtr storageManager;
    switch (model) {
        case MemoryModel::TSO: {
            TSO::InternalUpdateManagerPtr internalUpdateManager;
            INIT_INTERNAL_UPDATE_MANAGER(internalUpdateManager, TSO)
            storageManager =
                    std::make_unique<TSO::TotalStoreOrderStorageManager>(
                            10, programs.size(),
                            std::move(internalUpdateManager),
                            std::move(logger));
            break;
        }
        case MemoryModel::PSO: {
            PSO::InternalUpdateManagerPtr internalUpdateManager;
            INIT_INTERNAL_UPDATE_MANAGER(internalUpdateManager, PSO)
            storageManager =
                    std::make_unique<PSO::PartialStoreOrderStorageManager>(
                            10, programs.size(),
                            std::move(internalUpdateManager),
                            std::move(logger));
            break;
        }
        case MemoryModel::SC:
            storageManager =
                    std::make_unique<SC::SequentialConsistencyStorageManager>(
                            10, std::move(logger));
            break;
        case MemoryModel::RA:
            throw std::runtime_error("Release-Acquire not supported");
        case MemoryModel::SRA:
            throw std::runtime_error("Strong Release-Acquire not supported");
    }

    ExecutorPtr executor;
    switch (mode) {
        case ExecutionMode::Random: {
            std::random_device seedGen;
            executor = std::make_unique<RandomExecutor>(
                    programs, storageManager, 10, seedGen());
            break;
        }
        case ExecutionMode::Interactive:
            executor = std::make_unique<InteractiveExecutor>(
                    programs, storageManager, 10);
            break;
        case ExecutionMode::Enumerate:
            throw std::runtime_error("Enumerate execution is not implemented");
    }

    while (executor->execute()) {}
    executor->writeState(std::cout);
}
