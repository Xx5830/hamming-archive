#include <utility>
#include <type_traits>

#ifndef INCLUDE_ERROR
#define INCLUDE_ERROR

namespace error {

class Error {
    const char *error;

  public:
    Error(const char *text) { error = text; }
    bool IsError() const { return error == nullptr; }
    const char *GetError() { return error; }
};

template <typename T>
concept moveable = requires (T a) {std::move(a);};

template <typename T> requires moveable<T> class OptionError : public Error {
    T value;

    OptionError& operator =(const OptionError &other) = delete;
    OptionError() = delete;
    OptionError(const OptionError &other) = delete;
  public:
    OptionError(T &&value, const char *text) : Error::error(text), value(std::move(value)){} 
    T GetValue() { return value; }
};

} // namespace error

#endif