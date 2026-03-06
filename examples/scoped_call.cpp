#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    llm::LogConfig cfg;
    cfg.filepath    = "scoped_test.jsonl";
    cfg.also_stdout = true;

    llm::Logger logger(cfg);

    std::cout << "Making mock LLM call (ScopedCall RAII)...\n\n";
    {
        llm::Logger::ScopedCall call(logger, "gpt-4o-mini",
            "Write a haiku about C++.",
            {{"user", "demo"}, {"session", "abc123"}});

        // Simulate LLM work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        call.set_response("Pointers dance free\nMemory leaks everywhere\nValgrind weeps now");
        call.set_tokens(12, 14);
        call.set_cost(0.000004);
        // Destructor fires here, logs the completed entry automatically
    }

    std::cout << "\nCall logged automatically by RAII destructor.\n";
    return 0;
}
