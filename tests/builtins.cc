#include "test_case.h"

#include <thread>

#ifndef _WIN32
#include <cxxabi.h>
#endif

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  const std::size_t second_ns = 1000000000UL;
  const std::size_t minute_ns = 60UL * second_ns;
  const std::size_t hour_ns = 60UL * minute_ns;

  const std::map<std::string, double> duration_units = {
    {"ns", 1},
    {"us", 1000},
    {"µs", 1000},
    {"ms", 1000000},
    {"s", double(second_ns)},
    {"m", double(minute_ns)},
    {"h", double(hour_ns)}};

  std::chrono::nanoseconds parse_duration(const std::string& duration)
  {
    const char* duration_re =
      R"((-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)((?:ns|us|µs|ms|s|m|h)))";
    const RE2 re(duration_re);
    assert(re.ok());

    std::string number;
    std::string unit;
    std::int64_t ns = 0;
    std::size_t start = 0;
    while (start < duration.size())
    {
      re2::StringPiece input(duration.c_str() + start, duration.size() - start);
      if (RE2::PartialMatch(input, re, &number, &unit))
      {
        double number_d = std::stod(number);
        double unit_ns = duration_units.at(unit);
        ns += std::int64_t(number_d * unit_ns);
        start += number.size() + unit.size();
      }
      else
      {
        break;
      }
    }

    return std::chrono::nanoseconds(ns);
  }

  Node test_sleep(const Nodes& args)
  {
    Node duration =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("test.sleep"));
    if (duration == Error)
    {
      return duration;
    }

    std::chrono::nanoseconds ns = parse_duration(get_string(duration));
    std::this_thread::sleep_for(ns);

    return Term << (Scalar << (True ^ "true"));
  }

  Node test_sleep_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "duration")
                             << (bi::Description ^ "time in nanoseconds")
                             << (bi::Type << bi::Number)))
             << (bi::Result << (bi::Name ^ "void") << bi::Description
                            << (bi::Type << bi::Boolean));

  using namespace rego;
  namespace bi = rego::builtins;

  Node demangle_export_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "exportName")
                               << (bi::Description ^ "mangled symbol name")
                               << (bi::Type << bi::String))
                   << (bi::Arg << (bi::Name ^ "compartmentName")
                               << bi::Description << (bi::Type << bi::String)))
    << (bi::Result << (bi::Name ^ "demangled") << bi::Description
                   << (bi::Type << bi::String));

#ifndef _WIN32
  /**
   * Built-in function exposed to Rego for demangling the symbol names in
   * export entries.  Takes two arguments, the compartment name and the
   * mangled symbol name.
   */
  Node demangle_export(const Nodes& args)
  {
    Node exportName = unwrap_arg(args, UnwrapOpt(1).types({JSONString}));
    if (exportName->type() == Error)
    {
      return Undefined;
    }
    Node compartmentName = unwrap_arg(args, UnwrapOpt(0).types({JSONString}));
    if (compartmentName->type() == Error)
    {
      return Undefined;
    }
    auto string = get_string(exportName);
    auto compartmentNameString = get_string(compartmentName);
    const std::string_view LibraryExportPrefix = "__library_export_libcalls";
    const std::string_view ExportPrefix = "__export_";
    if (string.starts_with(LibraryExportPrefix))
    {
      string = string.substr(LibraryExportPrefix.size());
    }
    else
    {
      if (!string.starts_with(ExportPrefix))
      {
        return Undefined;
      }
      string = string.substr(ExportPrefix.size());
      if (!string.starts_with(compartmentNameString))
      {
        return Undefined;
      }
      string = string.substr(compartmentNameString.size());
    }
    if (!string.starts_with("_"))
    {
      return Undefined;
    }
    string = string.substr(1);
    // The way that rego-cpp exposes snmalloc can cause the realloc here to
    // crash.  Try to allocate a buffer that's large enough that we don't
    // care.
    size_t bufferSize = strlen(string.c_str()) * 8;
    char* buffer = static_cast<char*>(malloc(bufferSize));
    int error;
    buffer = abi::__cxa_demangle(string.c_str(), buffer, &bufferSize, &error);
    if (error != 0)
    {
      free(buffer);
      return Undefined;
    }
    std::string demangled(buffer);
    free(buffer);
    return scalar(std::move(demangled));
  }
