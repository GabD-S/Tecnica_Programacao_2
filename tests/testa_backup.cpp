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

TEST_CASE("restore: trim spaces in parameter entries") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_restore_case4";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create file on pen with exact clean name
    std::ofstream(tmp / "pen" / "R4.txt") << "trim-me";

    // Parameter file with leading/trailing spaces and blank lines
    std::ofstream parm(tmp / "Backup.parm");
    parm << "   R4.txt   \n";
    parm << "\n"; // extra blank line
    parm.close();

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    REQUIRE(r.code == 0);
    REQUIRE(fs::exists(tmp / "hd" / "R4.txt"));
    std::ifstream in(tmp / "hd" / "R4.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "trim-me");

    fs::remove_all(tmp);
}

TEST_CASE("restore: ignore comment lines in parameter file") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_restore_case5";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Prepare files
    std::ofstream(tmp / "pen" / "R5.txt") << "comment-ok";

    // Parameter file with comments and a valid entry
    std::ofstream parm(tmp / "Backup.parm");
    parm << "# This is a comment\n";
    parm << "; Another comment style\n";
    parm << " R5.txt \n"; // single valid entry surrounded by spaces
    parm.close();

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);
    REQUIRE(r.code == 0);
    REQUIRE(fs::exists(tmp / "hd" / "R5.txt"));
    std::ifstream in(tmp / "hd" / "R5.txt");
    std::string got; std::getline(in, got);
    REQUIRE(got == "comment-ok");

    fs::remove_all(tmp);
}

TEST_CASE("restore: preserve timestamps from pen to HD") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_restore_case6";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Create pen file and set a known timestamp
    auto penFile = tmp / "pen" / "R6.txt";
    std::ofstream(penFile) << "ts-sync";
    auto t_ref = fs::file_time_type::clock::now();
    // ensure a small delay then set mtime to t_ref
    std::this_thread::sleep_for(10ms);
    fs::last_write_time(penFile, t_ref);

    std::ofstream(tmp / "Backup.parm") << "R6.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);
    REQUIRE(r.code == 0);

    auto hdFile = tmp / "hd" / "R6.txt";
    REQUIRE(fs::exists(hdFile));
    auto t_hd = fs::last_write_time(hdFile);
    auto t_pen = fs::last_write_time(penFile);
    // Expect exact match
    REQUIRE(t_hd == t_pen);

    fs::remove_all(tmp);
}

TEST_CASE("restore: continue after first missing but return error") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_restore_case7";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Prepare: first missing, second exists
    std::ofstream(tmp / "pen" / "B.txt") << "available";
    std::ofstream(tmp / "Backup.parm") << "A.txt\nB.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // RED expectation: should copy B.txt to HD, but return non-zero due to missing A.txt
    REQUIRE(r.code != 0);
    REQUIRE(fs::exists(tmp / "hd" / "B.txt"));
    REQUIRE_FALSE(fs::exists(tmp / "hd" / "A.txt"));

    fs::remove_all(tmp);
}

TEST_CASE("restore: error when HD destination is not writable") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_restore_case8";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Prepare source on pen
    auto src = tmp / "pen" / "LOCK.txt";
    std::ofstream(src) << "locked";
    std::ofstream(tmp / "Backup.parm") << "LOCK.txt\n";

    // Create destination file and make it read-only (no write permission)
    auto dst = tmp / "hd" / "LOCK.txt";
    std::ofstream(dst) << "old";
    // Ensure src is newer so restore attempts to update
    auto newer = fs::file_time_type::clock::now();
    fs::last_write_time(src, newer);
    fs::last_write_time(dst, newer - std::chrono::seconds(10));
    fs::permissions(dst, fs::perms::owner_read, fs::perm_options::replace);

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // RED: Expect non-zero due to write failure and content unchanged
    REQUIRE(r.code != 0);
    std::ifstream in(dst);
    std::string got; std::getline(in, got);
    REQUIRE(got == "old");

    // cleanup: restore permissions to delete
    fs::permissions(dst, fs::perms::owner_all, fs::perm_options::add);
    fs::remove_all(tmp);
}

