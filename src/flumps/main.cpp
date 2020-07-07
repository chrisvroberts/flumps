/*
   Chris Roberts, 12 February 2020
*/

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <flumps/exception.h>
#include <flumps/json_parser.h>
#include <flumps/json_callback_visitor.h>
#include <flumps/json_echo_visitor.h>
#include <flumps/json_value_path.h>
#include <flumps/version.h>

namespace {

const std::string ARG_check_long         = "--check";
const std::string ARG_check_short        = "-c";
const std::string ARG_pretty_print_long  = "--pprint";
const std::string ARG_pretty_print_short = "-p";
const std::string ARG_filter_long        = "--filter";
const std::string ARG_filter_short       = "-f";
const std::string ARG_help_long          = "--help";
const std::string ARG_help_short         = "-h";
const std::string ARG_abort_depth_long   = "--abort-depth";
const std::string ARG_version_long       = "--version";
const std::string ARG_version_short      = "-v";
const std::string ARG_indent_long        = "--indent";
const std::string ARG_end_of_args        = "--";

const int RET_success                =  0;
const int RET_parse_error            =  1;
const int RET_missing_depth_value    =  2;
const int RET_file_stream_error      =  3;
const int RET_multiple_actions       =  4;
const int RET_cannot_open_file       =  5;
const int RET_parse_incomplete       =  6;
const int RET_invalid_depth_value    =  7;
const int RET_missing_indent_string  =  8;
const int RET_missing_filter_string  =  9;
const int RET_json_path_error        = 10;
const int RET_unknown_arg_flag       = 11;

bool matches(const char* arg, std::vector<std::string> vals) {
  return vals.end() != std::find(vals.begin(), vals.end(), arg);
}

void print_version() {
  std::cout <<
    "flumps " << flumps::FLUMPS_VERSION_MAJOR << "." <<
                 flumps::FLUMPS_VERSION_MAJOR << "." <<
                 flumps::FLUMPS_VERSION_PATCH << "\n"
    "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n"
    "\n"
    "Written by Chris Roberts <chrisvroberts@gmail.com>.\n"
    "Maintained at <https://github.com/chrisvroberts/flumps>."
  << std::endl;
}

void print_help() {
  std::cout <<
    "Usage: flumps [ARGS] [FILE..]\n"
    "\n"
    "Process JSON via FILE(s) or standard input if none are provided. The\n"
    "processing action is specified and controlled by ARGS. It can be one of\n"
    "check (-c), pretty-print (-p), or filter (-f). The Default action is check.\n"
    "On success the return code is set to zero, otherwise it is non-zero. See\n"
    "'Return codes' below for return code details.\n"
    "\n"
    "  -c, --check             ACTION (default): Verify the input is valid JSON.\n"
    "  -p, --pprint            ACTION: Output the input JSON in pretty format.\n"
    "  -f <str>\n"
    "      --filter <str>      ACTION: Output a selected part of the JSON input.\n"
    "                          Data is selected via the filter string <str>.\n"
    "                          See 'Filter language' below for syntax details.\n"
    "                          A value will be output each time it is selected\n"
    "                          by a value path within the filter string.\n"
    "                          Selected values are output as elements of a JSON\n"
    "                          array.\n"
    "  --                      Any arguments after this are always treated as\n"
    "                          files.\n"
    "      --abort-depth <val> Limit how deep the parser will step into the JSON\n"
    "                          tree to <val>. If the parser reaches <val> depth\n"
    "                          parsing will abort. The default value of <val>\n"
    "                          is 0 which means no depth limit. <val> is a\n"
    "                          decimal number. The depth at the root is zero and\n"
    "                          increases as the parser steps into each object\n"
    "                          or array.\n"
    "      --indent <str>      Indent string to be used when action is pretty-\n"
    "                          print. Default value of <str> is two spaces.\n"
    "  -v, --version           Report version.\n"
    "  -h, --help              Print this help message.\n"
    "\n"
    "Examples:\n"
    "  cat file | flumps       Validate content of file via standard input.\n"
    "  flumps -p file          Pretty-print file to standard output.\n"
    "\n"
    "Filter language:\n"
    "  A filter comprises one or more JSON value paths delimited by pipe\n"
    "  characters. A value path is a string identifying the location of zero or\n"
    "  more JSON values within the input. A value path begins at the root of the\n"
    "  JSON input and is a concatenation of either array accesses ([]) or object\n"
    "  member accesses (.<member_name>).\n"
    "\n"
    "  Filter examples:\n"
    "    ''                  Root value.\n"
    "    '.'                 Root value.\n"
    "    '.abc'              Value with key 'abc' in root object.\n"
    "    '.[]'               Values of root array.\n"
    "    '.abc.def'          Value with key 'def' in a nested object.\n"
    "    '.abc[]'            Values of array with key 'abc' in root object.\n"
    "    '.[][]'             Value of any array element within root array.\n"
    "    '.abc|.def'         Values with keys 'abc' or 'def' in root object.\n"
    "\n"
    "Return codes:\n"
    "  Success     0  Command ran successfully. All input provided was valid\n"
    "                 JSON.\n"
    "  Failures    1  Input provided was not valid JSON.\n"
    "              2  Depth argument supplied with no corresponding value.\n"
    "              3  Error encountered while reading input file.\n"
    "              4  More than one action argument supplied.\n"
    "              5  Input file could not be opened.\n"
    "              6  Input provided was valid JSON but truncated.\n"
    "              7  Depth value supplied was invalid.\n"
    "              8  Indent argument supplied with no corresponding string.\n"
    "              9  Filter action specified with no corresponding string.\n"
    "             10  Invalid JSON path supplied in filter string.\n"
    "             11  An unknown argument flag was supplied. If this is a\n"
    "                 filename it can be supplied at the end of the argument\n"
    "                 list after the '--' flag.\n"
    "\n"
    "Known issues:\n"
    "  * If multiple input filenames are supplied only the first is processed.\n"
    "  * The JSON value path syntax used when filtering only allows object\n"
    "    member names that are printable ASCII and that do not include space,\n"
    "    back-slash, double-quote, open-square-bracket or full-stop characters.\n"
    "\n"
    "For more details, visit: <https://github.com/chrisvroberts/flumps>."
  << std::endl;
}

void print_arg_error() {
  std::cerr << "Try 'flumps --help' for more information." << std::endl;
}

struct FilterResultPrinter {
  bool first_seen = false;
  void print_value(
      flumps::JsonValueType /*type*/,
      const std::string& /*path*/,
      const char* data,
      std::size_t length) {
    if (first_seen) {
      std::cout.put(',');
    }
    std::cout.write(data, length);
    first_seen = true;
  }
};

}  // end namespace

