#include <argparser.hpp>

std::string argparser::Parser::GetString(char *arr) {
    std::string cu;
    for (size_t index = 0; arr[index] != '\0'; index++){
        cu += arr[index];
    }

    return cu;
}

void argparser::Parser::Parse(size_t argc, char **argv) {
    for (size_t index = 1; index < argc; index++){
        std::string cu = GetString(argv[index]);

        if (cu == "-c" || cu == "--create"){
            create = true;
        }
        else if (cu == "-f" || cu == "--file"){
            archive = GetString(argv[index + 1]);
        }
        else if ((cu.size() > 7 && cu.substr(0, 7) == "--file=")){
            archive = cu.substr(7, cu.size() - 7);
        }
        else if (cu == "-l" || cu == "--list"){
            list = true;
        }
        else if (cu == "-x" || cu == "--extract"){
            extract = true;
        }
        else if (cu == "-a" || cu == "--append"){
            insert = true;
        }
        else if (cu == "-d" || cu == "--delete"){
            erase = true;
        }
        else if (cu == "-A" || cu == "--concatenate"){
            concatenate = true;
        }
        else{
            if (cu.size() > 3 && cu.substr(cu.size() - 3, 3) == ".haf"){
                archives.push_back(cu);
            }
            else{
                files.push_back(cu);
            }
        }
    }
}
