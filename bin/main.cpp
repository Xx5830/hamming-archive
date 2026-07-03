#include "../src/headers/argparser.hpp"
#include "../src/headers/manager_files.hpp"
#include <filesystem>
#include <iostream>

<<<<<<< HEAD
const managefile::Hamarc::ConfigEncodingFile default_config({8, 1}, {8, 1}, {8,1}, {8, 1});

=======
<<<<<<< Updated upstream
int main(int /*argc*/, char* /*argv*/[]) {
	// TODO: implement HamArc
	return 0;
}
=======
>>>>>>> d6641a0 (synch)
int main(int argc, char **argv) {
    argparser::Parser parser;
    parser.Parse(argc, argv);

<<<<<<< HEAD
=======
    managefile::Hamarc::ConfigEncodingFile config(parser.encoding_size_file, parser.encoding_size_name_file,
                                                  parser.encoding_name_file, parser.encoding_encoding_data_file);
    managefile::Hamarc::EncodingInfo encoding_data = parser.encoding_data;

>>>>>>> d6641a0 (synch)
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
<<<<<<< HEAD
    managefile::Hamarc my_archive(parser.archive, true, default_config);
=======
    managefile::Hamarc my_archive(parser.archive, true, config);
>>>>>>> d6641a0 (synch)

    if (parser.insert || parser.create) {
        for (uint32_t index = 0; index < parser.files.size(); index++) {
            std::string file_name = parser.files[index];
<<<<<<< HEAD
            managefile::File file(file_name);
            if (std::filesystem::exists(file_name)) {
                my_archive.Add(file);
=======

            if (std::filesystem::exists(file_name)) {
                managefile::File file(file_name);
                my_archive.Add(file, encoding_data);
>>>>>>> d6641a0 (synch)
            } else {
                std::cout << "can't add file: " << file_name << std::endl;
            }
        }
    }
    if (parser.concatenate) {
        for (uint32_t index = 0; index < parser.archives.size(); index++) {
            std::string archive_name = parser.archives[index];
<<<<<<< HEAD
            managefile::Hamarc file(archive_name, false, default_config);

            if (file.GetConfig() == my_archive.GetConfig() && std::filesystem::exists(archive_name)) {
=======
            if (std::filesystem::exists(archive_name)) {
                managefile::Hamarc file(archive_name, false, config);
>>>>>>> d6641a0 (synch)
                my_archive.Merge(file);
            } else {
                std::cout << "can't merge archive " << archive_name << std::endl;
            }
        }
    }
    if (parser.extract) {
        for (uint32_t index = 0; index < parser.files.size(); index++) {
            std::string file_name = parser.files[index];
            managefile::File file;
            bool result;
            std::tie(file, result) = my_archive.Get(file_name);
<<<<<<< HEAD
            
            if (result){
                managefile::File result_file(file_name, true, true, false);
                result_file.PushBack(file);
            }

			if (!result){
				std::cout << "archive hasn't file with name: " << file_name << std::endl;
			}
        }

        if (parser.files.size() == 0){
            my_archive.GetAll();
=======

            if (result) {
                managefile::File result_file(file_name, true, false, true);
                result_file.PushBack(file);
            }
            else {
                std::cout << "archive hasn't file with name: " << file_name << std::endl;
            }
        }

        if (parser.files.size() == 0) {
            std::vector <managefile::File> result_files = my_archive.GetAll();
            for (size_t index = 0; index < result_files.size(); index++){
                result_files[index].UnMakeTemp();
            }
>>>>>>> d6641a0 (synch)
        }
    }
    if (parser.erase) {
        for (uint32_t index = 0; index < parser.files.size(); index++) {
<<<<<<< HEAD
            std::string file_name = parser.files[index];;
=======
            std::string file_name = parser.files[index];
            
>>>>>>> d6641a0 (synch)
            my_archive.Delete(file_name);
        }
    }
    if (parser.list) {
        std::vector<std::pair<std::string, size_t>> mas = my_archive.Info();

        for (auto &item : mas) {
<<<<<<< HEAD
            std::cout << "name: \n" << item.first;
            std::cout << "size: \n" << item.second;
=======
            std::cout << "name: \n" << item.first << std::endl;
            std::cout << "size: \n" << item.second << std::endl;
>>>>>>> d6641a0 (synch)
            std::cout << std::endl;
        }
    }

    return 0;
<<<<<<< HEAD
}
=======
}
>>>>>>> Stashed changes
>>>>>>> d6641a0 (synch)