TEST_CASE("restore: ignore directory entries in parameter file") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_restore_case9";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen" / "DIR");

    // Put a file inside the directory
    std::ofstream(tmp / "pen" / "DIR" / "inside.txt") << "inside";

    // Parameter lists the directory name, not the file
    std::ofstream(tmp / "Backup.parm") << "DIR\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // RED expectation: either success or a defined error, but must NOT copy contents of the directory implicitly
    REQUIRE_FALSE(fs::exists(tmp / "hd" / "DIR" / "inside.txt"));
    fs::remove_all(tmp);
}

TEST_CASE("restore: equal timestamps -> no action") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_restore_case10";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    auto penFile = tmp / "pen" / "EQ.txt";
    auto hdFile = tmp / "hd" / "EQ.txt";

    // Different contents
    std::ofstream(hdFile) << "old";
    std::ofstream(penFile) << "new";

    // Set the same timestamp on both
    auto t = fs::file_time_type::clock::now();
    fs::last_write_time(hdFile, t);
    fs::last_write_time(penFile, t);

    std::ofstream(tmp / "Backup.parm") << "EQ.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // Expect success and no change to HD content
    REQUIRE(r.code == 0);
    std::string got;
    std::ifstream in(hdFile); std::getline(in, got);
    REQUIRE(got == "old");

    fs::remove_all(tmp);
}

TEST_CASE("restore: mixed scenario (pen-only, pen-newer, equal, missing, unwritable, nested)") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_restore_mixed";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // pen-only: P_ONLY.txt
    std::ofstream(tmp / "pen" / "P_ONLY.txt") << "only";

    // pen-newer: NEW.txt (exists in both, pen newer)
    auto hd_new = tmp / "hd" / "NEW.txt";
    auto pen_new = tmp / "pen" / "NEW.txt";
    std::ofstream(hd_new) << "old";
    std::ofstream(pen_new) << "new";
    auto now = fs::file_time_type::clock::now();
    fs::last_write_time(hd_new, now - 2s);
    fs::last_write_time(pen_new, now);

    // equal timestamps: EQ2.txt
    auto hd_eq = tmp / "hd" / "EQ2.txt";
    auto pen_eq = tmp / "pen" / "EQ2.txt";
    std::ofstream(hd_eq) << "hd_eq";
    std::ofstream(pen_eq) << "pen_eq";
    auto t_eq = now - 5s;
    fs::last_write_time(hd_eq, t_eq);
    fs::last_write_time(pen_eq, t_eq);

    // missing on pen: MISS.txt (listed but absent)

    // unwritable HD: LOCK2.txt (exists in both, pen newer, but HD not writable)
    auto hd_lock = tmp / "hd" / "LOCK2.txt";
    auto pen_lock = tmp / "pen" / "LOCK2.txt";
    std::ofstream(hd_lock) << "old";
    std::ofstream(pen_lock) << "new_locked";
    fs::last_write_time(hd_lock, now - 3s);
    fs::last_write_time(pen_lock, now);
    fs::permissions(hd_lock, fs::perms::owner_read, fs::perm_options::replace);

    // nested path: DIRX/sub/file.txt (pen-only nested)
    fs::create_directories(tmp / "pen" / "DIRX" / "sub");
    std::ofstream(tmp / "pen" / "DIRX" / "sub" / "file.txt") << "nested";

    // Build parameter list
    std::ofstream parm(tmp / "Backup.parm");
    parm << "P_ONLY.txt\n";
    parm << "NEW.txt\n";
    parm << "EQ2.txt\n";
    parm << "MISS.txt\n";
    parm << "LOCK2.txt\n";
    parm << "DIRX/sub/file.txt\n";
    parm.close();

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // Expect write error to dominate (code 5), but other valid copies still happen
    REQUIRE(r.code != 0);
    // pen-only copied
    REQUIRE(fs::exists(tmp / "hd" / "P_ONLY.txt"));
    // pen-newer updated
    std::string new_content; { std::ifstream in(hd_new); std::getline(in, new_content);} 
    REQUIRE(new_content.find("new") != std::string::npos);
    // equal unchanged
    std::string eq_content; { std::ifstream in(hd_eq); std::getline(in, eq_content);} 
    REQUIRE(eq_content == "hd_eq");
    // missing not created
    REQUIRE_FALSE(fs::exists(tmp / "hd" / "MISS.txt"));
    // nested created and copied
    std::string nested_content; { std::ifstream in(tmp / "hd" / "DIRX" / "sub" / "file.txt"); std::getline(in, nested_content);} 
    REQUIRE(nested_content == "nested");
    // unwritable remained old
    std::string lock_content; { std::ifstream in(hd_lock); std::getline(in, lock_content);} 
    REQUIRE(lock_content == "old");

    // cleanup: restore permissions
    fs::permissions(hd_lock, fs::perms::owner_all, fs::perm_options::add);
    fs::remove_all(tmp);
}

