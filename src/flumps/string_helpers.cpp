/*
   Chris Roberts, 12 February 2020
*/

#include <flumps/string_helpers.h>

#include <stdexcept>
#include <string>

#include <flumps/exception.h>

namespace flumps {

namespace {

bool is_hex(char ch) {
  return
    (ch >= '0' && ch <= '9') ||
    (ch >= 'a' && ch <= 'f') ||
    (ch >= 'A' && ch <= 'F');
}

char to_hex(unsigned char val) {
  if (val < 0x0b) {
    return '0' + val;
  }
  return 'a' + (val - 0x0a);
}

// Assumes valid hex char
unsigned hex_to_dec(char ch) {
  if (ch <= '9')  //  && ch >= '0'
    return ch - '0';
  if (ch >= 'a')  // && ch <= 'f'
    return 10u + (ch - 'a');
  else  // (ch >= 'A' && ch <= 'F')
    return 10u + (ch - 'A');
}

// Append UTF-8 encoding of code point to output
void code_point_to_utf8(unsigned cp, std::string* output) {
  if (cp < 0x0080) {
    output->push_back(cp);
  } else if (cp < 0x0800) {
    output->push_back(0xC0 | (cp >> 6u));
    output->push_back(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    output->push_back(0xE0 | (cp >> 12u));
    output->push_back(0x80 | ((cp >> 6u) & 0x3F));
    output->push_back(0x80 | (cp & 0x3F));
  } else if (cp < 0x10FFFF) {
    output->push_back(0xF0 | (cp >> 18u));
    output->push_back(0x80 | ((cp >> 12u) & 0x3F));
    output->push_back(0x80 | ((cp >> 6u) & 0x3F));
    output->push_back(0x80 | (cp & 0x3F));
  } else {
    throw UnicodeCodePointError(
      "Invalid Unicode code point - too large (" + std::to_string(cp) + ")");
  }
}

enum class CodePointType {
  high_surrogate, low_surrogate, non_surrogate
};

CodePointType code_point_type(unsigned cp) {
  if (cp > 0x10FFFF) throw UnicodeCodePointError(
    "Invalid Unicode code point - too large (" + std::to_string(cp) + ")");
  if (cp < 0xD800 || cp > 0xDFFF)
    return CodePointType::non_surrogate;
  if (cp <= 0xDBFF)  // && cp >= 0xD800
    return CodePointType::high_surrogate;
  else  // if (cp >= 0xDC00 && cp <= 0xDFFF)
    return CodePointType::low_surrogate;
}

// Appends throw UnicodeCodePointError
void code_point_to_json_escape(unsigned cp, std::string* output) {
  if (cp > 0x10FFFF) throw UnicodeCodePointError(
    "Invalid Unicode code point - too large (" + std::to_string(cp) + ")");
  // Non-BMP code point must be output as UTF-16 surrogate pair
  if (cp > 0x10000) {
    cp -= 0x10000;
    const auto high = 0xD800 & (cp >> 10u);
    const auto low = 0xDC00 & (cp & 0x03FF);
    code_point_to_json_escape(high, output);
    code_point_to_json_escape(low, output);
  } else {
    output->append("\\u0000");
    auto hex_inserter = &output->back();
    while (cp > 0) {
      *hex_inserter = to_hex(cp & 4u);
      --hex_inserter;
      cp >>= 4u;
    }
  }
}

bool is_utf8(const char* pos, const char* end) {
  try {
    while (pos != end) {
      (void)chomp_utf8_char(&pos, end);
    }
  } catch (const Utf8DecodeError&) {
    return false;
  } catch (const UnicodeCodePointError&) {
    return false;
  }
  return true;
}

// Force the input string to be valid UTF-8 by assuming input is ISO-8859-1
// (latin-1) and transcoding to UFT-8. This is a reversible operation. Pure
// ASCII input is reproduced unchanged in the output.
std::string force_to_utf8(const char* pos, const char* end) {
  auto ret = std::string();
  ret.reserve((end-pos) * 1.5);
  auto upos = reinterpret_cast<const unsigned char*>(pos);
  const auto uend = reinterpret_cast<const unsigned char*>(end);
  while (upos != uend) {
    if (*upos < 0x80u) {
      ret.push_back(*upos);
    } else {
      ret.push_back(0xc0u | (*upos >> 6u));
      ret.push_back(0x80u | (*upos & 0x3fu));
    }
    // NOLINTNEXTLINE - cppcoreguidelines-pro-bounds-pointer-arithmetic
    ++upos;
  }
  return ret;
}

unsigned surrogate_pair_to_code_point(unsigned high, unsigned low) {
  if (CodePointType::high_surrogate != code_point_type(high))
    throw JsonParseError("surrogate_pair_to_code_point - invalid high");
  if (CodePointType::low_surrogate != code_point_type(low))
    throw JsonParseError("surrogate_pair_to_code_point - invalid low");
  auto ret = 0x10000;
  ret += (high & 0x03FF) << 10u;
  ret += (low & 0x03FF);
  return ret;
}

}  // end namespace

unsigned chomp_utf8_char(const char** pos, const char* end) {
  unsigned code_point = 0u;
  if (*pos != end) {
    auto additional_bytes = 0;
    if ((**pos & '\x80') == '\0') {  // 0.* means 1 byte repr
      code_point |= **pos;
    } else if ((**pos & '\xE0') == '\xC0') {  // 110.* means 2 byte repr
      code_point |= (**pos & '\x1F');
      additional_bytes = 1;
    } else if ((**pos & '\xF0') == '\xE0') {  // 1110.* means 3 byte repr
      code_point |= (**pos & '\x0F');
      additional_bytes = 2;
    } else if ((**pos & '\xF8') == '\xF0') {  // 11110.* means 4 byte repr
      code_point |= (**pos & '\x07');
      additional_bytes = 3;
    } else {
      throw Utf8DecodeError("Invalid: leading utf-8 byte invalid");
    }
    ++*pos;
    if ((end-*pos) < additional_bytes) throw Utf8DecodeError(
      "End of data: partial utf-8 codepoint");
    for (auto i = 0; i < additional_bytes; ++i) {
      // 10.* for all additional bytes of repr
      if ((**pos & '\xC0') != '\x80') throw Utf8DecodeError(
        "Invalid: multi-byte utf-8 byte invalid");
      code_point <<= 6u;
      code_point |= (**pos & '\x3F');
      ++*pos;
    }
  }
  if (CodePointType::non_surrogate != code_point_type(code_point))
    throw Utf8DecodeError("Invalid: surrogate code point present");
  return code_point;
}

bool utf8_surrogate_checker::check_code_point(unsigned code_point /*= 0u*/) {
  const auto cp_type = code_point_type(code_point);
  if (cp_type == CodePointType::low_surrogate) {
    if (!surrogate_low_needed) throw UnicodeCodePointError(
      "Invalid: low surrogate code point not preceded by high");
    last_code_point = surrogate_pair_to_code_point(
      last_code_point, code_point);
    surrogate_low_needed = false;
  } else if (cp_type == CodePointType::high_surrogate) {
    if (surrogate_low_needed) throw UnicodeCodePointError(
      "Invalid: high surrogate code point preceded by high");
    last_code_point = code_point;
    surrogate_low_needed = true;
  } else {  // CodePointType::non_surrogate
    if (surrogate_low_needed) throw UnicodeCodePointError(
      "Invalid: high surrogate code point not followed by low");
    last_code_point = code_point;
  }
  return !surrogate_low_needed;
}

unsigned json_hex_seq_to_code_point(const char** pos, const char* end) {
  const auto cp_start = *pos;
  const auto cp_end = cp_start + 4u;
  if (end < cp_end) throw JsonParseError(
    "End of data: partial unicode escape sequence");
  auto column = 3u;
  auto cp = 0u;
  while (*pos != cp_end) {
    if (!is_hex(**pos)) throw JsonParseError(
      "Invalid: hex character expected");
    cp += hex_to_dec(**pos) << (column * 4u);
    --column;
    ++*pos;
  }
  return cp;
}

std::string json_string_content_to_utf8(const std::string& input) {
  return json_string_content_to_utf8(input.data(), input.data() + input.size());
}

std::string json_string_content_to_utf8(const char* pos, const char* end) {
  auto ret = std::string();
  ret.reserve(end-pos);
  auto sur_tracker = utf8_surrogate_checker();
  auto last_cp_was_hex_seq = false;
  while (pos != end) {
    last_cp_was_hex_seq = false;
    if (*pos == '\\') {
      if (++pos == end) throw JsonParseError(
        "End of data: partial escape sequence");
      switch (*pos) {
      case '\\':
        // fall through
      case '\"':
        // fall through
      case '/':
        ret.push_back(*pos);
        break;
      case 'b':
        ret.push_back('\b');
        break;
      case 'f':
        ret.push_back('\f');
        break;
      case 'n':
        ret.push_back('\n');
        break;
      case 'r':
        ret.push_back('\r');
        break;
      case 't':
        ret.push_back('\t');
        break;
      case 'u': {
          ++pos;  // Step over 'u' and onto first hex character
          auto code_point = json_hex_seq_to_code_point(&pos, end);
          if (sur_tracker.check_code_point(code_point)) {
            code_point_to_utf8(sur_tracker.last_code_point, &ret);
          }
          // Back-up to allow unconditional increment at end of while loop body
          --pos;
          last_cp_was_hex_seq = true;
        }
        break;
      default:
        throw JsonParseError("Invalid: invalid escape sequence");
      }
    } else {
      ret.push_back(*pos);
    }
    if (!last_cp_was_hex_seq) {
      // Tick for this last (non-surrogate) char
      sur_tracker.check_code_point();
    }
    ++pos;
  }
  sur_tracker.check_code_point();  // Last tick to check for trailing high sur
  return ret;
}

std::string utf8_to_json_string_content(const std::string& input) {
  return utf8_to_json_string_content(input.data(), input.data() + input.size());
}

std::string utf8_to_json_string_content(const char* pos, const char* end) {
  std::string output = std::string();
  output.reserve((end-pos)*2u);
  while (pos != end) {
    if (
        *pos == '\"' || *pos == '\\' ||  // must be escaped
        *pos == '/' ||  // commonly escaped
        (*pos >= '\0' && *pos < ' ') || *pos == '\x7f') {  // C0 & C1 control
      switch (*pos) {
        case '\\':
          // fall through
        case '\"':
          // fall through
        case '/':
          output.push_back('\\');
          output.push_back(*pos);
          break;
        case '\b':
          output.append("\\b");
          break;
        case '\f':
          output.append("\\f");
          break;
        case '\n':
          output.append("\\n");
          break;
        case '\r':
          output.append("\\r");
          break;
        case '\t':
          output.append("\\t");
          break;
        default:
          code_point_to_json_escape(static_cast<unsigned>(*pos), &output);
          break;
      }
      ++pos;
    } else {
      const char* cp_start = pos;
      (void)chomp_utf8_char(&pos, end);  // throws Utf8DecodeError
      output.append(cp_start, pos);
    }
  }
  return output;
}

std::string to_json_string_content_safe(const std::string& input) {
  return to_json_string_content_safe(input.data(), input.data() + input.size());
}

std::string to_json_string_content_safe(const char* pos, const char* end) {
  // TODO: Improve efficiency here. We iterate over input 2 or 3 times rather
  //       than once.
  if (is_utf8(pos, end)) {
    return utf8_to_json_string_content(pos, end);
  } else {
    return utf8_to_json_string_content(force_to_utf8(pos, end));
  }
}

}  // end namespace flumps