#endif

  /**
   * Helper that decodes the hex strings emitted for static sealed objects.
   * These are written as sequences of bytes, with a space between each four
   * bytes.
   *
   * Takes a node that must have been unwrapped to a JSONString.
   */
  std::vector<uint8_t> decode_hex_node(const Node& node)
  {
    if (node->type() == Error)
    {
      return {};
    }
    auto hexString = get_string(node);
    std::vector<uint8_t> result;
    while (hexString.size() >= 8)
    {
      for (size_t i = 0; i < 8; i += 2)
      {
        uint8_t byte;
        auto [p, ec] = std::from_chars(
          hexString.data() + i, hexString.data() + i + 2, byte, 16);
        if (ec != std::errc{})
        {
          return {};
        }
        result.push_back(byte);
      }
      hexString = hexString.substr(8);
      if (hexString.size() > 0 && hexString[0] == ' ')
      {
        hexString = hexString.substr(1);
      }
    }
    return result;
  }

  Node decode_integer_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "hex")
                               << (bi::Description ^ "The hex string to decode")
                               << (bi::Type << bi::String))
                   << (bi::Arg << (bi::Name ^ "offset")
                               << (bi::Description ^
                                   "The offset in the string to start decoding")
                               << (bi::Type << bi::Number))
                   << (bi::Arg
                       << (bi::Name ^ "length")
                       << (bi::Description ^ "The number of bytes to decode")
                       << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^ "Decoded integer")
                   << (bi::Type << bi::Number));

  /**
   * Built-in function exposed to Rego for decoding a hex string into an
   * integer.
   *
   * Takes three arguments:
   * 1. The hex string to decode
   * 2. The offset in the string to start decoding
   * 3. The number of bytes to decode
   *
   * The third argument must be 1, 2, 3, or 4 bytes (3 does not make sense,
   * but it's easier to allow it than exclude it).  This corresponds to
   * uint8_t, uint16_t, and uint32_t in the source.
   */
  Node decode_integer(const Nodes& args)
  {
    auto bytes =
      decode_hex_node(unwrap_arg(args, UnwrapOpt(0).types({JSONString})));
    auto offsetNode = unwrap_arg(args, UnwrapOpt(1).types({Int}));
    auto lengthNode = unwrap_arg(args, UnwrapOpt(2).types({Int}));
    if ((offsetNode->type() == Error) || (lengthNode->type() == Error))
    {
      return Undefined;
    }

    auto maybeOffset = get_int(offsetNode).to_size();
    if (!maybeOffset.has_value())
    {
      return err(offsetNode, "Integer out of range");
    }
    size_t offset = maybeOffset.value();

    auto maybeLength = get_int(lengthNode).to_size();
    if (!maybeLength.has_value())
    {
      return err(lengthNode, "Integer out of range");
    }
    size_t length = maybeLength.value();
    uint32_t result = 0;
    size_t end = offset + length;
    if ((length > 4) || (end > bytes.size()))
    {
      return Undefined;
    }
    for (size_t i = 0; i < length; i++)
    {
      result |= (bytes[offset + i] << (i * 8));
    }
    return scalar(BigInt{int64_t(result)});
  }

  Node decode_c_string_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "hex")
                               << (bi::Description ^ "The hex string to decode")
                               << (bi::Type << bi::String))
                   << (bi::Arg << (bi::Name ^ "offset")
                               << (bi::Description ^
                                   "The offset in the string to start decoding")
                               << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^ "Decoded string")
                   << (bi::Type << bi::String));

  /**
   * Built-in function exposed to Rego for decoding a hex string containing a
   * C string into a Rego string.  This takes two arguments, the hex string
   * and the offset where the C string starts.
   */
  Node decode_c_string(const Nodes& args)
  {
    auto bytes =
      decode_hex_node(unwrap_arg(args, UnwrapOpt(0).types({JSONString})));
    auto offsetNode = unwrap_arg(args, UnwrapOpt(1).types({Int}));

    auto maybeOffset = get_int(offsetNode).to_size();
    if (!maybeOffset.has_value())
    {
      return err(offsetNode, "Integer out of range");
    }
    size_t offset = maybeOffset.value();

    std::string result;
    if (offset >= bytes.size())
    {
      return Undefined;
    }
    for (auto it = bytes.begin() + offset; (it != bytes.end()) && (*it != '\0');
         it++)
    {
      result.push_back(*it);
    }
    return scalar(std::move(result));
  }
}

namespace rego_test
{
  std::vector<BuiltIn> custom_builtins(const std::string& note)
  {
    std::vector<BuiltIn> builtins;
    builtins.push_back(
      BuiltInDef::create({"test.sleep"}, test_sleep_decl, test_sleep));
    if (note.starts_with("cheriot"))
    {
#ifdef _WIN32
      builtins.push_back(BuiltInDef::placeholder(
        {"export_entry_demangle"},
        demangle_export_decl,
        "Demangling not supported on this platform"));
#else
      builtins.push_back(BuiltInDef::create(
        {"export_entry_demangle"}, demangle_export_decl, demangle_export));
#endif
      builtins.push_back(BuiltInDef::create(
        {"integer_from_hex_string"}, decode_integer_decl, decode_integer));
      builtins.push_back(BuiltInDef::create(
        {"string_from_hex_string"}, decode_c_string_decl, decode_c_string));
    }
    return builtins;
  }
}