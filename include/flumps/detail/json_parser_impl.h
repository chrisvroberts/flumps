/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_PARSER_IMPL_H_
#define FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_PARSER_IMPL_H_

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <flumps/exception.h>
#include <flumps/json_value_path.h>
#include <flumps/json_value_type.h>
#include <flumps/string_helpers.h>

namespace flumps {

namespace detail {

template<typename JsonVisitor>
class JsonParserImpl {
 private:
  enum class ParseElement { initial, parse_value };
  enum class ParseValue { initial, parse_array, parse_object };
  enum class ParseArray { initial, element_read, end_array };
  enum class ParseObject { initial, member_read, object_read };
  struct State {
    ParseElement parse_element = ParseElement::initial;
    ParseValue parse_value = ParseValue::initial;
    ParseArray parse_array = ParseArray::initial;
    ParseObject parse_object = ParseObject::initial;
    const char* start = nullptr;
  };
  std::vector<State> state_;

 public:
  explicit JsonParserImpl(
      JsonVisitor visitor = JsonVisitor(),
      unsigned depth_limit = 0u) :
    state_(),
    visitor_(std::move(visitor)),
    depth_(0u),
    depth_limit_(depth_limit) {
  }
  JsonVisitor& get_visitor() {
    return visitor_;
  }
  // Can throw JsonParseError and any exception thrown from JsonVisitor
  // Note: Exceptions of json::DecodeError type thrown from JsonVisitor methods
  //       will be decorated with parse offset and rethrown as JsonParseError
  bool parse_json(const char* begin, const char* end) {
    const char* pos = begin;
    state_.clear();
    inc_depth();
    try {
      do {
        auto call_again = parse_element(&pos, end);
        if (!call_again) dec_depth();
        if (state_.empty() && pos != end)
          throw JsonParseError("Invalid: extra data present after JSON");
      }  while (pos != end);
      if (!state_.empty()) throw JsonParseError("Invalid: JSON truncated");
    } catch (const DecodeError& e) {
      throw JsonParseError(
        e.what() + std::string(" (offset=") + std::to_string(pos-begin) + ")");
    }
    const bool complete_parse = pos == end;
    return complete_parse;
  }

 private:
  JsonVisitor visitor_;
  unsigned depth_;
  const unsigned depth_limit_;
  void inc_depth() {
    state_.emplace_back();
    if (depth_limit_ > 0u && state_.size() > depth_limit_) {
      throw JsonParseError(
        "Maximum JSON nesting depth (" + std::to_string(depth_limit_) +
        ") reached");
    }
  }
  void dec_depth() {
    state_.pop_back();
  }

