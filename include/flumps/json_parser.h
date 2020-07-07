/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_JSON_PARSER_H_
#define FLUMPS_INCLUDE_FLUMPS_JSON_PARSER_H_

#include <utility>

#include <flumps/exception.h>
#include <flumps/json_value_type.h>

#include <flumps/detail/json_parser_impl.h>

namespace flumps {

struct NullJsonVisitor {
  void on_array_start(const char* /*start*/) {}
  void on_array_end(const char* /*start*/, std::size_t /*length*/) {}
  void on_object_start(const char* /*start*/) {}
  void on_object_key(const char* /*start*/, std::size_t /*length*/) {}
  void on_object_end(const char* /*start*/, std::size_t /*length*/) {}
  void on_primitive_value(
    JsonValueType /*type*/, const char* /*start*/, std::size_t /*length*/) {}
};

// TODO: provide helpers to convert primitive value types to C++ types.

template<typename JsonVisitor = NullJsonVisitor>
class JsonParser {
 public:
  explicit JsonParser(
      JsonVisitor visitor = JsonVisitor(),
      unsigned depth_limit = 0u) :
    impl_(std::move(visitor), depth_limit) {
  }

  JsonVisitor& get_visitor() {
    return impl_.get_visitor();
  }

  // Can throw JsonParseError and any exception thrown from JsonVisitor
  // Note: Exceptions of json::DecodeError type thrown from JsonVisitor methods
  //       will be decorated with parse offset and rethrown as JsonParseError
  bool parse_json(const char* begin, const char* end) {
    return impl_.parse_json(begin, end);
  }

 private:
  flumps::detail::JsonParserImpl<JsonVisitor> impl_;
};

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_JSON_PARSER_H_
