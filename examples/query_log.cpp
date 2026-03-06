#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"
#include <iostream>

int main() {
    llm::LogConfig cfg;
    cfg.filepath = "query_test.jsonl";
    llm::Logger logger(cfg);

    // Write 10 mock entries across two models
    const char* models[] = {"gpt-4o-mini", "claude-3-5-haiku-20241022"};
    for (int i = 0; i < 10; ++i) {
        llm::LogEntry e;
        e.model         = models[i % 2];
        e.prompt        = "Question number " + std::to_string(i);
        e.response      = "Answer " + std::to_string(i);
        e.input_tokens  = static_cast<size_t>(10 + i);
        e.output_tokens = static_cast<size_t>(5 + i);
        e.cost_usd      = 0.000001 * (i + 1);
        e.latency_ms    = 100.0 + i * 20.0;
        e.success       = (i != 3 && i != 7); // two failures
        e.error         = e.success ? "" : "Timeout";
        e.timestamp     = std::time(nullptr);
        logger.log(e);
    }

    std::cout << "Wrote 10 entries to " << cfg.filepath << "\n\n";

    // Query: gpt-4o-mini only
    {
        llm::Logger::QueryOptions opts;
        opts.has_model_filter = true;
        opts.model_filter     = "gpt-4o-mini";
        auto results = logger.query(opts);
        std::cout << "gpt-4o-mini entries: " << results.size() << "\n";
    }

    // Query: successful only
    {
        llm::Logger::QueryOptions opts;
        opts.success_only = true;
        auto results = logger.query(opts);
        std::cout << "Successful entries:  " << results.size() << "\n";
    }

    // Full summary
    auto s = logger.summarize();
    std::cout << "\nFull Summary:\n"
              << "  Total:        " << s.total_calls    << "\n"
              << "  Successful:   " << s.successful     << "\n"
              << "  Failed:       " << s.failed         << "\n"
              << "  Total tokens: " << s.total_tokens   << "\n"
              << "  Total cost:   $" << s.total_cost_usd << "\n"
              << "  Avg latency:  " << s.avg_latency_ms  << " ms\n"
              << "\nCalls per model:\n";
    for (const auto& kv : s.calls_per_model)
        std::cout << "  " << kv.first << ": " << kv.second << "\n";
    return 0;
}
