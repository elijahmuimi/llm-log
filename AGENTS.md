# AGENTS.md -- llm-log

## Purpose
Single-header C++ structured logger for LLM API calls. Writes JSONL (one JSON object per line). Includes RAII ScopedCall for automatic latency measurement, query/filter, and summarize.

## Architecture
```
llm-log/
  include/llm_log.hpp   <- THE ENTIRE LIBRARY. Do not split.
  examples/
    basic_log.cpp
    scoped_call.cpp
    query_log.cpp
  CMakeLists.txt
```

## Build
```bash
cmake -B build && cmake --build build
```

## Rules
- Single header only. Never split llm_log.hpp into multiple files.
- No external dependencies whatsoever.
- All public API in namespace llm.
- Implementation inside #ifdef LLM_LOG_IMPLEMENTATION guard.

## API Surface
- LogEntry, LogConfig structs
- Logger class: log(), ScopedCall, query(), summarize()
- ScopedCall RAII: measures latency, logs on destruction
- query() reads JSONL file line by line, filters, returns up to limit results
- summarize() aggregates all entries
