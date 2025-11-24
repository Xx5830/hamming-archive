#include <string>
#include <vector>

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

    std::vector<std::string> files;
    std::vector<std::string> archives;
    std::string archive;

    inline Parser(){
        create = list = extract = insert = erase = concatenate = 0;
    }

    std::string GetString(char* arr);
    
    void Parse(size_t argc, char** argv);
};

} // namespace argparser

#endif