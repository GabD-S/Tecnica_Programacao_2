#include "backup.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace tp2 {

namespace {
// Copy file contents from src to dst, return true on success; preserves src mtime on dst.
bool copy_with_mtime_preserve(const std::filesystem::path& src, const std::filesystem::path& dst) {
    namespace fs = std::filesystem;
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;
    std::ofstream out(dst, std::ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    if (!out.good()) { out.close(); return false; }
    out.flush();
    out.close();
    auto t_src = fs::last_write_time(src);
    fs::last_write_time(dst, t_src);
    return true;
}

// Refactor helpers for backup flow (no behavior change)
bool is_dir_or_missing(const std::filesystem::path& p) {
    namespace fs = std::filesystem;
    return !fs::exists(p) || fs::is_directory(p);
}

bool backup_copy_or_update(const std::filesystem::path& src, const std::filesystem::path& dst) {
    namespace fs = std::filesystem;
    if (!fs::exists(dst)) {
        fs::create_directories(dst.parent_path());
        return copy_with_mtime_preserve(src, dst);
    }
    auto t_src = fs::last_write_time(src);
    auto t_dst = fs::last_write_time(dst);
    if (t_src > t_dst) {
        return copy_with_mtime_preserve(src, dst);
    }
    return true; // equal or dst newer => no action needed
}
}

std::vector<std::string> read_param_list(const std::string& paramFile) {
    std::vector<std::string> items;
    std::ifstream in(paramFile);
    if (!in.is_open()) return items; // empty => caller can treat as impossible
    std::string line;
    while (std::getline(in, line)) {
        // trim leading/trailing whitespace
        auto begin = line.find_first_not_of(" \t\r\n");
        auto end = line.find_last_not_of(" \t\r\n");
    if (begin == std::string::npos) continue; // blank line
    std::string trimmed = line.substr(begin, end - begin + 1);
    // Skip comment lines that start with '#' or ';'
    if (!trimmed.empty() && (trimmed[0] == '#' || trimmed[0] == ';')) continue;
    if (!trimmed.empty()) items.push_back(trimmed);
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

    try {
        if (op == Operation::Backup) {
            for (const auto& name : list) {
                fs::path src = fs::path(hdPath) / name;
                fs::path dst = fs::path(penPath) / name;

                if (is_dir_or_missing(src)) {
                    // skip missing or directory entries (no recursive copy)
                    continue;
                }

                (void)backup_copy_or_update(src, dst);
            }
        } else if (op == Operation::Restore) {
            // Minimal restore: when file exists only on Pen, copy to HD
            bool any_missing = false;
            bool any_write_error = false;
            for (const auto& name : list) {
                fs::path src = fs::path(penPath) / name;
                fs::path dst = fs::path(hdPath) / name;

                if (!fs::exists(src)) {
                    // Listed file missing on pen: mark error but keep going
                    any_missing = true;
                    continue;
                }

                if (!fs::exists(dst)) {
                    fs::create_directories(dst.parent_path());
                    if (!copy_with_mtime_preserve(src, dst)) { any_write_error = true; continue; }
                } else {
                    // Both exist: update HD if pen is newer
                    auto t_src = fs::last_write_time(src);
                    auto t_dst = fs::last_write_time(dst);
                    if (t_src > t_dst) {
                        if (!copy_with_mtime_preserve(src, dst)) { any_write_error = true; continue; }
                    }
                }
            }
            if (any_write_error) return {5, "failed to write to HD"};
            if (any_missing) return {4, "one or more source files missing on pen"};
        } else {
            return {2, "operation not supported in minimal implementation"};
        }
    } catch (const std::exception& e) {
        return {3, std::string("exception: ") + e.what()};
    }

    return {0, "ok"};
}

} // namespace tp2
