/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_STRING_HELPERS_H_
#define FLUMPS_INCLUDE_FLUMPS_STRING_HELPERS_H_

#include <stdexcept>
#include <string>

#include <flumps/exception.h>

namespace flumps {

struct utf8_surrogate_checker {
  unsigned last_code_point = 0u;
  bool surrogate_low_needed = false;
  // Use default param if known to be non-surrogate cp. Returns true if code
  // point is complete (this is available via last_code_point) and false if
  // you are part way through a surrogate pair i.e. !surrogate_low_needed.
  // Throws UnicodeCodePointError is code_point is invalid.
  bool check_code_point(unsigned code_point = 0u);
};

// Parse a single UTF-8 encoded code point from *pos. pos is updated and left
// at the next unconsumed character or, if an error is encountered, at the
// position of that error. No data is consumed beyond 'end'. Throws
// Utf8DecodeError or UnicodeCodePointError. Returns code point.
unsigned chomp_utf8_char(const char** pos, const char* end);

// Given the valid content of a JSON string (i.e. excluding enclosing double-
// quotes) a UTF-8 encoded string is returned. If JSON input has been validated
// then does not throw. Otherwise may throw JsonParseError or
// UnicodeCodePointError.
std::string json_string_content_to_utf8(const char* begin, const char* end);
std::string json_string_content_to_utf8(const std::string& input);

// Parse the 4 character hex string associated with a \u JSON string escape
// sequence. pos is moved to the next character to parse. The code point is
// returned. Throws JsonParseError.
unsigned json_hex_seq_to_code_point(const char** pos, const char* end);

// Given a valid UTF-8 encoded string, JSON encoded string content (i.e.
// excluding enclosing double-quotes) is returned. Does not throw if input is
// valid UTF-8. If input is not valid UTF-8 then output may be invalid JSON
// string content or may throw Utf8DecodeError or UnicodeCodePointError.
std::string utf8_to_json_string_content(const char* begin, const char* end);
std::string utf8_to_json_string_content(const std::string& input);

// Given a string which may not be valid UTF-8, first verify if it is valid
// UTF-8. If it is then call utf8_to_json_string_content, otherwise first force
// input string to UTF-8 by interpreting it as ISO-8859-1 (latin-1) and then
// calling utf8_to_json_string_content. Does not throw.
std::string to_json_string_content_safe(const char* begin, const char* end);
std::string to_json_string_content_safe(const std::string& input);

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_STRING_HELPERS_H_
