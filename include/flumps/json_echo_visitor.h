/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_JSON_ECHO_VISITOR_H_
#define FLUMPS_INCLUDE_FLUMPS_JSON_ECHO_VISITOR_H_

#include <cstddef>
#include <string>
#include <utility>

#include <flumps/json_value_type.h>

#include <flumps/detail/json_echo_visitor_impl.h>

namespace flumps {

template<typename OutputIterator>
class JsonEchoVisitor {
 public:
  explicit JsonEchoVisitor(OutputIterator out, std::string indent = "  ") :
    impl_(std::move(out), std::move(indent)) {
  }

  void on_object_start(const char* start) {
    impl_.on_object_start(start);
  }

  void on_object_key(const char* start, std::size_t length) {
    impl_.on_object_key(start, length);
  }

  void on_object_end(const char* start, std::size_t length) {
    impl_.on_object_end(start, length);
  }

  void on_array_start(const char* start) {
    impl_.on_array_start(start);
  }

  void on_array_end(const char* start, std::size_t length) {
    impl_.on_array_end(start, length);
  }

  void on_primitive_value(
      JsonValueType type, const char* start, std::size_t length) {
    impl_.on_primitive_value(type, start, length);
  }

 private:
  flumps::detail::JsonEchoVisitorImpl<OutputIterator> impl_;
};

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_JSON_ECHO_VISITOR_H_
