#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"
#include <iostream>

int main() {
    llm::LogConfig cfg;
    cfg.filepath    = "test_calls.jsonl";
    cfg.also_stdout = false;

    llm::Logger logger(cfg);

    auto make_entry = [](const std::string& model, const std::string& prompt,
                         const std::string& response, size_t in, size_t out,
                         double cost, double latency, bool ok,
                         const std::string& err = "") {
        llm::LogEntry e;
        e.model         = model;
        e.prompt        = prompt;
        e.response      = response;
        e.input_tokens  = in;
        e.output_tokens = out;
        e.cost_usd      = cost;
        e.latency_ms    = latency;
        e.success       = ok;
        e.error         = err;
        e.timestamp     = std::time(nullptr);
        return e;
    };

    logger.log(make_entry("gpt-4o-mini", "What is the capital of France?",
                           "Paris.", 12, 2, 0.000002, 234.5, true));
    logger.log(make_entry("claude-3-5-haiku-20241022", "Explain recursion in one line.",
                           "Recursion is a function that calls itself.", 10, 9, 0.000015, 412.0, true));
    logger.log(make_entry("gpt-4o", "Summarize quantum computing.", "",
                           0, 0, 0.0, 0.0, false, "Rate limit exceeded (429)"));

    std::cout << "Logged 3 calls to " << cfg.filepath << "\n\n";

    auto s = logger.summarize();
    std::cout << "Summary:\n"
              << "  Total calls:  " << s.total_calls    << "\n"
              << "  Successful:   " << s.successful     << "\n"
              << "  Failed:       " << s.failed         << "\n"
              << "  Total cost:   $" << s.total_cost_usd << "\n"
              << "  Total tokens: " << s.total_tokens   << "\n"
              << "  Avg latency:  " << s.avg_latency_ms << " ms\n";
    return 0;
}
