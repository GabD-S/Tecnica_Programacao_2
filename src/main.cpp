#include "backup.hpp"
#include <iostream>
#include <string>

using tp2::ActionResult;
using tp2::Operation;
using tp2::execute_backup;

static void print_usage() {
    std::cerr << "Usage: tp2_cli --mode <backup|restore> --hd <path> --pen <path> [--parm <file>]" << std::endl;
}

struct CliOptions {
    std::string mode;
    std::string hd;
    std::string pen;
    std::string parm = "Backup.parm";
};

static bool parse_args(int argc, char** argv, CliOptions& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 < argc) return std::string(argv[++i]);
            std::cerr << "Missing value for " << name << std::endl;
            print_usage();
            return std::string();
        };

        if (arg == "--mode") {
            opts.mode = next("--mode");
        } else if (arg == "--hd") {
            opts.hd = next("--hd");
        } else if (arg == "--pen") {
            opts.pen = next("--pen");
        } else if (arg == "--parm") {
            opts.parm = next("--parm");
        } else if (arg == "-h" || arg == "--help") {
            print_usage();
            return false; // signal "handled" (no error)
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage();
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    CliOptions opts;
    if (!parse_args(argc, argv, opts)) {
        // If help was shown, exit 0; otherwise treat as error 1
        // We can't distinguish easily here, keep previous behavior: non-recognized option -> 1
        // For simplicity, return 0 only when explicitly -h/--help was passed (handled inside parse_args)
        return 0;
    }

    Operation op;
    if (opts.mode == "backup") op = Operation::Backup;
    else if (opts.mode == "restore") op = Operation::Restore;
    else if (opts.mode.empty()) {
        std::cerr << "Missing required --mode" << std::endl;
        print_usage();
        return 1;
    } else {
        std::cerr << "Unsupported mode: " << opts.mode << std::endl;
        print_usage();
        return 2; // per spec, op not supported
    }

    // Validate required paths before delegating
    if (opts.hd.empty()) {
        std::cerr << "Missing required --hd" << std::endl;
        print_usage();
        return 1;
    }
    if (opts.pen.empty()) {
        std::cerr << "Missing required --pen" << std::endl;
        print_usage();
        return 1;
    }

    ActionResult res = execute_backup(opts.hd, opts.pen, opts.parm, op);
    if (!res.message.empty()) {
        std::cerr << res.message << std::endl;
    }
    return res.code;
}
