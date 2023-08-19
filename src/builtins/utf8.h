#pragma once

#include <cstdint>
#include <string>

namespace rego
{
  using runestring = std::basic_string<std::uint32_t>;
  using runestring_view = std::basic_string_view<std::uint32_t>;

  runestring utf8_to_runes(const std::string_view& utf8);
  std::string runes_to_utf8(const runestring_view& runes);
}