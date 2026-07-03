#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace fs = std::filesystem;

static std::string quote_path(const fs::path& p) {
	fs::path native_path = p;
	native_path.make_preferred();
	std::string s = native_path.string();
#ifdef _WIN32
	if (s.find(' ') != std::string::npos) {
		return "\"" + s + "\"";
	}
	return s;
#else
	return "\"" + s + "\"";
#endif
}

static bool filesEqual(const fs::path& a, const fs::path& b) {
	if (!fs::exists(a) || !fs::exists(b)) return false;
	if (fs::file_size(a) != fs::file_size(b)) return false;

	std::ifstream fa(a, std::ios::binary);
	std::ifstream fb(b, std::ios::binary);
	if (!fa || !fb) return false;

	const std::size_t kBufferSize = 1 << 20;
	std::vector<char> ba(kBufferSize), bb(kBufferSize);

	while (fa && fb) {
		fa.read(ba.data(), static_cast<std::streamsize>(ba.size()));
		fb.read(bb.data(), static_cast<std::streamsize>(bb.size()));
		const auto ca = fa.gcount();
		const auto cb = fb.gcount();
		if (ca != cb) return false;
		if (std::memcmp(ba.data(), bb.data(), static_cast<std::size_t>(ca)) != 0) return false;
	}
	return fa.eof() && fb.eof();
}

TEST(HamArcCLI, CreateAndExtractAndCompare) {
	const fs::path resourcesDir = fs::path(RESOURCES_DIR);
	const fs::path file1 = resourcesDir / "BjarneStroustrup.jpg";
	const fs::path file2 = resourcesDir / "Book.pdf";

	ASSERT_TRUE(fs::exists(file1));
	ASSERT_TRUE(fs::exists(file2));

	const auto tempRoot = fs::temp_directory_path();
	const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	const fs::path work = tempRoot / ("hamarc_test_" + std::to_string(now));
	const fs::path outDir = work / "out";
	ASSERT_TRUE(fs::create_directories(outDir));

	const fs::path archive = work / "archive.haf";
	const std::string hamarc = HAMARC_EXE_PATH;

	{
		std::ostringstream cmd;
		cmd << quote_path(hamarc)
		    << " --create"
		    << " --file=" << quote_path(archive)
		    << " " << quote_path(file1)
		    << " " << quote_path(file2);
		std::cout << "Create command: " << cmd.str() << std::endl;
		const int rc = std::system(cmd.str().c_str());
		std::cout << "Return code: " << rc << std::endl;
		ASSERT_EQ(rc, 0);
		ASSERT_TRUE(fs::exists(archive));
	}

	{
		const std::uintmax_t archive_size = fs::file_size(archive);
		std::fstream archive_file(archive, std::ios::in | std::ios::out | std::ios::binary);
		ASSERT_TRUE(archive_file.is_open());

		std::vector<std::pair<std::uintmax_t, int>> flipped_bits = {
			{100, 0},
			{archive_size / 2, 0},
			{archive_size - 1, 0}
		};

		for (const auto& [byte_pos, bit_pos] : flipped_bits) {
			archive_file.seekg(static_cast<std::streamoff>(byte_pos));
			char byte = 0;
			archive_file.read(&byte, 1);
			if (archive_file.gcount() != 1) continue;

			byte ^= (1 << bit_pos);

			archive_file.seekp(static_cast<std::streamoff>(byte_pos));
			archive_file.write(&byte, 1);
			archive_file.flush();
		}

		archive_file.close();
	}

	{
		const fs::path original_dir = fs::current_path();
		fs::current_path(outDir);
		
		std::ostringstream cmd;
		cmd << quote_path(hamarc)
		    << " --extract"
		    << " --file=" << quote_path(fs::path("..") / archive.filename());
		const int rc = std::system(cmd.str().c_str());
		
		fs::current_path(original_dir);
		ASSERT_EQ(rc, 0);
	}

	const fs::path extr1 = outDir / file1.filename();
	const fs::path extr2 = outDir / file2.filename();
	ASSERT_TRUE(fs::exists(extr1));
	ASSERT_TRUE(fs::exists(extr2));

	EXPECT_TRUE(filesEqual(file1, extr1));
	EXPECT_TRUE(filesEqual(file2, extr2));
}

