#include "backup.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace tp2 {

static std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    auto start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

std::vector<std::string> read_param_list(const std::string& paramFile) {
    std::vector<std::string> items;
    std::ifstream in(paramFile);
    if (!in.is_open()) return items; // empty => caller can treat as impossible
    std::string line;
    while (std::getline(in, line)) {
        auto t = trim(line);
        if (!t.empty()) items.push_back(t);
    }
    return items;
}

ActionResult execute_backup(const std::string& hdPath,
                            const std::string& penPath,
                            const std::string& paramFile,
                            Operation op) {
    using std::string;
    namespace fs = std::filesystem;

    auto list = read_param_list(paramFile);
    if (list.empty()) {
        return {1, "param file missing or empty"};
    }

    // For GREEN: implement only the case Operation::Backup and when file exists on HD but not on Pen => copy.
    if (op != Operation::Backup) {
        return {2, "operation not supported in minimal implementation"};
    }

    try {
        for (const auto& name : list) {
            fs::path src = fs::path(hdPath) / name;
            fs::path dst = fs::path(penPath) / name;

            if (!fs::exists(src)) {
                return {4, "source file missing on HD"};
            }

            if (!fs::exists(dst)) {
                fs::create_directories(dst.parent_path());
                std::ifstream in(src, std::ios::binary);
                std::ofstream out(dst, std::ios::binary);
                out << in.rdbuf();
            } else {
                // Exists in both: copy only if HD newer than Pen
                std::error_code ec1, ec2;
                auto ts_src = fs::last_write_time(src, ec1);
                auto ts_dst = fs::last_write_time(dst, ec2);
                if (!ec1 && !ec2 && ts_src > ts_dst) {
                    std::ifstream in(src, std::ios::binary);
                    std::ofstream out(dst, std::ios::binary | std::ios::trunc);
                    out << in.rdbuf();
                    // Optionally align mtime, but most filesystems update automatically on write
                }
            }
        }
    } catch (const std::exception& e) {
        return {3, std::string("exception: ") + e.what()};
    }

    return {0, "ok"};
}

} // namespace tp2
