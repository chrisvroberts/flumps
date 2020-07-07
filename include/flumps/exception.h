/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_EXCEPTION_H_
#define FLUMPS_INCLUDE_FLUMPS_EXCEPTION_H_

#include <stdexcept>
#include <string>

namespace flumps {

struct DecodeError : public std::runtime_error {
  explicit DecodeError(const std::string& msg);
};

struct UnicodeCodePointError : public DecodeError {
  explicit UnicodeCodePointError(const std::string& msg);
};

struct JsonParseError : public DecodeError {
  explicit JsonParseError(const std::string& msg);
};

struct Utf8DecodeError : public DecodeError {
  explicit Utf8DecodeError(const std::string& msg);
};

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_EXCEPTION_H_
