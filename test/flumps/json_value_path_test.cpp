/*
   Chris Roberts, 12 February 2020
*/

#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <flumps/json_value_path.h>

namespace flumps {

TEST(JsonValuePathTest, default_constructed) {
  auto jvp = JsonValuePath();
  ASSERT_TRUE(jvp.segments().empty());
  ASSERT_EQ(std::string(""), jvp.str());
}

TEST(JsonValuePathTest, empty_path) {
  auto jvp = JsonValuePath("");
  ASSERT_EQ(JsonValuePath(), jvp);
}

TEST(JsonValuePathTest, dot_alias_for_empty) {
  auto jvp = JsonValuePath(".");
  ASSERT_EQ(JsonValuePath(), jvp);
}

TEST(JsonValuePathTest, key) {
  auto jvp = JsonValuePath(".abc");
  ASSERT_EQ(1, jvp.segments().size());
  ASSERT_TRUE(jvp.segments()[0].is_object());
  ASSERT_EQ(std::string("abc"), jvp.segments()[0].key);
}

TEST(JsonValuePathTest, array) {
  auto jvp = JsonValuePath(".[]");
  ASSERT_EQ(1, jvp.segments().size());
  ASSERT_TRUE(jvp.segments()[0].is_array());
}

TEST(JsonValuePathTest, key_key) {
  auto jvp = JsonValuePath(".abc.efg");
  ASSERT_EQ(2, jvp.segments().size());
  ASSERT_TRUE(jvp.segments()[0].is_object());
  ASSERT_EQ(std::string("abc"), jvp.segments()[0].key);
  ASSERT_TRUE(jvp.segments()[1].is_object());
  ASSERT_EQ(std::string("efg"), jvp.segments()[1].key);
}

TEST(JsonValuePathTest, key_array) {
  auto jvp = JsonValuePath(".abc[]");
  ASSERT_EQ(2, jvp.segments().size());
  ASSERT_TRUE(jvp.segments()[0].is_object());
  ASSERT_EQ(std::string("abc"), jvp.segments()[0].key);
  ASSERT_TRUE(jvp.segments()[1].is_array());
}

TEST(JsonValuePathTest, array_array) {
  auto jvp = JsonValuePath(".[][]");
  ASSERT_EQ(2, jvp.segments().size());
  ASSERT_TRUE(jvp.segments()[0].is_array());
  ASSERT_TRUE(jvp.segments()[1].is_array());
}

TEST(JsonValuePathTest, error_cases) {
  ASSERT_THROW(JsonValuePath("a"), JsonValuePathError);
  ASSERT_THROW(JsonValuePath(". "), JsonValuePathError);
  ASSERT_THROW(JsonValuePath(".[a"), JsonValuePathError);
  ASSERT_THROW(JsonValuePath(".[]a"), JsonValuePathError);
  ASSERT_THROW(JsonValuePath(".a "), JsonValuePathError);
  ASSERT_THROW(JsonValuePath(".a. "), JsonValuePathError);
}

}  // end namespace flumps
