#pragma once
#include <string>
#include <vector>

namespace tp2 {

enum class Operation { Backup, Restore };

struct ActionResult {
    int code;              // 0 success; >0 error codes per spec
    std::string message;   // optional detail
};

// Reads Backup.parm and synchronizes files between hdPath and penPath
// according to the decision table (see course material).
ActionResult execute_backup(const std::string& hdPath,
                           const std::string& penPath,
                           const std::string& paramFile,
                           Operation op);

// Utility for listing parameter file entries; separated for testability.
std::vector<std::string> read_param_list(const std::string& paramFile);

} // namespace tp2
