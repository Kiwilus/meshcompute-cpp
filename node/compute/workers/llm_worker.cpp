#include "llm_worker.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace meshcompute {

nlohmann::json LLMWorker::execute(const Job& job) {
    // Beispiel: Aufruf eines lokalen LLM-Servers (Ollama, llama.cpp Server)
    std::string model = job.parameters.value("model", "llama3");
    std::string prompt = job.parameters.value("prompt", "Hello");
    std::string endpoint = job.parameters.value("endpoint", "http://localhost:11434/api/generate");

    // Sehr einfache HTTP-POST mit curl als Platzhalter (in Produktion lieber libcurl)
    std::string curl_cmd = "curl -s -X POST " + endpoint
        + " -H 'Content-Type: application/json' -d '{\"model\":\"" + model + "\",\"prompt\":\"" + prompt + "\"}'";
    
    std::array<char, 1024> buffer;
    std::string response;
    auto pipe = popen(curl_cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("Failed to call LLM endpoint");
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        response += buffer.data();
    pclose(pipe);

    auto result = nlohmann::json::parse(response);
    return result; // Enthält das generierte Feld "response"
}

} // namespace meshcompute