/* TEST(HamArcCLI, CreateAndListFiles) {
    const fs::path resourcesDir = fs::path(RESOURCES_DIR);
    const fs::path file1 = resourcesDir / "file1.txt";
    const fs::path file2 = resourcesDir / "file2.txt";

    // Создаем тестовые файлы
    std::ofstream f1(file1);
    f1 << "Test content 1";
    f1.close();
    
    std::ofstream f2(file2);
    f2 << "Test content 2";
    f2.close();

    const auto tempRoot = fs::temp_directory_path();
    const auto work = tempRoot / ("hamarc_test_" + std::to_string(std::time(nullptr)));
    fs::create_directories(work);
    
    const fs::path archive = work / "archive.haf";
    const std::string hamarc = HAMARC_EXE_PATH;

    // Создаем архив
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --create"
            << " --file=" << quote_path(archive)
            << " " << quote_path(file1)
            << " " << quote_path(file2);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Проверяем список файлов
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --list"
            << " --file=" << quote_path(archive);
        
        testing::internal::CaptureStdout();
        int rc = std::system(cmd.str().c_str());
        std::string output = testing::internal::GetCapturedStdout();
        
        ASSERT_EQ(rc, 0);
        ASSERT_NE(output.find(file1.filename().string()), std::string::npos);
        ASSERT_NE(output.find(file2.filename().string()), std::string::npos);
    }

    // Удаляем тестовые файлы
    fs::remove(file1);
    fs::remove(file2);
}

TEST(HamArcCLI, AddAndDeleteFiles) {
    const fs::path resourcesDir = fs::path(RESOURCES_DIR);
    
    // Создаем тестовые файлы
    const fs::path file1 = resourcesDir / "temp1.txt";
    const fs::path file2 = resourcesDir / "temp2.txt";
    const fs::path file3 = resourcesDir / "temp3.txt";

    std::ofstream(file1) << "Content 1";
    std::ofstream(file2) << "Content 2";
    std::ofstream(file3) << "Content 3";

    const auto work = fs::temp_directory_path() / ("hamarc_test_" + std::to_string(std::time(nullptr)));
    fs::create_directories(work);
    
    const fs::path archive = work / "archive.haf";
    const std::string hamarc = HAMARC_EXE_PATH;

    // Создаем архив с двумя файлами
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --create --file=" << quote_path(archive)
            << " " << quote_path(file1) << " " << quote_path(file2);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Добавляем третий файл
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --append --file=" << quote_path(archive)
            << " " << quote_path(file3);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Проверяем что все три файла в архиве
    {
        testing::internal::CaptureStdout();
        std::ostringstream cmd;
        cmd << quote_path(hamarc) << " --list --file=" << quote_path(archive);
        std::system(cmd.str().c_str());
        std::string output = testing::internal::GetCapturedStdout();
        
        ASSERT_NE(output.find(file1.filename().string()), std::string::npos);
        ASSERT_NE(output.find(file2.filename().string()), std::string::npos);
        ASSERT_NE(output.find(file3.filename().string()), std::string::npos);
    }

    // Удаляем второй файл
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --delete --file=" << quote_path(archive)
            << " " << quote_path(file2.filename());
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Проверяем что остались только первый и третий
    {
        testing::internal::CaptureStdout();
        std::ostringstream cmd;
        cmd << quote_path(hamarc) << " --list --file=" << quote_path(archive);
        std::system(cmd.str().c_str());
        std::string output = testing::internal::GetCapturedStdout();
        
        ASSERT_NE(output.find(file1.filename().string()), std::string::npos);
        ASSERT_EQ(output.find(file2.filename().string()), std::string::npos);
        ASSERT_NE(output.find(file3.filename().string()), std::string::npos);
    }

    // Очистка
    fs::remove(file1);
    fs::remove(file2);
    fs::remove(file3);
}

TEST(HamArcCLI, ExtractSpecificFile) {
    const fs::path resourcesDir = fs::path(RESOURCES_DIR);
    
    // Создаем тестовые файлы
    const fs::path file1 = resourcesDir / "specific1.txt";
    const fs::path file2 = resourcesDir / "specific2.txt";
    
    std::ofstream(file1) << "Specific file 1 content";
    std::ofstream(file2) << "Specific file 2 content";

    const auto work = fs::temp_directory_path() / ("hamarc_test_" + std::to_string(std::time(nullptr)));
    const fs::path outDir = work / "extracted";
    fs::create_directories(outDir);
    
    const fs::path archive = work / "archive.haf";
    const std::string hamarc = HAMARC_EXE_PATH;

    // Создаем архив
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --create --file=" << quote_path(archive)
            << " " << quote_path(file1) << " " << quote_path(file2);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Извлекаем только один файл
    {
        const fs::path original_dir = fs::current_path();
        fs::current_path(outDir);
        
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --extract --file=" << quote_path(fs::path("..") / archive.filename())
            << " " << quote_path(file1.filename());
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
        
        fs::current_path(original_dir);
    }

    // Проверяем что извлекся только указанный файл
    const fs::path extr1 = outDir / file1.filename();
    const fs::path extr2 = outDir / file2.filename();
    
    ASSERT_TRUE(fs::exists(extr1));
    ASSERT_FALSE(fs::exists(extr2));
    EXPECT_TRUE(filesEqual(file1, extr1));

    // Очистка
    fs::remove(file1);
    fs::remove(file2);
}

TEST(HamArcCLI, ConcatenateArchives) {
    const fs::path resourcesDir = fs::path(RESOURCES_DIR);
    
    // Создаем тестовые файлы
    const fs::path file1 = resourcesDir / "concat1.txt";
    const fs::path file2 = resourcesDir / "concat2.txt";
    const fs::path file3 = resourcesDir / "concat3.txt";
    
    std::ofstream(file1) << "Archive 1 file 1";
    std::ofstream(file2) << "Archive 1 file 2";
    std::ofstream(file3) << "Archive 2 file 1";

    const auto work = fs::temp_directory_path() / ("hamarc_test_" + std::to_string(std::time(nullptr)));
    fs::create_directories(work);
    
    const fs::path archive1 = work / "archive1.haf";
    const fs::path archive2 = work / "archive2.haf";
    const fs::path merged = work / "merged.haf";
    const std::string hamarc = HAMARC_EXE_PATH;

    // Создаем первый архив
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --create --file=" << quote_path(archive1)
            << " " << quote_path(file1) << " " << quote_path(file2);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Создаем второй архив
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --create --file=" << quote_path(archive2)
            << " " << quote_path(file3);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Объединяем архивы
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --concatenate " << quote_path(archive1) << " " << quote_path(archive2)
            << " --file=" << quote_path(merged);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Проверяем содержимое объединенного архива
    {
        testing::internal::CaptureStdout();
        std::ostringstream cmd;
        cmd << quote_path(hamarc) << " --list --file=" << quote_path(merged);
        std::system(cmd.str().c_str());
        std::string output = testing::internal::GetCapturedStdout();
        
        ASSERT_NE(output.find(file1.filename().string()), std::string::npos);
        ASSERT_NE(output.find(file2.filename().string()), std::string::npos);
        ASSERT_NE(output.find(file3.filename().string()), std::string::npos);
    }

    // Очистка
    fs::remove(file1);
    fs::remove(file2);
    fs::remove(file3);
}

TEST(HamArcCLI, HeavyCorruptionRecovery) {
    const fs::path resourcesDir = fs::path(RESOURCES_DIR);
    const fs::path file = resourcesDir / "important_data.bin";

    // Создаем тестовый бинарный файл
    {
        std::ofstream f(file, std::ios::binary);
        std::vector<uint8_t> data(1000);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<uint8_t>(i % 256);
        }
        f.write(reinterpret_cast<const char*>(data.data()), data.size());
        f.close();
    }

    const auto work = fs::temp_directory_path() / ("hamarc_test_" + std::to_string(std::time(nullptr)));
    const fs::path outDir = work / "out";
    fs::create_directories(outDir);
    
    const fs::path archive = work / "archive.haf";
    const std::string hamarc = HAMARC_EXE_PATH;

    // Создаем архив
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --create --file=" << quote_path(archive)
            << " " << quote_path(file);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Вносим множественные повреждения
    {
        const std::uintmax_t archive_size = fs::file_size(archive);
        std::fstream archive_file(archive, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(archive_file.is_open());

        // Повреждаем каждый 10-й байт
        for (std::uintmax_t i = 0; i < archive_size; i += 10) {
            archive_file.seekg(static_cast<std::streamoff>(i));
            char byte = 0;
            archive_file.read(&byte, 1);
            if (archive_file.gcount() != 1) continue;

            // Инвертируем несколько битов
            byte ^= 0x55; // 01010101

            archive_file.seekp(static_cast<std::streamoff>(i));
            archive_file.write(&byte, 1);
        }
        archive_file.close();
    }

    // Пытаемся извлечь
    {
        const fs::path original_dir = fs::current_path();
        fs::current_path(outDir);
        
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --extract"
            << " --file=" << quote_path(fs::path("..") / archive.filename());
        int rc = std::system(cmd.str().c_str());
        
        fs::current_path(original_dir);
        
        // Должен или успешно извлечь, или сообщить об ошибке
        // но не упасть
        ASSERT_TRUE(rc == 0 || rc != 255); // 255 обычно указывает на критическую ошибку
    }

    // Очистка
    fs::remove(file);
}

TEST(HamArcCLI, EmptyArchiveOperations) {
    const auto work = fs::temp_directory_path() / ("hamarc_test_" + std::to_string(std::time(nullptr)));
    fs::create_directories(work);
    
    const fs::path archive = work / "empty.haf";
    const std::string hamarc = HAMARC_EXE_PATH;

    // Создаем пустой архив
    {
        std::ostringstream cmd;
        cmd << quote_path(hamarc)
            << " --create --file=" << quote_path(archive);
        ASSERT_EQ(std::system(cmd.str().c_str()), 0);
    }

    // Проверяем список файлов пустого архива
    {
        testing::internal::CaptureStdout();
        std::ostringstream cmd;
        cmd << quote_path(hamarc) << " --list --file=" << quote_path(archive);
        int rc = std::system(cmd.str().c_str());
        std::string output = testing::internal::GetCapturedStdout();
        
        ASSERT_EQ(rc, 0);
        // Должен быть пустой вывод или сообщение о пустом архиве
    }
}
 */