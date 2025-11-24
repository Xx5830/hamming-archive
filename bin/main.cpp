#include "../src/headers/argparser.hpp"
#include "../src/headers/manager_files.hpp"
#include <filesystem>
#include <iostream>

int main(int argc, char *argv[]) {
    argparser::Parser parser;
    parser.Parse(argc, argv);

    if (parser.archive.size() == 0) {
        std::cerr << "need name archive" << std::endl;
        return -1;
    }
    if (!parser.create) {
        if (!std::filesystem::exists(parser.archive)) {
            std::cerr << "don't have archive .haf with name " << parser.archive << std::endl;
            return -1;
        }
    }
    managefile::Hamarc my_archive(parser.archive);

    if (parser.insert || parser.create) {
        for (uint32_t index = 0; index < parser.files.size(); index++) {
            std::string file_name = parser.files[index];
            managefile::File file(file_name);
            if (std::filesystem::exists(file_name)) {
                my_archive.AddFile(file);
            } else {
                std::cout << "can't add file: " << file_name << std::endl;
            }
        }
    }
    if (parser.concatenate) {
        for (uint32_t index = 0; index < parser.archives.size(); index++) {
            std::string archive_name = parser.archives[index];
            managefile::Hamarc file(archive_name);

            if (std::filesystem::exists(archive_name)) {
                my_archive.Merge(file);
            } else {
                std::cout << "can't marge archive " << archive_name << std::endl;
            }
        }
    }
    if (parser.extract) {
        for (uint32_t index = 0; index < parser.files.size(); index++) {
            std::string file_name = parser.files[index];
            bool result = my_archive.Extract(file_name);

			if (!result){
				std::cout << "archive hasn't file with name: " << file_name << std::endl;
			}
        }

        if (parser.files.size() == 0){
            my_archive.ExtractAll();
        }
    }
    if (parser.erase) {
        for (uint32_t index = 0; index < parser.files.size(); index++) {
            std::string file_name = parser.files[index];
            bool result = my_archive.DeleteFile(file_name);

            if (!result) {
                std::cout << "can't delete file " << file_name << std::endl;
            }
        }
    }
    if (parser.list) {
        std::vector<std::pair<std::string, long long>> mas = my_archive.GetInfo();

        for (auto &item : mas) {
            std::cout << "name: \n" << item.first;
            std::cout << "size: \n" << item.second;
            std::cout << std::endl;
        }
    }

    return 0;
}
