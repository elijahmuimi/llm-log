#pragma once

// llm_log.hpp -- Zero-dependency single-header C++ structured logger for LLM API calls.
// Writes JSONL (one JSON object per line). Includes RAII ScopedCall, query, and summarize.
//
// USAGE:
//   In exactly ONE .cpp file:
//     #define LLM_LOG_IMPLEMENTATION
//     #include "llm_log.hpp"
//   In all other files:
//     #include "llm_log.hpp"
//
// No external dependencies. C++17.

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace llm {

// ---------------------------------------------------------------------------
// LogEntry — one recorded LLM call
// ---------------------------------------------------------------------------
struct LogEntry {
    std::string id;
    std::time_t timestamp    = 0;
    std::string model;
    std::string prompt;
    std::string response;
    size_t input_tokens      = 0;
    size_t output_tokens     = 0;
    double cost_usd          = 0.0;
    double latency_ms        = 0.0;
    bool   success           = true;
    std::string error;
    std::map<std::string, std::string> metadata;
};

// ---------------------------------------------------------------------------
// LogConfig
// ---------------------------------------------------------------------------
struct LogConfig {
    std::string filepath          = "llm_calls.jsonl"; ///< "" = no file output
    bool also_stdout              = false;
    bool pretty_print             = false;
    size_t max_prompt_chars       = 0; ///< 0 = no truncation
    size_t max_response_chars     = 0;
};

// ---------------------------------------------------------------------------
// Logger
// ---------------------------------------------------------------------------
class Logger {
public:
    explicit Logger(LogConfig config = {});
    ~Logger();

    /// Write a completed LogEntry.
    void log(const LogEntry& entry);

    /// RAII helper: starts a timer, logs on destruction.
    class ScopedCall {
    public:
        ScopedCall(Logger& logger,
                   const std::string& model,
                   const std::string& prompt,
                   std::map<std::string, std::string> metadata = {});
        ~ScopedCall();

        void set_response(const std::string& response);
        void set_tokens(size_t input, size_t output);
        void set_cost(double usd);
        void set_error(const std::string& error);

    private:
        Logger& logger_;
        LogEntry entry_;
        std::chrono::steady_clock::time_point start_;
        bool logged_ = false;
    };

    /// Query options (use has_* flags instead of std::optional for simplicity).
    struct QueryOptions {
        bool has_model_filter = false;
        std::string model_filter;
        bool has_after  = false;
        std::time_t after  = 0;
        bool has_before = false;
        std::time_t before = 0;
        bool success_only = false;
        size_t limit = 100;
    };

    /// Read log file and return matching entries.
    std::vector<LogEntry> query(const QueryOptions& opts = {});

    struct Summary {
        size_t total_calls    = 0;
        size_t successful     = 0;
        size_t failed         = 0;
        double total_cost_usd = 0.0;
        size_t total_tokens   = 0;
        double avg_latency_ms = 0.0;
        std::map<std::string, size_t> calls_per_model;
    };

    /// Summarize all entries in the log file.
    Summary summarize();

private:
    LogConfig    config_;
    std::ofstream file_;
    std::mutex   mutex_;
};

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_LOG_IMPLEMENTATION

#include <algorithm>
#include <cstring>
#include <iomanip>

namespace llm {
namespace detail {

// Generate an 8-char hex ID
static std::string generate_id() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << dist(rng);
    return oss.str();
}

// JSON-escape a string value
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

static std::string truncate(const std::string& s, size_t max_chars) {
    if (max_chars == 0 || s.size() <= max_chars) return s;
    return s.substr(0, max_chars) + "...[truncated]";
}

// Serialize a LogEntry to a single JSON line
static std::string entry_to_json(const LogEntry& e, const LogConfig& cfg, bool pretty) {
    std::string prompt   = truncate(e.prompt,   cfg.max_prompt_chars);
    std::string response = truncate(e.response, cfg.max_response_chars);

    const char* nl  = pretty ? "\n" : "";
    const char* ind = pretty ? "  " : "";
    std::ostringstream o;
    o << "{" << nl
      << ind << "\"id\":\""            << json_escape(e.id)       << "\"," << nl
      << ind << "\"timestamp\":"       << static_cast<long long>(e.timestamp) << "," << nl
      << ind << "\"model\":\""         << json_escape(e.model)    << "\"," << nl
      << ind << "\"prompt\":\""        << json_escape(prompt)     << "\"," << nl
      << ind << "\"response\":\""      << json_escape(response)   << "\"," << nl
      << ind << "\"input_tokens\":"    << e.input_tokens          << "," << nl
      << ind << "\"output_tokens\":"   << e.output_tokens         << "," << nl
      << ind << "\"cost_usd\":"        << std::fixed << std::setprecision(8) << e.cost_usd << "," << nl
      << ind << "\"latency_ms\":"      << std::fixed << std::setprecision(3) << e.latency_ms << "," << nl
      << ind << "\"success\":"         << (e.success ? "true" : "false") << "," << nl
      << ind << "\"error\":\""         << json_escape(e.error)    << "\"," << nl
      << ind << "\"metadata\":{";
    bool first = true;
    for (const auto& kv : e.metadata) {
        if (!first) o << ",";
        o << nl << ind << ind << "\"" << json_escape(kv.first) << "\":\"" << json_escape(kv.second) << "\"";
        first = false;
    }
    o << (pretty && !e.metadata.empty() ? "\n  " : "") << "}" << nl << "}";
    return o.str();
}

// Extract a string value from a flat JSON line by key
static std::string json_get_string(const std::string& line, const std::string& key) {
    std::string pat = "\"" + key + "\":\"";
    auto pos = line.find(pat);
    if (pos == std::string::npos) return {};
    pos += pat.size();
    std::string result;
    while (pos < line.size() && line[pos] != '"') {
        if (line[pos] == '\\' && pos + 1 < line.size()) {
            switch (line[pos+1]) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += line[pos+1]; break;
            }
            pos += 2;
        } else {
            result += line[pos++];
        }
    }
    return result;
}

static long long json_get_number(const std::string& line, const std::string& key) {
    std::string pat = "\"" + key + "\":";
    auto pos = line.find(pat);
    if (pos == std::string::npos) return 0;
    pos += pat.size();
    // skip quote if string number
    if (pos < line.size() && line[pos] == '"') ++pos;
    long long val = 0;
    bool neg = false;
    if (pos < line.size() && line[pos] == '-') { neg = true; ++pos; }
    while (pos < line.size() && line[pos] >= '0' && line[pos] <= '9')
        val = val * 10 + (line[pos++] - '0');
    return neg ? -val : val;
}

static double json_get_double(const std::string& line, const std::string& key) {
    std::string pat = "\"" + key + "\":";
    auto pos = line.find(pat);
    if (pos == std::string::npos) return 0.0;
    pos += pat.size();
    std::string num;
    while (pos < line.size() && (std::isdigit(static_cast<unsigned char>(line[pos]))
           || line[pos] == '.' || line[pos] == '-' || line[pos] == 'e' || line[pos] == 'E' || line[pos] == '+'))
        num += line[pos++];
    try { return std::stod(num); } catch (...) { return 0.0; }
}

static bool json_get_bool(const std::string& line, const std::string& key) {
    std::string pat = "\"" + key + "\":";
    auto pos = line.find(pat);
    if (pos == std::string::npos) return false;
    pos += pat.size();
    return line.substr(pos, 4) == "true";
}

static LogEntry json_to_entry(const std::string& line) {
    LogEntry e;
    e.id            = json_get_string(line, "id");
    e.timestamp     = static_cast<std::time_t>(json_get_number(line, "timestamp"));
    e.model         = json_get_string(line, "model");
    e.prompt        = json_get_string(line, "prompt");
    e.response      = json_get_string(line, "response");
    e.input_tokens  = static_cast<size_t>(json_get_number(line, "input_tokens"));
    e.output_tokens = static_cast<size_t>(json_get_number(line, "output_tokens"));
    e.cost_usd      = json_get_double(line, "cost_usd");
    e.latency_ms    = json_get_double(line, "latency_ms");
    e.success       = json_get_bool(line, "success");
    e.error         = json_get_string(line, "error");
    return e;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Logger implementation
// ---------------------------------------------------------------------------

Logger::Logger(LogConfig config) : config_(std::move(config)) {
    if (!config_.filepath.empty()) {
        file_.open(config_.filepath, std::ios::app);
    }
}

Logger::~Logger() {
    if (file_.is_open()) file_.close();
}

void Logger::log(const LogEntry& in) {
    LogEntry e = in;
    if (e.id.empty())        e.id        = detail::generate_id();
    if (e.timestamp == 0)    e.timestamp = std::time(nullptr);

    std::string line = detail::entry_to_json(e, config_, config_.pretty_print);

    std::lock_guard<std::mutex> lk(mutex_);
    if (file_.is_open()) {
        file_ << line << "\n";
        file_.flush();
    }
    if (config_.also_stdout) {
        // Print compact single-line version to stdout
        std::string compact = detail::entry_to_json(e, config_, false);
        std::cout << compact << "\n";
    }
}

// ScopedCall
Logger::ScopedCall::ScopedCall(Logger& logger, const std::string& model,
                                const std::string& prompt,
                                std::map<std::string, std::string> metadata)
    : logger_(logger), start_(std::chrono::steady_clock::now())
{
    entry_.model    = model;
    entry_.prompt   = prompt;
    entry_.metadata = std::move(metadata);
    entry_.timestamp = std::time(nullptr);
}

Logger::ScopedCall::~ScopedCall() {
    if (!logged_) {
        double elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start_).count();
        entry_.latency_ms = elapsed;
        logger_.log(entry_);
        logged_ = true;
    }
}

void Logger::ScopedCall::set_response(const std::string& r) { entry_.response = r; }
void Logger::ScopedCall::set_tokens(size_t in, size_t out)  { entry_.input_tokens = in; entry_.output_tokens = out; }
void Logger::ScopedCall::set_cost(double usd)               { entry_.cost_usd = usd; }
void Logger::ScopedCall::set_error(const std::string& err)  { entry_.error = err; entry_.success = false; }

// query
std::vector<LogEntry> Logger::query(const QueryOptions& opts) {
    std::vector<LogEntry> results;
    if (config_.filepath.empty()) return results;

    std::ifstream f(config_.filepath);
    if (!f.is_open()) return results;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] != '{') continue;
        LogEntry e = detail::json_to_entry(line);

        if (opts.has_model_filter && e.model != opts.model_filter) continue;
        if (opts.has_after  && e.timestamp < opts.after)  continue;
        if (opts.has_before && e.timestamp > opts.before) continue;
        if (opts.success_only && !e.success)              continue;

        results.push_back(std::move(e));
        if (results.size() >= opts.limit) break;
    }
    return results;
}

// summarize
Logger::Summary Logger::summarize() {
    Summary s;
    auto entries = query();
    s.total_calls = entries.size();
    double latency_sum = 0.0;
    for (const auto& e : entries) {
        if (e.success) ++s.successful; else ++s.failed;
        s.total_cost_usd += e.cost_usd;
        s.total_tokens   += e.input_tokens + e.output_tokens;
        latency_sum      += e.latency_ms;
        s.calls_per_model[e.model]++;
    }
    s.avg_latency_ms = s.total_calls > 0 ? latency_sum / s.total_calls : 0.0;
    return s;
}

} // namespace llm

#endif // LLM_LOG_IMPLEMENTATION
