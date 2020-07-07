/*
   Chris Roberts, 12 February 2020
*/

#include <flumps/exception.h>

namespace flumps {

DecodeError::DecodeError(const std::string& msg) :
  std::runtime_error(msg) {
}

UnicodeCodePointError::UnicodeCodePointError(const std::string& msg) :
  DecodeError(msg) {
}

JsonParseError::JsonParseError(const std::string& msg) : DecodeError(msg) {
}

Utf8DecodeError::Utf8DecodeError(const std::string& msg) : DecodeError(msg) {
}

}  // end namespace flumps
