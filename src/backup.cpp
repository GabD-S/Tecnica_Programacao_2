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
        if (!line.empty()) items.push_back(line);
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
    } catch (const std::exception& e) {
        return {3, std::string("exception: ") + e.what()};
    }

    return {0, "ok"};
}

} // namespace tp2
