#include "backup.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace tp2 {

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

                if (!fs::exists(src)) {
                    // In full implementation this might be an error per table decision; keep going for now.
                    continue;
                }

                if (!fs::exists(dst)) {
                    fs::create_directories(dst.parent_path());
                    std::ifstream in(src, std::ios::binary);
                    std::ofstream out(dst, std::ios::binary);
                    out << in.rdbuf();
                }
            }
        } else if (op == Operation::Restore) {
            // Minimal restore: when file exists only on Pen, copy to HD
            for (const auto& name : list) {
                fs::path src = fs::path(penPath) / name;
                fs::path dst = fs::path(hdPath) / name;

                if (!fs::exists(src)) {
                    // Listed file missing on pen: treat as error
                    return {4, "source file missing on pen"};
                }

                if (!fs::exists(dst)) {
                    fs::create_directories(dst.parent_path());
                    std::ifstream in(src, std::ios::binary);
                    std::ofstream out(dst, std::ios::binary);
                    out << in.rdbuf();
                    out.flush();
                    out.close();
                    // preserve timestamp from pen to HD (set after closing stream)
                    auto t_src = fs::last_write_time(src);
                    fs::last_write_time(dst, t_src);
                } else {
                    // Both exist: update HD if pen is newer
                    auto t_src = fs::last_write_time(src);
                    auto t_dst = fs::last_write_time(dst);
                    if (t_src > t_dst) {
                        std::ifstream in(src, std::ios::binary);
                        std::ofstream out(dst, std::ios::binary);
                        out << in.rdbuf();
                        out.flush();
                        out.close();
                        // keep destination timestamp in sync with source (set after closing stream)
                        fs::last_write_time(dst, t_src);
                    }
                }
            }
        } else {
            return {2, "operation not supported in minimal implementation"};
        }
    } catch (const std::exception& e) {
        return {3, std::string("exception: ") + e.what()};
    }

    return {0, "ok"};
}

} // namespace tp2
