#include <argparser.hpp>

/* template <typename T> argparser::Argument<T> &argparser::Argument<T>::operator=(const Argument &other) {
    short_name = other.short_name;
    long_name = other.long_name;
    pos_name = other.pos_name;

    flag = other.flag;
    data = other.data;

    return *this;
}

template <typename T> argparser::Argument<T>::Argument(const Argument &other) { *this = other; }

template <typename T>
argparser::Argument<T>::Argument(const ShortName &short_name, const LongName &long_name, const PosName &pos_name,
                                 const Counting &many, bool *flag) {
    this->short_name = std::move(short_name.name);
    this->long_name = std::move(long_name.name);
    this->pos_name = std::move(pos_name.name);
    this->many = many;
    this->flag = flag;
}

template <typename T> void argparser::Argument<T>::AddData(const T &element) { memory.PushBack(element); }

template <typename T>
void argparser::ArgParser<T>::Add(const argparser::Argument<T>::ShortName &short_name,
                                  const argparser::Argument<T>::LongName &long_name,
                                  const argparser::Argument<T>::PosName &pos_name,
                                  const argparser::Argument<T>::Counting &many, bool *flag) {}
 */

