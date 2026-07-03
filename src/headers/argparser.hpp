#include <string>
#include <vector>
#include "manager_files.hpp"

#ifndef INCLUDE_ARG_PARSER
#define INCLUDE_ARG_PARSER

namespace argparser {

struct Parser{
    bool create;
    bool list;
    bool extract;
    bool insert;
    bool erase;
    bool concatenate;
    managefile::Hamarc::EncodingInfo encoding_data;
    managefile::Hamarc::EncodingInfo encoding_size_file;
    managefile::Hamarc::EncodingInfo encoding_size_name_file;
    managefile::Hamarc::EncodingInfo encoding_name_file;
    managefile::Hamarc::EncodingInfo encoding_encoding_data_file;

    std::vector<std::string> files;
    std::vector<std::string> archives;
    std::string archive;

    inline Parser(){
        create = list = extract = insert = erase = concatenate = 0;
<<<<<<< HEAD
        encoding_data = managefile::Hamarc::EncodingInfo(8, 1);
=======
        encoding_data = managefile::Hamarc::EncodingInfo(256, 1);
>>>>>>> d6641a0 (synch)
        encoding_size_file = managefile::Hamarc::EncodingInfo(8, 1);
        encoding_size_name_file = managefile::Hamarc::EncodingInfo(8, 1);
        encoding_name_file = managefile::Hamarc::EncodingInfo(8, 1);
        encoding_encoding_data_file = managefile::Hamarc::EncodingInfo(8, 1);
    }

    std::string GetString(char* arr);
    
    void Parse(size_t argc, char** argv);
};

} // namespace argparser

#endif