  bool parse_element(const char** pos, const char* end) {
    switch (state_.back().parse_element) {
    case ParseElement::initial:
      chomp_whitespace(pos, end);
      state_.back().parse_element = ParseElement::parse_value;
      // fall through
    case ParseElement::parse_value:
      auto call_again = parse_value(pos, end);
      if (call_again) return call_again;
      chomp_whitespace(pos, end);
    }
    return false;
  }
  void chomp_whitespace(const char** pos, const char* end) {
    while (*pos != end && is_whitespace(**pos)) {
      ++*pos;
    }
  }
  bool is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
  }
  bool parse_value(const char** pos, const char* end) {
    do {
      switch (state_.back().parse_value) {
      case ParseValue::initial:
      {
        if (*pos == end) throw JsonParseError("End of data: value expected");
        if (**pos == '{') {
          state_.back().parse_value = ParseValue::parse_object;
          break;
        } else if (**pos == '[') {
          state_.back().parse_value = ParseValue::parse_array;
          break;
        } else if (**pos == '"') {
          const char* start = *pos;
          parse_string(pos, end);
          visitor_.on_primitive_value(
            JsonValueType::String, start+1u, (*pos-start)-2u);
        } else if (**pos == '-' || is_dec(**pos)) {
          const char* start = *pos;
          parse_number(pos, end);
          visitor_.on_primitive_value(JsonValueType::Number, start, *pos-start);
        } else if ((end-*pos) >= 4 && 0 == std::strncmp("true", *pos, 4u)) {
          visitor_.on_primitive_value(JsonValueType::True, *pos, 4u);
          *pos += 4u;
        } else if ((end-*pos) >= 4 && 0 == std::strncmp("null", *pos, 4u)) {
          visitor_.on_primitive_value(JsonValueType::Null, *pos, 4u);
          *pos += 4u;
        } else if ((end-*pos) >= 5 && 0 == std::strncmp("false", *pos, 5u)) {
          visitor_.on_primitive_value(JsonValueType::False, *pos, 5u);
          *pos += 5u;
        } else {
          throw JsonParseError("Invalid: value expected");
        }
        return false;
      }
      case ParseValue::parse_object:
      {
        return parse_object(pos, end);
      }
      case ParseValue::parse_array:
      {
        return parse_array(pos, end);
      }
      }
    } while (*pos != end);
    return false;
  }
  bool parse_object(const char** pos, const char* end) {
    do {
      switch (state_.back().parse_object) {
      case ParseObject::initial:
      {
        state_.back().start = *pos;
        visitor_.on_object_start(state_.back().start);
        ++*pos;  // checked and known to be present and '{' by caller
        chomp_whitespace(pos, end);  // to handle empty object case
        if (*pos == end || **pos != '}') {
          parse_member_key(pos, end);
          state_.back().parse_object = ParseObject::member_read;
          inc_depth();
          return true;  // read element
        } else {
          state_.back().parse_object = ParseObject::object_read;
        }
        break;
      }
      case ParseObject::member_read:
      {
        if (*pos != end) {
          if (**pos == '}') {
            state_.back().parse_object = ParseObject::object_read;
          } else if (**pos == ',') {
            ++*pos;
            parse_member_key(pos, end);
            inc_depth();
            return true;  // read element
          } else {
            throw JsonParseError(
              "Invalid: end of object or comma expected");
          }
        } else {
          throw JsonParseError(
            "End of data: end of object or comma expected");
        }
        break;
      }
      case ParseObject::object_read:
        ++*pos;  // step over '}'
        visitor_.on_object_end(state_.back().start, *pos-state_.back().start);
        return false;
      }
    } while (*pos != end);
    return false;
  }
  void parse_member_key(const char** pos, const char* end) {
    chomp_whitespace(pos, end);
    if (*pos == end) throw JsonParseError(
      "End of data: object key expected");
    if (**pos != '"') throw JsonParseError(
      "Invalid: object key string expected");
    const char* start = *pos;
    parse_string(pos, end);
    // Exclude quotes from object member key
    visitor_.on_object_key(start+1u, (*pos-start)-2u);
    chomp_whitespace(pos, end);
    if (*pos == end || **pos != ':') throw JsonParseError(
      "Invalid: object member colon expected");
    ++*pos;
  }
  bool parse_array(const char** pos, const char* end) {
    do {
      switch (state_.back().parse_array) {
      case ParseArray::initial:
      {
        state_.back().start = *pos;
        visitor_.on_array_start(state_.back().start);
        ++*pos;  // checked and known to be present and '[' by caller
        chomp_whitespace(pos, end);  // to handle empty array case
        if (*pos == end || **pos != ']') {
          state_.back().parse_array = ParseArray::element_read;
          inc_depth();
          return true;  // read element
        } else {
          state_.back().parse_array = ParseArray::end_array;
        }
        break;
      }
      case ParseArray::element_read:
      {
        if (*pos != end) {
          if (**pos == ']') {
            state_.back().parse_array = ParseArray::end_array;
          } else if (**pos == ',') {
            ++*pos;
            inc_depth();
            return true;  // read element
          } else {
            throw JsonParseError(
              "Invalid: end of array or comma expected");
          }
        } else {
          throw JsonParseError(
            "End of data: end of array or comma expected");
        }
        break;
      }
      case ParseArray::end_array:
      {
        ++*pos;  // step over ']'
        visitor_.on_array_end(state_.back().start, *pos-state_.back().start);
        return false;
      }
      }
    } while (*pos != end);
    return false;
  }
  void parse_string(const char** pos, const char* end) {
    ++*pos;  // checked and known to be present and '"' by caller
    auto sur_tracker = utf8_surrogate_checker();
    auto last_cp_was_hex_seq = false;
    while (*pos != end && **pos != '"') {
      if (**pos != '\\') {
        if (**pos >= '\0' && **pos <= '\x1f') throw JsonParseError(
          "Invalid: control characters must be escaped as \\uxxxx");
        (void)chomp_utf8_char(pos, end);  // throws Utf8DecodeError
      } else {
        ++*pos;
        if (*pos == end) throw JsonParseError(
          "End of data: partial escape sequence");
        static const std::string escape_char = "\"\\/bfnrtu";
        if (std::string::npos == escape_char.find(**pos)) throw JsonParseError(
          "Invalid: invalid escape sequence");
        if (**pos != 'u') {
          ++*pos;
        } else {
          ++*pos;
          const auto code_point = json_hex_seq_to_code_point(pos, end);
          sur_tracker.check_code_point(code_point);
          last_cp_was_hex_seq = true;
        }
      }
      if (!last_cp_was_hex_seq) {
        sur_tracker.check_code_point();
      }
    }
    sur_tracker.check_code_point();  // Last tick to check for trailing low sur
    if (*pos == end) throw JsonParseError(
      "End of data: no string '\"' terminator reached");
    ++*pos;  // step over '"'
  }
  void parse_number(const char** pos, const char* end) {
    // *pos checked and known to be present and '-' or '0'-'9' by caller
    // Decode integer...
    if (**pos == '-') {
      ++*pos;
      if (*pos == end) throw JsonParseError(
        "End of data: no integer part following leading '-'");
    }
    if (!is_dec(**pos)) throw JsonParseError(
      "Invalid: non-decimal in integer part");
    if (**pos == '0') {
      ++*pos;
    } else {
      ++*pos;
      while (*pos != end && is_dec(**pos)) ++*pos;  // chomp digits
    }
    if (*pos == end) return;  // end of integer
    if (**pos == '.') {
      // Decode fraction
      ++*pos;
      if (*pos == end) throw JsonParseError(
        "End of data: no digits following decimal point");
      if (!is_dec(**pos)) throw JsonParseError(
        "Invalid: digit expected following decimal point");
      ++*pos;
      while (*pos != end && is_dec(**pos)) ++*pos;  // chomp digits
    }
    if (*pos == end) return;  // end of integer.fraction
    if (**pos == 'e' || **pos == 'E') {
      // Decode exponent
      ++*pos;
      if (*pos == end) throw JsonParseError(
        "End of data: no exponent following 'e'");
      if (**pos == '-' || **pos == '+') ++*pos;
      if (*pos == end) throw JsonParseError(
        "End of data: no exponent digit following 'e(+|-|)'");
      if (!is_dec(**pos)) throw JsonParseError(
        "Invalid: digit expected following 'e(+|-|)'");
      ++*pos;
      while (*pos != end && is_dec(**pos)) ++*pos;  // chomp digits
    }
  }
  bool is_dec(char ch) {
    return ch >= '0' && ch <= '9';
  }
};

}  // end namespace detail

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_PARSER_IMPL_H_
