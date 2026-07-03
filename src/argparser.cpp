#include "headers/argparser.hpp"

std::string argparser::Parser::GetString(char *arr) {
    std::string cu;
    for (size_t index = 0; arr[index] != '\0'; index++) {
        cu += arr[index];
    }

    return cu;
}

void argparser::Parser::Parse(size_t argc, char **argv) {
    for (size_t index = 1; index < argc; index++) {
        std::string cu = GetString(argv[index]);

        if (cu == "-c" || cu == "--create") {
            create = true;
        } else if (cu == "-f" || cu == "--file") {
            archive = GetString(argv[index + 1]);
            ++index;
        } else if ((cu.size() > 7 && cu.substr(0, 7) == "--file=")) {
            archive = cu.substr(7, cu.size() - 7);
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
        } else if (cu == "-c-d" || cu == "--encoding-data") {
            size_t count_bit = atoll(argv[index + 1]);
            size_t count_copy = atoll(argv[index + 2]);
            encoding_data = managefile::Hamarc::EncodingInfo(count_bit, count_copy);
            index += 2;
        } else if (cu == "-c-sf" || cu == "--encoding-size-file") {
            size_t count_bit = atoll(argv[index + 1]);
            size_t count_copy = atoll(argv[index + 2]);
            encoding_size_file = managefile::Hamarc::EncodingInfo(count_bit, count_copy);
            index += 2;
        } else if (cu == "-c-snf" || cu == "--encoding-size-name-file") {
            size_t count_bit = atoll(argv[index + 1]);
            size_t count_copy = atoll(argv[index + 2]);
            encoding_size_name_file = managefile::Hamarc::EncodingInfo(count_bit, count_copy);
            index += 2;
        } else if (cu == "-c-nf" || cu == "--encoding-name-file") {
            size_t count_bit = atoll(argv[index + 1]);
            size_t count_copy = atoll(argv[index + 2]);
            encoding_name_file = managefile::Hamarc::EncodingInfo(count_bit, count_copy);
            index += 2;
        } else if (cu == "-c-edf" || cu == "--encoding-encoding-data-file") {
            size_t count_bit = atoll(argv[index + 1]);
            size_t count_copy = atoll(argv[index + 2]);
            encoding_encoding_data_file = managefile::Hamarc::EncodingInfo(count_bit, count_copy);
            index += 2;
        } else {
<<<<<<< HEAD
            if (cu.size() > 3 && cu.substr(cu.size() - 3, 3) == ".haf") {
=======
            if (cu.size() > 3 && cu.substr(cu.size() - 4, 4) == ".haf") {
>>>>>>> d6641a0 (synch)
                archives.push_back(cu);
            } else {
                files.push_back(cu);
            }
        }
    }
}
