/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_ECHO_VISITOR_IMPL_H_
#define FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_ECHO_VISITOR_IMPL_H_

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

#include <flumps/json_parser.h>

namespace flumps {

namespace detail {

template<typename OutputIterator>
class JsonEchoVisitorImpl {
 private:
  enum class state : char {
    root,
    start_array,
    in_array,
    start_object,
    first_key_seen,
    in_object,
    subsequent_key
  };
  const bool pp_;
  const std::string indent_;
  OutputIterator out_;
  void output_new_line() {
    if (pp_) {
      output_char('\n');
      for (auto i = stack.size(); i > 1u; --i) {
        output_string(indent_.data(), indent_.size());
      }
    }
  }
  void output_string(const char* start, std::size_t length) {
    std::copy(start, start+length, out_);
  }
  void output_char(char ch) {
    *out_ = ch;
    ++out_;
  }

 public:
  explicit JsonEchoVisitorImpl(OutputIterator out, std::string indent) :
    pp_(true),
    indent_(indent),
    out_(std::move(out)),
    stack(1u, static_cast<char>(state::root)) {
  }
  // TODO: hold back this output until paired object end or next key (whichever
  //       comes first. if the object is actually empty just output as '{}'.
  //       Same applied to on_array_start and [].
  void on_object_start(const char* /*start*/) {
    if (is_back(state::in_array)) {
      output_char(',');
      output_new_line();
    } else if (is_back(state::in_object)) {
      output_char(',');
      output_new_line();
    }
    stack_push(state::start_object);
    output_char('{');
    output_new_line();
  }
  void on_object_key(const char* start, std::size_t length) {
    if (is_back(state::in_object)) {
      set_back(state::subsequent_key);
      output_char(',');
      output_new_line();
    } else {
      set_back(state::first_key_seen);
    }

    output_char('"');
    output_string(start, length);
    output_char('"');
    output_char(':');
    if (pp_) output_char(' ');
  }
  void on_object_end(const char* /*start*/, std::size_t /*length*/) {
    stack.pop_back();
    output_new_line();
    output_char('}');
    if (is_back(state::start_array)) set_back(state::in_array);
    if (is_back(state::first_key_seen)) set_back(state::in_object);
    if (is_back(state::subsequent_key)) set_back(state::in_object);
  }
  void on_array_start(const char* /*start*/) {
    if (is_back(state::in_array)) {
      output_char(',');
      output_new_line();
    } else if (is_back(state::in_object)) {
      output_char(',');
      output_new_line();
    }
    stack_push(state::start_array);
    output_char('[');
    output_new_line();
  }
  void on_array_end(const char* /*start*/, std::size_t /*length*/) {
    stack.pop_back();
    output_new_line();
    output_char(']');
    if (is_back(state::start_array)) set_back(state::in_array);
    if (is_back(state::first_key_seen)) set_back(state::in_object);
    if (is_back(state::subsequent_key)) set_back(state::in_object);
  }
  void on_primitive_value(
    JsonValueType type, const char* start, std::size_t length) {
    if (is_back(state::in_array)) {
      output_char(',');
      output_new_line();
    }
    if (type == JsonValueType::String) output_char('"');
    output_string(start, length);
    if (type == JsonValueType::String) output_char('"');
    if (is_back(state::start_array)) set_back(state::in_array);
    if (is_back(state::first_key_seen)) set_back(state::in_object);
    if (is_back(state::subsequent_key)) set_back(state::in_object);
  }

 private:
  bool is_back(state s) {
    return static_cast<state>(stack.back()) == s;
  }
  state stack_back() const {
    return static_cast<state>(stack.back());
  }
  void stack_push(state s) {
    stack.push_back(static_cast<char>(s));
  }
  void set_back(state s) {
    stack.back() = static_cast<char>(s);
  }
  std::string stack;
};

}  // end namespace detail

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_ECHO_VISITOR_IMPL_H_
