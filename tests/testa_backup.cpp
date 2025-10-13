#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_NO_POSIX_SIGNALS 1
#include "catch.hpp"
#include "backup.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace tp2;

TEST_CASE("execute_backup returns error when param file missing or empty") {
    fs::path tmp = fs::current_path() / "_tmp_test";
    fs::create_directories(tmp);
    auto hd = (tmp / "hd").string();
    auto pen = (tmp / "pen").string();
    fs::create_directories(hd);
    fs::create_directories(pen);

    // Case 1: missing file
    auto r1 = execute_backup(hd, pen, (tmp / "Backup.missing").string(), Operation::Backup);
    REQUIRE(r1.code != 0);

    // Case 2: empty file
    std::ofstream((tmp / "Backup.parm").string()).close();
    auto r2 = execute_backup(hd, pen, (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r2.code != 0);

    fs::remove_all(tmp);
}

TEST_CASE("backup: file exists only on HD -> copied to pen") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_test_case1";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create file on HD only
    std::ofstream(tmp / "hd" / "ArqX.txt") << "content-x";
    // Parameter file listing ArqX.txt
    std::ofstream(tmp / "Backup.parm") << "ArqX.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);

    // RED expectation: pen should contain the copied file with same content
    REQUIRE(r.code == 0);
    REQUIRE(fs::exists(tmp / "pen" / "ArqX.txt"));
    std::ifstream in(tmp / "pen" / "ArqX.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "content-x");

    fs::remove_all(tmp);
}

TEST_CASE("restore: file exists only on Pen -> copied to HD") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_restore_case1";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create file only on pen
    std::ofstream(tmp / "pen" / "R1.txt") << "restore-me";
    std::ofstream(tmp / "Backup.parm") << "R1.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // RED expectation: should succeed and copy to HD
    REQUIRE(r.code == 0);
    REQUIRE(fs::exists(tmp / "hd" / "R1.txt"));
    std::ifstream in(tmp / "hd" / "R1.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "restore-me");

    fs::remove_all(tmp);
}

TEST_CASE("restore: pen file newer than HD -> update HD") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_restore_case2";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create same-named file in both places
    std::ofstream(tmp / "hd" / "R2.txt") << "old";
    std::ofstream(tmp / "pen" / "R2.txt") << "new";
    // Ensure pen is newer by delaying and touching file
    std::this_thread::sleep_for(50ms);
    std::ofstream(tmp / "pen" / "R2.txt", std::ios::app) << "!"; // modify to bump mtime

    std::ofstream(tmp / "Backup.parm") << "R2.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // RED expectation: should succeed and update HD to match pen
    REQUIRE(r.code == 0);
    REQUIRE(fs::exists(tmp / "hd" / "R2.txt"));
    std::ifstream in_hd(tmp / "hd" / "R2.txt");
    std::string got_hd; std::getline(in_hd, got_hd);
    REQUIRE(got_hd == "new!");

    fs::remove_all(tmp);
}

TEST_CASE("restore: listed file missing on Pen -> error") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_restore_case3";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // File is listed but does not exist on pen
    std::ofstream(tmp / "Backup.parm") << "R3.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    REQUIRE(r.code != 0);

    fs::remove_all(tmp);
}
