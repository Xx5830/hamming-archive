#include "../src/headers/argparser.hpp"
#include "../src/headers/manager_files.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    argparser::Parser parser;
    try {
        parser.Parse(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "hamarc: argument error: " << ex.what() << "\n";
        return 1;
    }

    if (parser.archive.empty()) {
        std::cerr << "hamarc: archive name is required (-f / --file=)\n";
        return 1;
    }
    if (!parser.create && !std::filesystem::exists(parser.archive)) {
        std::cerr << "hamarc: archive not found: " << parser.archive << "\n";
        return 1;
    }

    managefile::Hamarc::ConfigEncodingFile config(
        parser.encoding_size_file,
        parser.encoding_size_name_file,
        parser.encoding_name_file,
        parser.encoding_encoding_data_file
    );
    managefile::Hamarc::EncodingInfo encoding_data = parser.encoding_data;

    managefile::Hamarc my_archive(parser.archive, true, config);

    if (parser.create || parser.insert) {
        for (const auto& file_name : parser.files) {
            if (!std::filesystem::exists(file_name)) {
                std::cerr << "hamarc: file not found: " << file_name << "\n";
                continue;
            }
            managefile::File f(file_name, false);
            my_archive.Add(f, encoding_data);
        }
    }

    if (parser.concatenate) {
        for (const auto& arc_name : parser.archives) {
            if (!std::filesystem::exists(arc_name)) {
                std::cerr << "hamarc: archive not found: " << arc_name << "\n";
                continue;
            }
            managefile::Hamarc other(arc_name, false, config);
            if (!my_archive.Merge(other)) {
                std::cerr << "hamarc: skipped incompatible archive: " << arc_name << "\n";
            }
        }
    }

    if (parser.extract) {
        if (parser.files.empty()) {
            auto all = my_archive.GetAll();
            for (auto& f : all) {
                f.UnMakeTemp();
            }
        } else {
            for (const auto& file_name : parser.files) {
                auto [extracted, found] = my_archive.Get(file_name);
                if (!found) {
                    std::cerr << "hamarc: not in archive: " << file_name << "\n";
                    continue;
                }
                managefile::File out(file_name, true, false, true);
                out.PushBack(extracted);
            }
        }
    }

    if (parser.erase) {
        for (const auto& file_name : parser.files) {
            my_archive.Delete(file_name);
        }
    }

    if (parser.list) {
        auto entries = my_archive.Info();
        for (const auto& [name, size] : entries) {
            std::cout << name << "\t" << size << " bytes\n";
        }
    }

    return 0;
}
