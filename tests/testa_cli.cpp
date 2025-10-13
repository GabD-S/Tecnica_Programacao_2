#include "catch.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unistd.h>

namespace fs = std::filesystem;

// Helper to get process exit status from std::system on POSIX
static int exit_status_from_system(int rc) {
    if (rc == -1) return -1; // system() failed
    // On POSIX, return value is encoded: (exit_status << 8) | signal
    return (rc >> 8) & 0xFF;
}

TEST_CASE("cli: smoke test for backup mode") {
    // Arrange a temporary workspace
    fs::path tmp = fs::temp_directory_path() / ("tp2_cli_smoke_" + std::to_string(::getpid()));
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create a file only on HD and list it in parm
    std::ofstream(tmp / "hd" / "CLI_X.txt") << "from-hd";
    std::ofstream(tmp / "Backup.parm") << "CLI_X.txt\n";

    // Build command line for the future CLI binary
    fs::path hd = tmp / "hd";
    fs::path pen = tmp / "pen";
    fs::path parm = tmp / "Backup.parm";

    // Quote paths to handle spaces
    auto q = [](const fs::path& p) { return std::string("\"") + p.string() + "\""; };

    std::string cmd = std::string("./bin/tp2_cli ") +
                      "--mode backup " +
                      "--hd " + q(hd) + " " +
                      "--pen " + q(pen) + " " +
                      "--parm " + q(parm);

    int rc = std::system(cmd.c_str());
    int ec = exit_status_from_system(rc);

    // Expect success (exit code 0) and that the file was copied to pen
    REQUIRE(ec == 0);
    REQUIRE(fs::exists(pen / "CLI_X.txt"));
    std::string got; { std::ifstream in(pen / "CLI_X.txt"); std::getline(in, got);} 
    REQUIRE(got == "from-hd");

    fs::remove_all(tmp);
}
