#include <concepts>
#include "List.hpp";

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
;

template <typename T> struct Argument {
    const char *short_name = nullptr;
    const char *long_name = nullptr;

    List<bool *> flags;
    List<T *> vars;

    Argument& operator=(const Argument<T> &other) {
        short_name = other.short_name;
        long_name = other.long_name;

        flags = other.flags;
        vars = other.vars;

        return *this;
    }

    Argument(const Argument &other){
        *this = other;
    }
};

template <typename T> class ArgParser {
    List<Argument> arguments;
};

} // namespace argparser

#endif