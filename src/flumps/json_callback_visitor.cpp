/*
   Chris Roberts, 12 February 2020
*/

#include <flumps/detail/json_callback_visitor_impl.h>

namespace flumps {

namespace detail {

const JsonCallbackVisitorImpl::JsonCollectionTraits
JsonCallbackVisitorImpl::json_object_traits = {
  Node::Type::Object, OverFlowType::Object, JsonValueType::Object, "."
};

const JsonCallbackVisitorImpl::JsonCollectionTraits
JsonCallbackVisitorImpl::json_array_traits = {
  Node::Type::Array, OverFlowType::Array, JsonValueType::Array, "[]"
};

}  // end namespace detail

}  // end namespace flumps
