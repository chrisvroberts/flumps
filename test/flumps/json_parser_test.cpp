/*
   Chris Roberts, 12 February 2020
*/

#include <experimental/filesystem>

#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <flumps/json_parser.h>
#include <flumps/test_config.h>

namespace flumps {

namespace {

void parse_json_string(const std::string& input) {
  auto parser = JsonParser<>();
  const bool complete_parse = parser.parse_json(
    input.data(), input.data() + input.size());
  if (!complete_parse) {
    throw JsonParseError("JSON input incomplete");
  }
}

void parse_json_file(const std::string& input_filename) {
  auto f = std::ifstream(input_filename);
  auto json_data = std::string(
    (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  parse_json_string(json_data);
}

namespace filesystem = std::experimental::filesystem;

}  // end namespace

TEST(JsonParserTest, valid_json_files) {
  for (const auto& file : filesystem::directory_iterator(
    TestConfig::input_directory() + "/accepted")) {
    parse_json_file(file.path());
  }
}

TEST(JsonParserTest, invalid_json_files) {
  for (const auto& file : filesystem::directory_iterator(
    TestConfig::input_directory() + "/rejected")) {
    ASSERT_THROW(parse_json_file(file.path()), JsonParseError);
  }
}

TEST(JsonParserTest, undefined_json_files) {
  for (const auto& file : filesystem::directory_iterator(
    TestConfig::input_directory() + "/undefined")) {
    try {
      parse_json_file(file.path());
    } catch (const JsonParseError&) {
      // Parser can accept or reject these but must not crash!
    }
  }
}

TEST(JsonParserTest, valid_json_strings) {
  const std::vector<std::string> json_elements = {
    "null",
    "false",
    "true",
    "\"hello\"",
    "{}",
    "{ }",
    "[]",
    "[ ]",
    "1",
    "0",
    "1.0",
    "-1.0",
    "-1",
    "-0",
    "-1e1",
    "-1e+1",
    "-1e-1",
    "-1.0e-1",
    "-1.0e1",
    "-1.0e+1",
    "   12   ",
    " [  1 , 2] ",
    " [  1 , 2, {}] ",
    " [  1 , 2, { \"123\": 123 }] ",
    " [  1 , 2, { \"123\": [] }] ",
    " [  1 , 2, { \"123\": {} }] ",
    " [  1 , 2, { \"123\": \"jkn\" }] ",
  };
  for (const auto& json_element : json_elements) {
    parse_json_string(json_element);
  }
}

TEST(JsonParserTest, invalid_json_strings) {
  const std::vector<std::string> json_elements = {
    "-1e",
    "1.",
    " [  1 , 2, { 2}] ",
    " [  1 , 2, { \"123\" }] ",
    "[1,]",
    "{\"a\":1,}",
    "{\"a\":1\"b\":2}",
    "{\"a\":1 \"b\":2}",
    "[1\"a\"]",
    "[1 2]",
  };
  for (const auto& json_element : json_elements) {
    ASSERT_THROW(parse_json_string(json_element), JsonParseError);
  }
}

}  // end namespace flumps
