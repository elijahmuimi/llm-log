# llm-log

Log every LLM call to JSONL in C++. One header, zero deps.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License MIT](https://img.shields.io/badge/license-MIT-green.svg)
![Single Header](https://img.shields.io/badge/single-header-orange.svg)
![No deps](https://img.shields.io/badge/deps-none-lightgrey.svg)

## Quickstart

```cpp
#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"

llm::Logger logger;  // writes to llm_calls.jsonl

// RAII: timer starts here, entry logged on scope exit
{
    llm::Logger::ScopedCall call(logger, "gpt-4o-mini", "Explain recursion.",
                                 {{"user", "alice"}, {"session", "xyz"}});

    std::string response = call_my_llm_api(prompt); // your API call here

    call.set_response(response);
    call.set_tokens(12, 48);
    call.set_cost(0.0000072);
}  // <-- logged here automatically, latency measured

// Query and summarize
auto s = logger.summarize();
std::cout << "Total spend: $" << s.total_cost_usd << "\n";
```

## Sample JSONL output

```json
{"id":"a3f1c2d0","timestamp":1700000000,"model":"gpt-4o-mini","prompt":"Explain recursion.","response":"Recursion is...","input_tokens":12,"output_tokens":48,"cost_usd":0.00000720,"latency_ms":312.441,"success":true,"error":"","metadata":{"session":"xyz","user":"alice"}}
```

## API Reference

### LogEntry

```cpp
struct LogEntry {
    std::string id;            // auto-generated 8-char hex if empty
    std::time_t timestamp;     // auto-set to now() if 0
    std::string model;
    std::string prompt;
    std::string response;
    size_t input_tokens;
    size_t output_tokens;
    double cost_usd;
    double latency_ms;
    bool   success;
    std::string error;
    std::map<std::string, std::string> metadata;
};
```

### Logger

```cpp
llm::Logger logger;                   // default: writes to "llm_calls.jsonl"
llm::Logger logger(config);           // custom config

logger.log(entry);                    // write a completed entry

// RAII helper (recommended)
llm::Logger::ScopedCall call(logger, model, prompt, metadata);
call.set_response(text);
call.set_tokens(input, output);
call.set_cost(usd);
call.set_error("reason");             // marks success=false

// Query
llm::Logger::QueryOptions opts;
opts.has_model_filter = true;
opts.model_filter     = "gpt-4o-mini";
opts.success_only     = true;
opts.limit            = 50;
auto entries = logger.query(opts);

// Summarize all entries
auto s = logger.summarize();
// s.total_calls, s.successful, s.failed, s.total_cost_usd,
// s.total_tokens, s.avg_latency_ms, s.calls_per_model
```

### LogConfig

```cpp
struct LogConfig {
    std::string filepath        = "llm_calls.jsonl"; // "" = no file
    bool also_stdout            = false;
    bool pretty_print           = false;
    size_t max_prompt_chars     = 0;   // 0 = no truncation
    size_t max_response_chars   = 0;
};
```

## Examples

| File | What it shows |
|------|--------------|
| [`examples/basic_log.cpp`](examples/basic_log.cpp) | Log 3 mock calls, print summary |
| [`examples/scoped_call.cpp`](examples/scoped_call.cpp) | RAII ScopedCall wrapping a mock API call |
| [`examples/query_log.cpp`](examples/query_log.cpp) | Write 10 entries, filter by model, summarize |

## Building

```bash
cmake -B build && cmake --build build
./build/basic_log
./build/scoped_call
./build/query_log
```

## Requirements

C++17. No external dependencies.

## See Also

| Repo | What it does |
|------|-------------|
| [llm-stream](https://github.com/Mattbusel/llm-stream) | Streaming responses |
| [llm-cache](https://github.com/Mattbusel/llm-cache) | Response caching |
| [llm-cost](https://github.com/Mattbusel/llm-cost) | Token counting + cost |
| [llm-retry](https://github.com/Mattbusel/llm-retry) | Retry + circuit breaker |
| [llm-format](https://github.com/Mattbusel/llm-format) | Structured output |
| [llm-embed](https://github.com/Mattbusel/llm-embed) | Embeddings + vector search |
| [llm-pool](https://github.com/Mattbusel/llm-pool) | Concurrent request pool |
| **llm-log** *(this repo)* | Structured JSONL logging |
| [llm-template](https://github.com/Mattbusel/llm-template) | Prompt templating |
| [llm-agent](https://github.com/Mattbusel/llm-agent) | Tool-calling agent loop |
| [llm-rag](https://github.com/Mattbusel/llm-rag) | RAG pipeline |
| [llm-eval](https://github.com/Mattbusel/llm-eval) | Evaluation + consistency scoring |

## License

MIT -- see [LICENSE](LICENSE).
