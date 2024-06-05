#include "../rego_test.h"

#include <charconv>
#include <cxxabi.h>

namespace
{
  using namespace rego;

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
      return scalar(false);
    }
    Node compartmentName = unwrap_arg(args, UnwrapOpt(0).types({JSONString}));
    if (compartmentName->type() == Error)
    {
      return scalar(false);
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
        return scalar(false);
      }
      string = string.substr(ExportPrefix.size());
      if (!string.starts_with(compartmentNameString))
      {
        return scalar(false);
      }
      string = string.substr(compartmentNameString.size());
    }
    if (!string.starts_with("_"))
    {
      return scalar(false);
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
      return scalar(false);
    }
    std::string demangled(buffer);
    free(buffer);
    return scalar(std::move(demangled));
  }

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
      return scalar(false);
    }
    size_t offset = get_int(offsetNode).to_int();
    size_t length = get_int(lengthNode).to_int();
    uint32_t result = 0;
    size_t end = offset + length;
    if ((length > 4) || (end > bytes.size()))
    {
      return scalar(false);
    }
    for (size_t i = 0; i < length; i++)
    {
      result |= (bytes[offset + i] << (i * 8));
    }
    return scalar(BigInt{int64_t(result)});
  }

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
    size_t offset = get_int(offsetNode).to_int();
    std::string result;
    if (offset >= bytes.size())
    {
      return scalar(false);
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
  std::vector<BuiltIn> cheriot_builtins()
  {
    return {
      BuiltInDef::create(Location("export_entry_demangle"), 2, demangle_export),
      BuiltInDef::create(
        Location("integer_from_hex_string"), 3, decode_integer),
      BuiltInDef::create(
        Location("string_from_hex_string"), 2, decode_c_string)};
  }
}