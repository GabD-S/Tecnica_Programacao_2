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

TEST_CASE("backup: file exists on both and HD newer -> pen is updated") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_test_case2";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create older file on pen
    {
        std::ofstream(tmp / "pen" / "ArqY.txt") << "old";
    }
    // Ensure timestamp difference
    std::this_thread::sleep_for(1100ms);
    // Create newer file on hd with new content
    {
        std::ofstream(tmp / "hd" / "ArqY.txt") << "new-content";
    }

    // Parameter file listing ArqY.txt
    std::ofstream(tmp / "Backup.parm") << "ArqY.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r.code == 0);

    // pen must be updated to new content
    std::ifstream in(tmp / "pen" / "ArqY.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "new-content");

    // And pen's mtime should be >= hd's mtime after copy
    auto m_hd = fs::last_write_time(tmp / "hd" / "ArqY.txt");
    auto m_pen = fs::last_write_time(tmp / "pen" / "ArqY.txt");
    REQUIRE(m_pen >= m_hd);

    fs::remove_all(tmp);
}

TEST_CASE("backup: listed file missing on HD -> returns error") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_test_case3";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Parameter file lists a file that doesn't exist on HD
    std::ofstream(tmp / "Backup.parm") << "ArqMissing.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    // Decision table: for backup op, if file doesn't exist on HD => error
    REQUIRE(r.code != 0);

    fs::remove_all(tmp);
}
