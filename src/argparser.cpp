#include "headers/argparser.hpp"
#include <cstdlib>
#include <stdexcept>

namespace argparser {

Parser::Parser()
    : create(false), list(false), extract(false), insert(false), erase(false), concatenate(false),
      encoding_data(256, 1), encoding_size_file(8, 1), encoding_size_name_file(8, 1), encoding_name_file(8, 1),
      encoding_encoding_data_file(8, 1) {}

std::string Parser::ArgToString(char* arr) { return std::string(arr); }

managefile::Hamarc::EncodingInfo Parser::ParseEncodingArg(int& index, int argc, char** argv) {
    if (index + 2 >= argc) {
        throw std::invalid_argument("encoding flag requires two arguments: <bits> <copies>");
    }
    size_t bits = static_cast<size_t>(std::atoll(argv[index + 1]));
    size_t copies = static_cast<size_t>(std::atoll(argv[index + 2]));
    index += 2;
    return managefile::Hamarc::EncodingInfo(bits, static_cast<uint32_t>(copies));
}

void Parser::Parse(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
        std::string cu = ArgToString(argv[index]);

        if (cu == "-c" || cu == "--create") {
            create = true;
        } else if (cu == "-l" || cu == "--list") {
            list = true;
        } else if (cu == "-x" || cu == "--extract") {
            extract = true;
        } else if (cu == "-a" || cu == "--append") {
            insert = true;
        } else if (cu == "-d" || cu == "--delete") {
            erase = true;
        } else if (cu == "-A" || cu == "--concatenate") {
            concatenate = true;

        } else if (cu == "-f" || cu == "--file") {
            if (index + 1 < argc) {
                archive = ArgToString(argv[++index]);
            }
        } else if (cu.size() > 7 && cu.substr(0, 7) == "--file=") {
            archive = cu.substr(7);

        } else if (cu == "--encoding-data" || cu == "-e-d") {
            encoding_data = ParseEncodingArg(index, argc, argv);
        } else if (cu == "--encoding-size-file" || cu == "-e-sf") {
            encoding_size_file = ParseEncodingArg(index, argc, argv);
        } else if (cu == "--encoding-size-name" || cu == "-e-sn") {
            encoding_size_name_file = ParseEncodingArg(index, argc, argv);
        } else if (cu == "--encoding-name" || cu == "-e-nf") {
            encoding_name_file = ParseEncodingArg(index, argc, argv);
        } else if (cu == "--encoding-meta" || cu == "-e-m") {
            encoding_encoding_data_file = ParseEncodingArg(index, argc, argv);

        } else {
            if (cu.size() > 4 && cu.substr(cu.size() - 4) == ".haf") {
                archives.push_back(cu);
            } else {
                files.push_back(cu);
            }
        }
    }
}

} // namespace argparser
