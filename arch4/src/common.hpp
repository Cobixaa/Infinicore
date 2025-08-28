// Minimal utilities shared across the ARCH4 project
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <random>
#include <filesystem>
#include <cmath>

namespace arch4 {

inline std::string read_file_to_string(const std::string &path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) return std::string();
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline bool write_string_to_file(const std::string &path, const std::string &data) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << data;
    return true;
}

inline std::string trim_copy(const std::string &s) {
    size_t start = 0; while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size(); while (end > start && std::isspace(static_cast<unsigned char>(s[end-1]))) end--;
    return s.substr(start, end - start);
}

inline std::string to_lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> out; std::string item; std::istringstream iss(s);
    while (std::getline(iss, item, delim)) out.push_back(item);
    return out;
}

inline std::vector<std::string> tokenize_words(const std::string &s) {
    std::vector<std::string> tokens; tokens.reserve(64);
    std::string cur;
    for (char ch : s) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch=='_' ) {
            cur.push_back(std::tolower(static_cast<unsigned char>(ch)));
        } else {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

inline std::string now_iso8601() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

inline std::string join(const std::vector<std::string> &v, const std::string &sep) {
    std::ostringstream oss; for (size_t i=0;i<v.size();++i){ if(i) oss<<sep; oss<<v[i]; } return oss.str();
}

}

