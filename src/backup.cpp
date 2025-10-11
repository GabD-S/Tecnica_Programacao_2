#include "backup.hpp"
#include <fstream>
#include <sstream>

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

ActionResult execute_backup(const std::string& /*hdPath*/,
                            const std::string& /*penPath*/,
                            const std::string& paramFile,
                            Operation /*op*/) {
    // Minimal placeholder to enable building tests (TDD RED stage follows).
    auto list = read_param_list(paramFile);
    if (list.empty()) {
        return {1, "param file missing or empty"};
    }
    return {0, "not implemented: synchronization logic"};
}

} // namespace tp2
