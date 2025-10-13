#include "backup.hpp"
#include <iostream>
#include <string>

using tp2::ActionResult;
using tp2::Operation;
using tp2::execute_backup;

static void print_usage() {
    std::cerr << "Usage: tp2_cli --mode <backup|restore> --hd <path> --pen <path> [--parm <file>]" << std::endl;
}

int main(int argc, char** argv) {
    std::string mode;
    std::string hd;
    std::string pen;
    std::string parm = "Backup.parm";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 < argc) return std::string(argv[++i]);
            std::cerr << "Missing value for " << name << std::endl;
            print_usage();
            return std::string();
        };

        if (arg == "--mode") {
            mode = next("--mode");
        } else if (arg == "--hd") {
            hd = next("--hd");
        } else if (arg == "--pen") {
            pen = next("--pen");
        } else if (arg == "--parm") {
            parm = next("--parm");
        } else if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage();
            return 1;
        }
    }

    Operation op;
    if (mode == "backup") op = Operation::Backup;
    else if (mode == "restore") op = Operation::Restore;
    else if (mode.empty()) {
        std::cerr << "Missing required --mode" << std::endl;
        print_usage();
        return 1;
    } else {
        std::cerr << "Unsupported mode: " << mode << std::endl;
        print_usage();
        return 2; // per spec, op not supported
    }

    ActionResult res = execute_backup(hd, pen, parm, op);
    if (!res.message.empty()) {
        std::cerr << res.message << std::endl;
    }
    return res.code;
}
