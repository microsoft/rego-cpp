#pragma once

#include "internal.h"

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> aggregates();
    std::vector<BuiltIn> arrays();
    std::vector<BuiltIn> bits();
    std::vector<BuiltIn> casts();
    std::vector<BuiltIn> encoding();
    std::vector<BuiltIn> numbers();
    std::vector<BuiltIn> objects();
    std::vector<BuiltIn> regex();
    std::vector<BuiltIn> semver();
    std::vector<BuiltIn> sets();
    std::vector<BuiltIn> strings();
    std::vector<BuiltIn> time();
    std::vector<BuiltIn> types();
    std::vector<BuiltIn> units();
  }

  struct rune
  {
    std::uint32_t value;
    std::string_view source;
  };

  using runestring = std::basic_string<std::uint32_t>;
  using runestring_view = std::basic_string_view<std::uint32_t>;

  std::vector<rune> utf8_to_runes(const std::string_view& utf8);
  runestring utf8_to_runestring(const std::string_view& utf8);
  std::string runestring_to_utf8(const runestring_view& runes);
}