TEST_CASE("restore: error precedence (write error over missing)") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_restore_err_prec";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Missing entry
    // Also create a pair that will cause a write error: pen newer, HD read-only
    auto hd_lock = tmp / "hd" / "LOCKE.txt";
    auto pen_lock = tmp / "pen" / "LOCKE.txt";
    std::ofstream(hd_lock) << "old";
    std::ofstream(pen_lock) << "new";
    auto now = fs::file_time_type::clock::now();
    fs::last_write_time(hd_lock, now - 2s);
    fs::last_write_time(pen_lock, now);
    fs::permissions(hd_lock, fs::perms::owner_read, fs::perm_options::replace);

    std::ofstream(tmp / "Backup.parm") << "MISSING_X.txt\nLOCKE.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Restore);

    // Expect write error (5) to dominate over missing (4)
    REQUIRE(r.code == 5);

    // cleanup permissions
    fs::permissions(hd_lock, fs::perms::owner_all, fs::perm_options::add);
    fs::remove_all(tmp);
}

TEST_CASE("backup: mixed scenario (hd-only, hd-newer, equal, missing, pen-newer, nested)") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_backup_mixed";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    auto now = fs::file_time_type::clock::now();

    // hd-only: H_ONLY.txt on HD
    std::ofstream(tmp / "hd" / "H_ONLY.txt") << "only";

    // hd-newer: NEW_B.txt exists in both, HD newer
    auto hd_new = tmp / "hd" / "NEW_B.txt";
    auto pen_new = tmp / "pen" / "NEW_B.txt";
    std::ofstream(hd_new) << "hd_new";
    std::ofstream(pen_new) << "pen_old";
    fs::last_write_time(hd_new, now);
    fs::last_write_time(pen_new, now - 2s);

    // equal timestamps: EQ_B.txt
    auto hd_eq = tmp / "hd" / "EQ_B.txt";
    auto pen_eq = tmp / "pen" / "EQ_B.txt";
    std::ofstream(hd_eq) << "old";
    std::ofstream(pen_eq) << "pen_eq";
    auto t_eq = now - 5s;
    fs::last_write_time(hd_eq, t_eq);
    fs::last_write_time(pen_eq, t_eq);

    // missing on HD: MISS_B.txt (listed but absent on HD)

    // pen newer than HD: PEN_NEWER.txt (should no-op in backup)
    auto hd_pn = tmp / "hd" / "PEN_NEWER.txt";
    auto pen_pn = tmp / "pen" / "PEN_NEWER.txt";
    std::ofstream(hd_pn) << "old";
    std::ofstream(pen_pn) << "new";
    fs::last_write_time(hd_pn, now - 2s);
    fs::last_write_time(pen_pn, now);

    // nested path: DIRB/sub/file.txt (HD only)
    fs::create_directories(tmp / "hd" / "DIRB" / "sub");
    std::ofstream(tmp / "hd" / "DIRB" / "sub" / "file.txt") << "nested_b";

    // Build parameter list
    std::ofstream parm(tmp / "Backup.parm");
    parm << "H_ONLY.txt\n";
    parm << "NEW_B.txt\n";
    parm << "EQ_B.txt\n";
    parm << "MISS_B.txt\n";
    parm << "PEN_NEWER.txt\n";
    parm << "DIRB/sub/file.txt\n";
    parm.close();

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);

    // Expectations:
    // hd-only copied
    REQUIRE(fs::exists(tmp / "pen" / "H_ONLY.txt"));
    // hd-newer updated
    std::string new_content; { std::ifstream in(pen_new); std::getline(in, new_content);} 
    REQUIRE(new_content.find("hd_new") != std::string::npos);
    // equal unchanged (Pen keeps pen_eq)
    std::string eq_content; { std::ifstream in(pen_eq); std::getline(in, eq_content);} 
    REQUIRE(eq_content == "pen_eq");
    // missing on HD not created and should mark non-zero (we'll accept either behavior now, but prefer non-zero once implemented)
    // pen newer than HD no-op
    std::string pn_content; { std::ifstream in(pen_pn); std::getline(in, pn_content);} 
    REQUIRE(pn_content == "new");
    // nested created and copied
    std::string nested_content; { std::ifstream in(tmp / "pen" / "DIRB" / "sub" / "file.txt"); std::getline(in, nested_content);} 
    REQUIRE(nested_content == "nested_b");

    fs::remove_all(tmp);
}

