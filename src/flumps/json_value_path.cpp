/*
   Chris Roberts, 12 February 2020
*/

#include <flumps/json_value_path.h>

namespace flumps {

JsonValuePathError::JsonValuePathError(const std::string& msg) :
  std::runtime_error(msg) {
}

bool JsonValuePath::PathSegment::is_array() const {
  return key.empty();
}

bool JsonValuePath::PathSegment::is_object() const {
  return !key.empty();
}

bool JsonValuePath::PathSegment::operator==(const PathSegment& rhs) const {
  return key == rhs.key;
}

bool JsonValuePath::allowed_key_byte(char ch) {
  return
    ch > ' ' && ch <= '~'       // Printable ASCII excluding space
    && ch != '\\' && ch != '"'  // Exclude escape and string for future full
                                // json key name support
    && ch != '[' && ch != '.';  // Exclude structural path characters
}

JsonValuePath::JsonValuePath() {
}

JsonValuePath::JsonValuePath(const std::string& path) :
    path_(), path_repr_(path) {
  enum State {
    empty_root, dot_root, partial_array, complete_array,
    start_key, within_key
  } state = empty_root;
  auto key = std::string();
  for (auto pos = path.begin(); true; ++pos) {
    switch (state) {
      case empty_root:
        if (pos == path.end()) return;
        else if (*pos != '.') throw JsonValuePathError(
          "First char must be '.'");
        else
          state = dot_root;
        break;
      case dot_root:
        if (pos == path.end()) {
          return;
        } else if (*pos == '[') {
          state = partial_array;
        } else if (allowed_key_byte(*pos)) {
          state = within_key;
          key.push_back(*pos);
        } else {
          throw JsonValuePathError("'Start of array or key expected");
        }
        break;
      case partial_array:
        if (pos != path.end() && *pos == ']') {
          state = complete_array;
          path_.push_back(PathSegment{});
        } else {
          throw JsonValuePathError("End of array expected");
        }
        break;
      case complete_array:
        if (pos == path.end()) return;
        else if (*pos == '.') state = start_key;
        else if (*pos == '[') state = partial_array;
        else
          throw JsonValuePathError("Start of array or key expected");
        break;
      case start_key:
        if (pos != path.end() && allowed_key_byte(*pos)) {
          state = within_key;
          key.push_back(*pos);
        } else {
          throw JsonValuePathError("Key character expected");
        }
        break;
      case within_key:
        if (pos == path.end()) {
          path_.push_back(PathSegment{key});
          key.clear();
          return;
        } else if (*pos == '.') {
          state = start_key;
          path_.push_back(PathSegment{key});
          key.clear();
        } else if (*pos == '[') {
          state = partial_array;
          path_.push_back(PathSegment{key});
          key.clear();
        } else if (allowed_key_byte(*pos)) {
          key.push_back(*pos);
        } else {
          throw JsonValuePathError("Invalid key character");
        }
        break;
    }
  }
}

const std::vector<JsonValuePath::PathSegment> JsonValuePath::segments() const {
  return path_;
}

const std::string& JsonValuePath::str() const {
  return path_repr_;
}

bool JsonValuePath::operator==(const JsonValuePath& rhs) const {
  // Ignore path_repr as "" and "." are equivalent.
  return path_ == rhs.path_;
}

}  // end namespace flumps
