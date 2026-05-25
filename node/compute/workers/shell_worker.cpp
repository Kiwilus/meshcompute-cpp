#include "shell_worker.h"
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

namespace meshcompute {

nlohmann::json ShellWorker::execute(const Job& job) {
    std::string cmd = job.parameters.value("command", "");
    if (cmd.empty()) throw std::runtime_error("No command provided");
    std::array<char, 128> buffer;
    std::string result;
    auto pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed");
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    int rc = pclose(pipe);
    return nlohmann::json{{"stdout", result}, {"exit_code", rc}};
}

} // namespace meshcompute