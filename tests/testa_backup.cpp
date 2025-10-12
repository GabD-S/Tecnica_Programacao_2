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

TEST_CASE("backup: files have identical timestamps -> do nothing") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_test_case4";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create both files quickly to keep same mtime
    {
        std::ofstream(tmp / "hd" / "ArqSame.txt") << "hd-keep";
        std::ofstream(tmp / "pen" / "ArqSame.txt") << "pen-original";
    }

    // Align timestamps explicitly just in case
    auto ts = fs::last_write_time(tmp / "hd" / "ArqSame.txt");
    fs::last_write_time(tmp / "pen" / "ArqSame.txt", ts);

    std::ofstream(tmp / "Backup.parm") << "ArqSame.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r.code == 0);

    // Expect pen NOT to change because timestamps are equal
    std::ifstream in(tmp / "pen" / "ArqSame.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "pen-original");

    fs::remove_all(tmp);
}

TEST_CASE("backup: file exists only on Pen -> error (HD missing)") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_test_case5";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // File exists only on pen
    std::ofstream(tmp / "pen" / "OnlyPen.txt") << "pen-content";
    std::ofstream(tmp / "Backup.parm") << "OnlyPen.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r.code != 0); // Expect error in backup when HD source missing

    fs::remove_all(tmp);
}

TEST_CASE("backup: nested path on HD -> creates dirs and copies to pen") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_test_case6";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd" / "dir" / "sub");
    fs::create_directories(tmp / "pen");

    // Create nested file only on HD
    {
        std::ofstream(tmp / "hd" / "dir" / "sub" / "Deep.txt") << "deep-content";
    }
    // Param file lists nested relative path
    std::ofstream(tmp / "Backup.parm") << "dir/sub/Deep.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r.code == 0);

    // Expect directory tree created and file copied on pen
    REQUIRE(fs::exists(tmp / "pen" / "dir" / "sub" / "Deep.txt"));
    std::ifstream in(tmp / "pen" / "dir" / "sub" / "Deep.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "deep-content");

    fs::remove_all(tmp);
}

TEST_CASE("backup: multiple entries (HD-only, HD-newer, equal) -> respective actions") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_test_case7";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // A: only on HD -> copy
    std::ofstream(tmp / "hd" / "A.txt") << "A";

    // B: both exist, HD newer -> update pen
    {
        std::ofstream(tmp / "pen" / "B.txt") << "old";
    }
    std::this_thread::sleep_for(1100ms);
    {
        std::ofstream(tmp / "hd" / "B.txt") << "new";
    }

    // C: both exist, equal timestamps -> do nothing (keep pen content)
    {
        std::ofstream(tmp / "hd" / "C.txt") << "hd-keep";
        std::ofstream(tmp / "pen" / "C.txt") << "pen-keep";
    }
    auto tsC = fs::last_write_time(tmp / "hd" / "C.txt");
    fs::last_write_time(tmp / "pen" / "C.txt", tsC);

    // Param file lists all
    std::ofstream parm(tmp / "Backup.parm");
    parm << "A.txt\nB.txt\nC.txt\n";
    parm.close();

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r.code == 0);

    // Validate A copied
    REQUIRE(fs::exists(tmp / "pen" / "A.txt"));
    {
        std::ifstream in(tmp / "pen" / "A.txt");
        std::string got; std::getline(in, got);
        REQUIRE(got == "A");
    }

    // Validate B updated
    {
        std::ifstream in(tmp / "pen" / "B.txt");
        std::string got; std::getline(in, got);
        REQUIRE(got == "new");
    }

    // Validate C unchanged
    {
        std::ifstream in(tmp / "pen" / "C.txt");
        std::string got; std::getline(in, got);
        REQUIRE(got == "pen-keep");
    }

    fs::remove_all(tmp);
}

TEST_CASE("backup: pen newer than HD -> do nothing") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_test_case8";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create older file on HD
    {
        std::ofstream(tmp / "hd" / "PenNew.txt") << "hd-old";
    }
    // Ensure timestamp difference so Pen is newer
    std::this_thread::sleep_for(1100ms);
    {
        std::ofstream(tmp / "pen" / "PenNew.txt") << "pen-new";
    }

    std::ofstream(tmp / "Backup.parm") << "PenNew.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r.code == 0);

    // Expect pen unchanged (should still be pen-new)
    std::ifstream in(tmp / "pen" / "PenNew.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "pen-new");

    // And pen mtime should be >= hd mtime
    auto m_hd = fs::last_write_time(tmp / "hd" / "PenNew.txt");
    auto m_pen = fs::last_write_time(tmp / "pen" / "PenNew.txt");
    REQUIRE(m_pen >= m_hd);

    fs::remove_all(tmp);
}

TEST_CASE("backup: ignore blank lines in Backup.parm") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_test_case9";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create one valid file and some blank/space lines
    std::ofstream(tmp / "hd" / "Valid.txt") << "ok";
    {
        std::ofstream parm(tmp / "Backup.parm");
        parm << "\n   \nValid.txt\n\n";
    }

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);
    REQUIRE(r.code == 0);
    REQUIRE(std::filesystem::exists(tmp / "pen" / "Valid.txt"));

    std::ifstream in(tmp / "pen" / "Valid.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "ok");

    fs::remove_all(tmp);
}