int main(int argc, char* argv[]) {
  enum { UNSPECIFIED, CHECK, PRETTY_PRINT, FILTER } action = UNSPECIFIED;
  auto input_filenames = std::vector<std::string>();
  auto parse_depth = 0u;
  auto indent_str = std::string("  ");
  auto filter_str = std::string();
  auto end_of_args = false;

  for (auto i=1; i < argc; ++i) {
    if (end_of_args) {
      // Treat all remaining args as filenames
      input_filenames.emplace_back(argv[i]);
    } else if (matches(argv[i], { ARG_check_long, ARG_check_short })) {
      if (action != UNSPECIFIED) {
        std::cerr << "Only one action can be specified" << std::endl;
        print_arg_error();
        return RET_multiple_actions;
      }
      action = CHECK;
    } else if (matches(argv[i], { ARG_pretty_print_long, ARG_pretty_print_short })) {
      if (action != UNSPECIFIED) {
        std::cerr << "Only one action can be specified" << std::endl;
        print_arg_error();
        return RET_multiple_actions;
      }
      action = PRETTY_PRINT;
    } else if (matches(argv[i], { ARG_filter_long, ARG_filter_short })) {
      if (action != UNSPECIFIED) {
        std::cerr << "Only one action can be specified" << std::endl;
        print_arg_error();
        return RET_multiple_actions;
      }
      action = FILTER;
      ++i;
      if (i >= argc) {
        std::cerr << "No filter string supplied" << std::endl;
        print_arg_error();
        return RET_missing_filter_string;
      }
      filter_str.assign(argv[i]);
    } else if (matches(argv[i], { ARG_abort_depth_long })) {
      ++i;
      if (i >= argc) {
        std::cerr << "No depth value supplied" << std::endl;
        print_arg_error();
        return RET_missing_depth_value;
      }
      const char* end = argv[i] + std::strlen(argv[i]);
      char* next;
      parse_depth = std::strtoul(argv[i], &next, 10);
      // Input string empty or not all input processed
      if (end == argv[i] || next != end) {
        std::cerr << "Invalid depth value supplied" << std::endl;
        print_arg_error();
        return RET_invalid_depth_value;
      }
    } else if (matches(argv[i], { ARG_indent_long })) {
      ++i;
      if (i >= argc) {
        std::cerr << "No indent string supplied" << std::endl;
        print_arg_error();
        return RET_missing_indent_string;
      }
      indent_str.assign(argv[i]);
    } else if (matches(argv[i], { ARG_help_long, ARG_help_short })) {
      print_help();
      return RET_success;
    } else if (matches(argv[i], { ARG_version_long, ARG_version_short })) {
      print_version();
      return RET_success;
    } else if (matches(argv[i], { ARG_end_of_args })) {
      end_of_args = true;
    } else {
      if (strlen(argv[i]) > 0u && *argv[i] == '-') {
        std::cerr << "Unknown argument flag '" << argv[i] << "' supplied" << std::endl;
        print_arg_error();
        return RET_unknown_arg_flag;
      }
      // Assume filename
      input_filenames.emplace_back(argv[i]);
    }
  }

  if (action == UNSPECIFIED) {
    action = CHECK;
  }
  auto input = std::string();
  if (!input_filenames.empty()) {
    auto file_stream = std::ifstream(input_filenames[0]);  // TODO handle multiple files and print filename or error
    if (file_stream.fail()) {
      return RET_cannot_open_file;
    }
    file_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
      file_stream.seekg(0, std::ios::end);
      input.reserve(file_stream.tellg());
      file_stream.seekg(0, std::ios::beg);

      input.assign(
        (std::istreambuf_iterator<char>(file_stream)),
        std::istreambuf_iterator<char>());
    } catch (const std::ifstream::failure& e) {
      std::cerr << "File stream failures: " << e.what() << std::endl;
      return RET_file_stream_error;
    }
  } else {
    input.assign(
      (std::istreambuf_iterator<char>(std::cin)),
      std::istreambuf_iterator<char>());
  }

  try {
    if (action == CHECK) {
      auto parser = flumps::JsonParser<flumps::NullJsonVisitor>(
        flumps::NullJsonVisitor(),
        parse_depth);
      const bool complete_parse = parser.parse_json(
        input.data(), input.data() + input.size());
      if (!complete_parse) {
        std::cerr << "Parse incomplete" << std::endl;
        return RET_parse_incomplete;
      }
    } else if (action == PRETTY_PRINT) {
      using EchoVisitor = flumps::JsonEchoVisitor<std::ostream_iterator<char>>;
      auto parser = flumps::JsonParser<EchoVisitor>(
        EchoVisitor(std::ostream_iterator<char>(std::cout, ""), indent_str),
        parse_depth);
      const bool complete_parse = parser.parse_json(
        input.data(), input.data() + input.size());
      if (!complete_parse) {
        std::cerr << "Parse incomplete" << std::endl;
        return RET_parse_incomplete;
      }
      std::cout << std::endl;
    } else {  // action == FILTER
      using namespace std::placeholders;  // NOLINT(build/namespaces)
      auto frp = FilterResultPrinter();
      auto parser = flumps::JsonParser<flumps::JsonCallbackVisitor>(
        flumps::JsonCallbackVisitor(), parse_depth);
      // TODO: Move this '|' splitting logic to be part the JsonValuePath itself?
      auto pos = filter_str.data();
      const auto end = pos + filter_str.size();
      auto path_start = pos;
      while (true) {
        if (pos == end || *pos == '|') {
          const auto path = std::string(path_start, pos);
          try {
            parser.get_visitor().register_callback(
              flumps::JsonValuePath(path),
              std::bind(&FilterResultPrinter::print_value, &frp, _1, _2, _3, _4));
          } catch (const flumps::JsonValuePathError& e) {
            std::cerr << "Invalid JSON path: " << e.what() << std::endl;
            return RET_json_path_error;
          }
          if (pos == end) {
            break;
          }
          path_start = pos+1;
        }
        ++pos;
      }
      std::cout.put('[');
      const bool complete_parse = parser.parse_json(
        input.data(), input.data() + input.size());
      if (!complete_parse) {
        std::cerr << "Parse incomplete" << std::endl;
        return RET_parse_incomplete;
      }
      std::cout << ']' << std::endl;
    }
  } catch(const flumps::JsonParseError& e) {
    std::cerr << "JSON parse error: " << e.what() << std::endl;
    return RET_parse_error;
  }

  return RET_success;
}
