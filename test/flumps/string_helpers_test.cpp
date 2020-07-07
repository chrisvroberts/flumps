/*
   Chris Roberts, 12 February 2020
*/

#include <cstring>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <flumps/string_helpers.h>
#include <flumps/exception.h>

namespace flumps {

namespace {

}  // end namespace

TEST(StringHelpersTest, chomp_utf8_char_1byte_valid) {
  const auto start = "\x7F_______";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_EQ(0x7F, chomp_utf8_char(&pos, end));
  ASSERT_EQ(start+1u, pos);
}

TEST(StringHelpersTest, chomp_utf8_char_2byte_valid) {
  const auto start = "\xCF\x8F______";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_EQ(0x3CF, chomp_utf8_char(&pos, end));
  ASSERT_EQ(start+2u, pos);
}

TEST(StringHelpersTest, chomp_utf8_char_3byte_valid) {
  const auto start = "\xEF\x8F\x8F_____";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_EQ(0xF3CF, chomp_utf8_char(&pos, end));
  ASSERT_EQ(start+3u, pos);
}

TEST(StringHelpersTest, chomp_utf8_char_4byte_valid) {
  const auto start = "\xF4\x8F\xBF\xBF____";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_EQ(0x10FFFF, chomp_utf8_char(&pos, end));
  ASSERT_EQ(start+4u, pos);
}

TEST(StringHelpersTest, chomp_utf8_char_bad_first_byte) {
  const auto start = "\xF8_______";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), Utf8DecodeError);
}

TEST(StringHelpersTest, chomp_utf8_char_2byte_truncated) {
  const auto start = "\xC7";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), Utf8DecodeError);
}

TEST(StringHelpersTest, chomp_utf8_char_2byte_subsequent_byte_invalid) {
  const auto start = "\xC7\xFF";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), Utf8DecodeError);
}

TEST(StringHelpersTest, chomp_utf8_char_3byte_truncated) {
  const auto start = "\xE7\x8F";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), Utf8DecodeError);
}

TEST(StringHelpersTest, chomp_utf8_char_3byte_subsequent_byte_invalid) {
  const auto start = "\xF7\x8F\xFF";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), Utf8DecodeError);
}

TEST(StringHelpersTest, chomp_utf8_char_4byte_truncated) {
  const auto start = "\xF7\x8F\x8F";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), Utf8DecodeError);
}

TEST(StringHelpersTest, chomp_utf8_char_4byte_subsequent_byte_invalid) {
  const auto start = "\xF4\x8F\xB0\xFF";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), Utf8DecodeError);
}

TEST(StringHelpersTest, chomp_utf8_char_4byte_too_large) {
  const auto start = "\xF4\x90\x90\x90";  // max_code_point+1
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(chomp_utf8_char(&pos, end), UnicodeCodePointError);
}

TEST(StringHelpersTest, json_hex_seq_to_code_point_valid) {
  const auto start = "09aF____";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  const auto cp = json_hex_seq_to_code_point(&pos, end);
  ASSERT_EQ(start+4u, pos);
  ASSERT_EQ(0x09af, cp);
}

TEST(StringHelpersTest, json_hex_seq_to_code_point_insufficient_data) {
  const auto start = "000";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(json_hex_seq_to_code_point(&pos, end), JsonParseError);
}

TEST(StringHelpersTest, json_hex_seq_to_code_point_invalid_data) {
  const auto start = "000_";
  auto pos = start;
  const auto end = pos + std::strlen(start);
  ASSERT_THROW(json_hex_seq_to_code_point(&pos, end), JsonParseError);
}

namespace {
  const auto non_sur_cp = 0x00000;
  const auto sur_cp = 0x10000;
  const auto sur_high_cp = 0xD800;
  const auto sur_low_cp = 0xDC00;
}

TEST(StringHelpersTest, utf8_surrogate_checker_initialise) {
  auto checker = utf8_surrogate_checker();
  ASSERT_EQ(false, checker.surrogate_low_needed);
}

TEST(StringHelpersTest, utf8_surrogate_checker_non_surrogate) {
  auto checker = utf8_surrogate_checker();
  ASSERT_EQ(true, checker.check_code_point(non_sur_cp));
  ASSERT_EQ(false, checker.surrogate_low_needed);
  ASSERT_EQ(non_sur_cp, checker.last_code_point);
}

TEST(StringHelpersTest, utf8_surrogate_checker_valid_surrogate_pair) {
  auto checker = utf8_surrogate_checker();
  ASSERT_EQ(false, checker.check_code_point(sur_high_cp));
  ASSERT_EQ(true, checker.surrogate_low_needed);
  ASSERT_EQ(true, checker.check_code_point(sur_low_cp));
  ASSERT_EQ(false, checker.surrogate_low_needed);
  ASSERT_EQ(sur_cp, checker.last_code_point);
}

TEST(StringHelpersTest, utf8_surrogate_checker_invalid_sur_non_low) {
  auto checker = utf8_surrogate_checker();
  ASSERT_EQ(true, checker.check_code_point(non_sur_cp));
  ASSERT_EQ(false, checker.surrogate_low_needed);
  ASSERT_THROW(checker.check_code_point(sur_low_cp), UnicodeCodePointError);
}

TEST(StringHelpersTest, utf8_surrogate_checker_truncated_sur_non_high) {
  auto checker = utf8_surrogate_checker();
  ASSERT_EQ(true, checker.check_code_point(non_sur_cp));
  ASSERT_EQ(false, checker.surrogate_low_needed);
  ASSERT_EQ(false, checker.check_code_point(sur_high_cp));
  ASSERT_EQ(true, checker.surrogate_low_needed);
  ASSERT_THROW(checker.check_code_point(), UnicodeCodePointError);
}

TEST(StringHelpersTest, utf8_surrogate_checker_invalid_sur_low) {
  auto checker = utf8_surrogate_checker();
  ASSERT_THROW(checker.check_code_point(sur_low_cp), UnicodeCodePointError);
}

TEST(StringHelpersTest, utf8_surrogate_checker_invalid_sur_high_high) {
  auto checker = utf8_surrogate_checker();
  ASSERT_EQ(false, checker.check_code_point(sur_high_cp));
  ASSERT_EQ(true, checker.surrogate_low_needed);
  ASSERT_THROW(checker.check_code_point(sur_high_cp), UnicodeCodePointError);
}

TEST(StringHelpersTest, utf8_surrogate_checker_invalid_sur_high_non) {
  auto checker = utf8_surrogate_checker();
  ASSERT_EQ(false, checker.check_code_point(sur_high_cp));
  ASSERT_EQ(true, checker.surrogate_low_needed);
  ASSERT_THROW(checker.check_code_point(non_sur_cp), UnicodeCodePointError);
}

TEST(StringHelpersTest, to_json_string_content_safe_basic_tests) {
  const auto input = std::string("bad json = \"truncated");
  const auto output = to_json_string_content_safe(input);
  const auto expected_output = std::string("bad json = \\\"truncated");
  ASSERT_EQ(expected_output, output);
}

TEST(StringHelpersTest, json_string_content_to_utf8_u_escape_non_sur) {
  const auto input = std::string("\\u0041");
  const auto output = json_string_content_to_utf8(input);
  const auto expected_output = std::string("A");
  ASSERT_EQ(expected_output, output);
}

// TODO: Add tests for these methods.
// std::string json_string_content_to_utf8(const std::string& input);
// std::string utf8_to_json_string_content(const std::string& input);
// std::string to_json_string_content_safe(const std::string& input);

}  // end namespace flumps