TEST_CASE("backup: continue after missing on HD but return error") {
    namespace fs = std::filesystem;
    fs::path tmp = fs::current_path() / "_tmp_backup_acc_missing";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // One existing on HD, one missing
    std::ofstream(tmp / "hd" / "A_b.txt") << "available";
    std::ofstream(tmp / "Backup.parm") << "B_miss.txt\nA_b.txt\n"; // first missing, then available

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);

    // RED: Expect non-zero due to missing B_miss.txt, but A_b.txt should be copied to Pen
    REQUIRE(r.code != 0);
    REQUIRE(fs::exists(tmp / "pen" / "A_b.txt"));
    REQUIRE_FALSE(fs::exists(tmp / "pen" / "B_miss.txt"));

    fs::remove_all(tmp);
}

TEST_CASE("backup: error when writing to Pen returns 5 and continues") {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    fs::path tmp = fs::current_path() / "_tmp_backup_write_error";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "hd");
    fs::create_directories(tmp / "pen");

    // Unwritable file on Pen: force update by making HD newer
    auto hd_lock = tmp / "hd" / "LOCKB.txt";
    auto pen_lock = tmp / "pen" / "LOCKB.txt";
    std::ofstream(hd_lock) << "new_content";
    std::ofstream(pen_lock) << "old_content";
    auto now = fs::file_time_type::clock::now();
    fs::last_write_time(pen_lock, now - 2s);
    fs::last_write_time(hd_lock, now);
    fs::permissions(pen_lock, fs::perms::owner_read, fs::perm_options::replace);

    // A second file that should still be copied successfully
    std::ofstream(tmp / "hd" / "OK.txt") << "ok";

    std::ofstream(tmp / "Backup.parm") << "LOCKB.txt\nOK.txt\n";

    auto r = execute_backup((tmp / "hd").string(), (tmp / "pen").string(), (tmp / "Backup.parm").string(), Operation::Backup);

    REQUIRE(r.code == 5);
    // Ensure the second file was processed
    REQUIRE(fs::exists(tmp / "pen" / "OK.txt"));
    // The unwritable file should remain unchanged
    std::string lock_content; { std::ifstream in(pen_lock); std::getline(in, lock_content);} 
    REQUIRE(lock_content == "old_content");

    // Cleanup permissions
    fs::permissions(pen_lock, fs::perms::owner_all, fs::perm_options::add);
    fs::remove_all(tmp);
}
