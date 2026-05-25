#pragma once
#include <string>

namespace meshcompute::compute::workers {

std::string run_llm_inference(const std::string& prompt);

} // namespace meshcompute::compute::workers