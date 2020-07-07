/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_JSON_CALLBACK_VISITOR_H_
#define FLUMPS_INCLUDE_FLUMPS_JSON_CALLBACK_VISITOR_H_

#include <cstddef>
#include <functional>
#include <string>

#include <flumps/json_value_path.h>
#include <flumps/json_value_type.h>

#include <flumps/detail/json_callback_visitor_impl.h>

namespace flumps {

using JsonValueCallback =
  std::function<void(
    JsonValueType type,
    const std::string& path,
    const char* data,
    std::size_t length)>;

class JsonCallbackVisitor {
 public:
  JsonCallbackVisitor() :
    impl_() {
  }

  void register_callback(
      const JsonValuePath& path,
      JsonValueCallback callback) {
    impl_.register_callback(path, callback);
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
  flumps::detail::JsonCallbackVisitorImpl impl_;
};

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_JSON_CALLBACK_VISITOR_H_
