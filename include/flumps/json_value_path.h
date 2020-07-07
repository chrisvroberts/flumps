/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_JSON_VALUE_PATH_H_
#define FLUMPS_INCLUDE_FLUMPS_JSON_VALUE_PATH_H_

#include <stdexcept>
#include <string>
#include <vector>

namespace flumps {

struct JsonValuePathError : public std::runtime_error {
  explicit JsonValuePathError(const std::string& msg);
};

class JsonValuePath {
 public:
  struct PathSegment {
    bool is_array() const;
    bool is_object() const;
    std::string key;
    bool operator==(const PathSegment& rhs) const;
  };

  JsonValuePath();

  // ''                  Root value
  // '.'                 Root value
  // '.abc'              Value of key 'abc' in root object
  // '.[]'               Values of root array
  // '.abc.def'          Value of key 'def' in nested object
  // '.abc[]'            Values of array at key 'abc' in root object
  // '.[][]'             Value of any array within root array
  //
  // Future features:
  // * array indices e.g. '[4]'
  // * full json quoted key names e.g. '."a z"' or '."a[]b"' or '."a\"b"'
  //   or '."a\rb"'
  explicit JsonValuePath(const std::string& path);
  const std::vector<PathSegment> segments() const;
  const std::string& str() const;
  bool operator==(const JsonValuePath& rhs) const;

 private:
  std::vector<PathSegment> path_;
  std::string path_repr_;
  bool allowed_key_byte(char ch);
};

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_JSON_VALUE_PATH_H_
