/*
   Chris Roberts, 12 February 2020
*/

#include <iostream>
#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <flumps/json_callback_visitor.h>
#include <flumps/json_parser.h>

namespace flumps {

namespace {

struct MemoCallbackHandler {
  struct CallbackMemo {
    int callback_id;
    JsonValueType type;
    std::string path;
    std::string data;
    bool operator==(const CallbackMemo& rhs) const {
      return std::tie(callback_id, type, path, data) ==
        std::tie(rhs.callback_id, rhs.type, rhs.path, rhs.data);
    }
  };

  std::vector<CallbackMemo> callbacks;

  void on_event(
      int callback_id,
      JsonValueType type,
      const std::string& path,
      const char* data,
      std::size_t len) {
    callbacks.emplace_back(
      CallbackMemo{callback_id, type, path, {data, len}});
  }
};

}  // end namespace

TEST(JsonCallbackVisitorTest, callback_sequences) {
  MemoCallbackHandler handler;
  auto parser = JsonParser<JsonCallbackVisitor>();
  auto& visitor = parser.get_visitor();
  using namespace std::placeholders;  // NOLINT(build/namespaces)
  using JVT = JsonValueType;

  visitor.register_callback(
    JsonValuePath("."),
    std::bind(&MemoCallbackHandler::on_event, &handler, 1, _1, _2, _3, _4));
  visitor.register_callback(
    JsonValuePath(".abc"),
    std::bind(&MemoCallbackHandler::on_event, &handler, 2, _1, _2, _3, _4));
  visitor.register_callback(
    JsonValuePath(".[]"),
    std::bind(&MemoCallbackHandler::on_event, &handler, 3, _1, _2, _3, _4));
  visitor.register_callback(
    JsonValuePath(".abc.efg"),
    std::bind(&MemoCallbackHandler::on_event, &handler, 4, _1, _2, _3, _4));
  visitor.register_callback(
    JsonValuePath(".abc[]"),
    std::bind(&MemoCallbackHandler::on_event, &handler, 5, _1, _2, _3, _4));
  visitor.register_callback(
    JsonValuePath(".[].abc"),
    std::bind(&MemoCallbackHandler::on_event, &handler, 6, _1, _2, _3, _4));
  visitor.register_callback(
    JsonValuePath(".[][]"),
    std::bind(&MemoCallbackHandler::on_event, &handler, 7, _1, _2, _3, _4));

  auto json_data = std::string();
  auto complete_parse = bool();
  auto expected_callbacks = std::vector<MemoCallbackHandler::CallbackMemo>();

  json_data = std::string(
    "{\"abc\":1,\"abc\":{\"efg\":2,\"efg\":{},\"efg\":[]},\"abc\":[],"
      "\"abc\":[3,{}]}");

  complete_parse = parser.parse_json(
    json_data.data(), json_data.data() + json_data.size());

  expected_callbacks = std::vector<MemoCallbackHandler::CallbackMemo>{
    { 2, JVT::Number, ".abc",     "1" },
    { 4, JVT::Number, ".abc.efg", "2" },
    { 4, JVT::Object, ".abc.efg", "{}" },
    { 4, JVT::Array,  ".abc.efg", "[]" },
    { 2, JVT::Object, ".abc",     "{\"efg\":2,\"efg\":{},\"efg\":[]}" },
    { 2, JVT::Array,  ".abc",     "[]" },
    { 5, JVT::Number, ".abc[]",   "3" },
    { 5, JVT::Object, ".abc[]",   "{}" },
    { 2, JVT::Array,  ".abc",     "[3,{}]" },
    { 1, JVT::Object, ".",        "{\"abc\":1,\"abc\":{\"efg\":2,\"efg\":{},"
                                    "\"efg\":[]},\"abc\":[],\"abc\":[3,{}]}" },
  };

  ASSERT_TRUE(complete_parse);
  ASSERT_EQ(expected_callbacks, handler.callbacks);
  handler.callbacks.clear();

  json_data = std::string(
    "[5,[8,[],{}],{\"abc\":6,\"abc\":[],\"abc\":{}}]");

  complete_parse = parser.parse_json(
    json_data.data(), json_data.data() + json_data.size());

  expected_callbacks = std::vector<MemoCallbackHandler::CallbackMemo>{
    { 3, JVT::Number, ".[]",     "5" },
    { 7, JVT::Number, ".[][]",   "8" },
    { 7, JVT::Array,  ".[][]",   "[]" },
    { 7, JVT::Object, ".[][]",   "{}" },
    { 3, JVT::Array,  ".[]",     "[8,[],{}]" },
    { 6, JVT::Number, ".[].abc", "6" },
    { 6, JVT::Array,  ".[].abc", "[]" },
    { 6, JVT::Object, ".[].abc", "{}" },
    { 3, JVT::Object, ".[]",     "{\"abc\":6,\"abc\":[],\"abc\":{}}" },
    { 1, JVT::Array,  ".",       "[5,[8,[],{}],{\"abc\":6,\"abc\":[],"
                                  "\"abc\":{}}]" },
  };

  ASSERT_TRUE(complete_parse);
  ASSERT_EQ(expected_callbacks, handler.callbacks);
  handler.callbacks.clear();

  json_data = std::string(
    "\"abc\"");

  complete_parse = parser.parse_json(
    json_data.data(), json_data.data() + json_data.size());

  expected_callbacks = std::vector<MemoCallbackHandler::CallbackMemo>{
    { 1, JVT::String, ".", "abc" },
  };

  ASSERT_TRUE(complete_parse);
  ASSERT_EQ(expected_callbacks, handler.callbacks);
  handler.callbacks.clear();

  json_data = std::string(
    "{\"def\":[{\"xyz\":[]}],\"def\":{\"xyz\":[3]}}");

  complete_parse = parser.parse_json(
    json_data.data(), json_data.data() + json_data.size());

  expected_callbacks = std::vector<MemoCallbackHandler::CallbackMemo>{
    { 1, JVT::Object, ".", "{\"def\":[{\"xyz\":[]}],\"def\":{\"xyz\":[3]}}" },
  };

  ASSERT_TRUE(complete_parse);
  ASSERT_EQ(expected_callbacks, handler.callbacks);
  handler.callbacks.clear();

  json_data = std::string(
    "[[{\"a\":[]},{}],{\"x\":[],\"y\":{}}]");

  complete_parse = parser.parse_json(
    json_data.data(), json_data.data() + json_data.size());

  expected_callbacks = std::vector<MemoCallbackHandler::CallbackMemo>{
    { 7, JVT::Object, ".[][]", "{\"a\":[]}" },
    { 7, JVT::Object, ".[][]", "{}" },
    { 3, JVT::Array,  ".[]",   "[{\"a\":[]},{}]" },
    { 3, JVT::Object, ".[]",   "{\"x\":[],\"y\":{}}" },
    { 1, JVT::Array,  ".",    "[[{\"a\":[]},{}],{\"x\":[],\"y\":{}}]" },
  };

  ASSERT_TRUE(complete_parse);
  ASSERT_EQ(expected_callbacks, handler.callbacks);
  handler.callbacks.clear();

  json_data = std::string(
    "[true,false,null]");

  complete_parse = parser.parse_json(
    json_data.data(), json_data.data() + json_data.size());

  expected_callbacks = std::vector<MemoCallbackHandler::CallbackMemo>{
    { 3, JVT::True,  ".[]", "true" },
    { 3, JVT::False, ".[]", "false" },
    { 3, JVT::Null,  ".[]", "null" },
    { 1, JVT::Array, ".",   "[true,false,null]" },
  };

  ASSERT_TRUE(complete_parse);
  ASSERT_EQ(expected_callbacks, handler.callbacks);
  handler.callbacks.clear();
}

}  // end namespace flumps
