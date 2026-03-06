# CLAUDE.md -- llm-log

## Build
```bash
cmake -B build && cmake --build build
./build/Release/basic_log
```

## THE ONE RULE: SINGLE HEADER
`include/llm_log.hpp` is the entire library. Never split it.

## API Surface
```cpp
namespace llm {
    struct LogEntry { /* id, timestamp, model, prompt, response, tokens, cost, latency, success, error, metadata */ };
    struct LogConfig { /* filepath, also_stdout, pretty_print, max_*_chars */ };
    class Logger {
        Logger(LogConfig config = {});
        void log(const LogEntry&);
        class ScopedCall { /* RAII: set_response/tokens/cost/error, logs on ~ScopedCall() */ };
        std::vector<LogEntry> query(const QueryOptions&);
        Summary summarize();
    };
}
```

## Common Mistakes
- `#include <iostream>` is in the header itself (needed for `also_stdout`). Don't remove it.
- ScopedCall `logged_` flag prevents double-logging if destructor fires early.
- query() opens the file fresh each call -- works for append-only logs.
- IDs are 8-char hex from mt19937. Not cryptographically secure, just unique enough for logs.
- JSONL format: one flat JSON object per line, no pretty printing in query output.
