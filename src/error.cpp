#include "error.hpp"

error::Error::Error(const char *text) { error = text; }

bool error::Error::IsError() const { return error == nullptr; }

const char *error::Error::GetError() { return error; }

template <typename T> error::OptionError<T>::OptionError(T &&value, const char *text) {}

template <typename T> inline T error::OptionError<T>::GetValue() { return value; }
