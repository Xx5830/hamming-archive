#include <concepts>
#include <string>
#include <vector>

#ifndef INCLUDE_ARG_PARSER
#define INCLUDE_ARG_PARSER

namespace argparser {

/* template <typename SameType> class SameLinkedList {
    struct Nil {};

    template <typename Head, typename Tail> struct Cons {
        using head = Head;
        using tail = Tail;
    };

    template <class T, class Tail>
    using add = Cons<T, Tail>;

    template <typename T, typename TList> struct AppendImpl {
        using type = Cons<typename TList::head, typename AppendImpl<T, typename TList::tail>::Type>;
    };

    template <typename T> struct AppendImpl<T, Nil> {
        using type = Cons<T, Nil>;
    };

    template <class T, class TList> using push_back = typename Cons<T, TList>::type;
} */

/* struct Argument {
    struct ShortName{
        std::string name;
    };
    struct LongName{
        std::string name;
    };
    struct PosName{
        std::string name;
    };
    enum Counting{
        ZeroOne,
        ZeroMany,
        OneOne,
        OneMany
    };
    Counting many;
    std::string short_name;
    std::string long_name;
    std::string pos_name;

    bool *flag;
    
    template <typename T>
    List<T> data;

    Argument& operator=(const Argument &other);

    Argument(const Argument &other);
    explicit Argument(const ShortName &short_name, const LongName &long_name, const PosName &pos_name, const Counting &many, bool *flag);

    void AddData(const T &element);
};

class ArgParser {
    List<Argument> arguments;

    void Add(const ShortName &short_name, const LongName &long_name, const PosName &pos_name, const Counting &many, bool *flag){
        Argument item(short_name, long_name, pos_name, many, flag);
        Arguments.PushBack(item);
    }

    List<T>::Node* GetNode(const std::string &short_name, const std::string &long_name, const std::string &pos_name) const{
        List<T>::Node* node = arguments.begin();

        while (node != nullptr){
            if (short_name.size() && node->short_name.size() && short_name == node->short_name){
                return node;
            }
            else if (long_name.size() && node->long_name.size() && long_name == node->short_name){
                return node;
            }
            else if (pos_name.size() && node->short_name.size() && pos_name == node->short_name){
                return node;
            }
        }

        return node;
    }
}; */

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

    std::string GetString(char* arr){
        std::string cu;
        for (size_t index = 0; arr[index] != '\0'; index++){
            cu += arr[index];
        }

        return cu;
    }
    
    void Parse(size_t argc, char** argv){
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
};

} // namespace argparser

#endif