/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_JSON_VALUE_TYPE_H_
#define FLUMPS_INCLUDE_FLUMPS_JSON_VALUE_TYPE_H_

#include <ostream>

namespace flumps {

enum class JsonValueType {
  Array      = 1 << 0,
  Object     = 1 << 1,
  True       = 1 << 2,
  False      = 1 << 3,
  Null       = 1 << 4,
  String     = 1 << 5,
  Number     = 1 << 6,
  Bool       = True | False,
  Primitive  = Bool | Null | String | Number,
  Collection = Array | Object,
  Any        = Collection | Primitive
};

std::ostream& operator<<(std::ostream& os, JsonValueType t);

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_JSON_VALUE_TYPE_H_
