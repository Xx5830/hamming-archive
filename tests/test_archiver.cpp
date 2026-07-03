#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string QuotePath(const fs::path& p) {
    fs::path native = p;
    native.make_preferred();
    return "\"" + native.string() + "\"";
}

static bool FilesEqual(const fs::path& a, const fs::path& b) {
    if (!fs::exists(a) || !fs::exists(b)) return false;
    if (fs::file_size(a) != fs::file_size(b)) return false;

    std::ifstream fa(a, std::ios::binary);
    std::ifstream fb(b, std::ios::binary);
    if (!fa || !fb) return false;

    const std::size_t kBuf = 1 << 20;
    std::vector<char> ba(kBuf), bb(kBuf);
    while (fa && fb) {
        fa.read(ba.data(), static_cast<std::streamsize>(kBuf));
        fb.read(bb.data(), static_cast<std::streamsize>(kBuf));
        auto ca = fa.gcount(), cb = fb.gcount();
        if (ca != cb) return false;
        if (std::memcmp(ba.data(), bb.data(), static_cast<std::size_t>(ca)) != 0) return false;
    }
    return fa.eof() && fb.eof();
}

static fs::path MakeTempDir() {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path dir = fs::temp_directory_path() / ("hamarc_test_" + std::to_string(now));
    fs::create_directories(dir);
    return dir;
}

static fs::path WriteTestFile(const fs::path& dir, const std::string& name,
                               const std::string& content) {
    fs::path p = dir / name;
    std::ofstream(p) << content;
    return p;
}

static fs::path WriteBinaryFile(const fs::path& dir, const std::string& name, size_t size) {
    fs::path p = dir / name;
    std::ofstream f(p, std::ios::binary);
    for (size_t i = 0; i < size; ++i) {
        f.put(static_cast<char>(i % 256));
    }
    return p;
}

static int RunHamarc(const std::string& args) {
    std::string cmd = QuotePath(HAMARC_EXE_PATH) + " " + args;
    return std::system(cmd.c_str());
}

TEST(HamArcCLI, CreateAndExtractTextFiles) {
    const fs::path work = MakeTempDir();
    const fs::path out  = work / "out";
    fs::create_directories(out);

    fs::path f1 = WriteTestFile(work, "hello.txt", "Hello, HamArc!\n");
    fs::path f2 = WriteTestFile(work, "world.txt", "World content here.\n");

    fs::path archive = work / "test.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(archive) +
                        " " + QuotePath(f1) + " " + QuotePath(f2)), 0);
    ASSERT_TRUE(fs::exists(archive));

    {
        fs::path saved = fs::current_path();
        fs::current_path(out);
        ASSERT_EQ(RunHamarc("--extract --file=" + QuotePath(fs::path("..") / archive.filename())), 0);
        fs::current_path(saved);
    }

    EXPECT_TRUE(FilesEqual(f1, out / f1.filename()));
    EXPECT_TRUE(FilesEqual(f2, out / f2.filename()));
}

TEST(HamArcCLI, ListFiles) {
    const fs::path work = MakeTempDir();
    fs::path f1 = WriteTestFile(work, "alpha.txt", "aaa");
    fs::path f2 = WriteTestFile(work, "beta.txt",  "bbb");
    fs::path archive = work / "list.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(archive) +
                        " " + QuotePath(f1) + " " + QuotePath(f2)), 0);

    // Перенаправить вывод в временный файл
    fs::path out_log = work / "list.txt";
    int rc = std::system((QuotePath(HAMARC_EXE_PATH) + " --list --file=" +
                          QuotePath(archive) + " > " + QuotePath(out_log)).c_str());
    ASSERT_EQ(rc, 0);

    std::ifstream log(out_log);
    std::string content((std::istreambuf_iterator<char>(log)),
                         std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("alpha.txt"), std::string::npos);
    EXPECT_NE(content.find("beta.txt"),  std::string::npos);
}

