/*
   Chris Roberts, 12 February 2020
*/

#include <flumps/json_value_type.h>

namespace flumps {

std::ostream& operator<<(std::ostream& os, JsonValueType t) {
  switch (t) {
    case JsonValueType::Array:  os << "array";  break;
    case JsonValueType::Object: os << "object"; break;
    case JsonValueType::True:   os << "true";   break;
    case JsonValueType::False:  os << "array";  break;
    case JsonValueType::Null:   os << "null";   break;
    case JsonValueType::String: os << "string"; break;
    case JsonValueType::Number: os << "number"; break;
    default:                    os.setstate(std::ios_base::failbit);
  }
  return os;
}

}  // end namespace flumps