TEST(HamArcCLI, AppendThenDelete) {
    const fs::path work = MakeTempDir();
    fs::path f1 = WriteTestFile(work, "keep.txt",   "keep me");
    fs::path f2 = WriteTestFile(work, "remove.txt", "remove me");
    fs::path f3 = WriteTestFile(work, "extra.txt",  "extra");
    fs::path archive = work / "mod.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(archive) +
                        " " + QuotePath(f1) + " " + QuotePath(f2)), 0);

    ASSERT_EQ(RunHamarc("--append --file=" + QuotePath(archive) +
                        " " + QuotePath(f3)), 0);

    ASSERT_EQ(RunHamarc("--delete --file=" + QuotePath(archive) +
                        " remove.txt"), 0);

    const fs::path out = work / "out";
    fs::create_directories(out);
    {
        fs::path saved = fs::current_path();
        fs::current_path(out);
        ASSERT_EQ(RunHamarc("--extract --file=" + QuotePath(fs::path("..") / archive.filename())), 0);
        fs::current_path(saved);
    }

    EXPECT_TRUE(fs::exists(out / "keep.txt"));
    EXPECT_FALSE(fs::exists(out / "remove.txt"));
    EXPECT_TRUE(fs::exists(out / "extra.txt"));
}

TEST(HamArcCLI, ExtractSpecificFile) {
    const fs::path work = MakeTempDir();
    fs::path f1 = WriteTestFile(work, "one.txt", "one");
    fs::path f2 = WriteTestFile(work, "two.txt", "two");
    fs::path archive = work / "spec.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(archive) +
                        " " + QuotePath(f1) + " " + QuotePath(f2)), 0);

    const fs::path out = work / "out";
    fs::create_directories(out);
    {
        fs::path saved = fs::current_path();
        fs::current_path(out);
        ASSERT_EQ(RunHamarc("--extract --file=" + QuotePath(fs::path("..") / archive.filename()) +
                            " one.txt"), 0);
        fs::current_path(saved);
    }

    EXPECT_TRUE(fs::exists(out / "one.txt"));
    EXPECT_FALSE(fs::exists(out / "two.txt"));
    EXPECT_TRUE(FilesEqual(f1, out / "one.txt"));
}

TEST(HamArcCLI, ConcatenateArchives) {
    const fs::path work = MakeTempDir();
    fs::path fa1 = WriteTestFile(work, "arc1_file.txt", "from archive 1");
    fs::path fa2 = WriteTestFile(work, "arc2_file.txt", "from archive 2");

    fs::path arc1   = work / "a1.haf";
    fs::path arc2   = work / "a2.haf";
    fs::path merged = work / "merged.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(arc1) + " " + QuotePath(fa1)), 0);
    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(arc2) + " " + QuotePath(fa2)), 0);
    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(merged) +
                        " --concatenate " + QuotePath(arc1) + " " + QuotePath(arc2)), 0);

    const fs::path out = work / "out";
    fs::create_directories(out);
    {
        fs::path saved = fs::current_path();
        fs::current_path(out);
        ASSERT_EQ(RunHamarc("--extract --file=" + QuotePath(fs::path("..") / merged.filename())), 0);
        fs::current_path(saved);
    }

    EXPECT_TRUE(FilesEqual(fa1, out / fa1.filename()));
    EXPECT_TRUE(FilesEqual(fa2, out / fa2.filename()));
}

TEST(HamArcCLI, BinaryRoundTrip) {
    const fs::path work = MakeTempDir();
    const fs::path out  = work / "out";
    fs::create_directories(out);

    fs::path bin = WriteBinaryFile(work, "data.bin", 4096);
    fs::path archive = work / "bin.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(archive) + " " + QuotePath(bin)), 0);

    {
        fs::path saved = fs::current_path();
        fs::current_path(out);
        ASSERT_EQ(RunHamarc("--extract --file=" + QuotePath(fs::path("..") / archive.filename())), 0);
        fs::current_path(saved);
    }

    EXPECT_TRUE(FilesEqual(bin, out / bin.filename()));
}

TEST(HamArcCLI, EmptyArchiveList) {
    const fs::path work = MakeTempDir();
    fs::path archive = work / "empty.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(archive)), 0);
    ASSERT_TRUE(fs::exists(archive));
    EXPECT_EQ(RunHamarc("--list --file=" + QuotePath(archive)), 0);
}

TEST(HamArcCLI, CustomEncodingFlags) {
    const fs::path work = MakeTempDir();
    const fs::path out  = work / "out";
    fs::create_directories(out);

    fs::path f = WriteTestFile(work, "custom.txt", "custom encoding test");
    fs::path archive = work / "custom.haf";

    ASSERT_EQ(RunHamarc("--create --file=" + QuotePath(archive) +
                        " --encoding-data 16 3 " + QuotePath(f)), 0);

    {
        fs::path saved = fs::current_path();
        fs::current_path(out);
        ASSERT_EQ(RunHamarc("--extract --file=" + QuotePath(fs::path("..") / archive.filename())), 0);
        fs::current_path(saved);
    }

    EXPECT_TRUE(FilesEqual(f, out / f.filename()));
}
