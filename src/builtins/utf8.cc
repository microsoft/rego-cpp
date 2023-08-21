#include "utf8.h"

#include <vector>

namespace
{
  const std::uint8_t MaskX = 0b11000000;
  const std::uint8_t Mask1 = 0b10000000;
  const std::uint8_t Mask2 = 0b11100000;
  const std::uint8_t Mask3 = 0b11110000;
  const std::uint8_t Mask4 = 0b11111000;

  const std::uint8_t MarkX = 0b10000000;
  const std::uint8_t Mark1 = 0b00000000;
  const std::uint8_t Mark2 = 0b11000000;
  const std::uint8_t Mark3 = 0b11100000;
  const std::uint8_t Mark4 = 0b11110000;

  const std::uint8_t ValueX = 0b00111111;
  const std::uint8_t Value1 = 0b01111111;
  const std::uint8_t Value2 = 0b00011111;
  const std::uint8_t Value3 = 0b00001111;
  const std::uint8_t Value4 = 0b00000111;

  const std::uint32_t Max1 = 0x007F;
  const std::uint32_t Max2 = 0x07FF;
  const std::uint32_t Max3 = 0xFFFF;
  const std::uint32_t Max4 = 0x10FFFF;

  const std::size_t ShiftX = 6;

  const std::uint32_t Bad = 0xFFFD;

  void to_utf8(std::uint32_t value, std::string& utf8)
  {
    if (value <= Max1)
    {
      switch (value)
      {
        case '\f':
          utf8.push_back('\\');
          utf8.push_back('f');
          break;
        case '\n':
          utf8.push_back('\\');
          utf8.push_back('n');
          break;
        case '\r':
          utf8.push_back('\\');
          utf8.push_back('r');
          break;
        case '\t':
          utf8.push_back('\\');
          utf8.push_back('t');
          break;
        case '\v':
          utf8.push_back('\\');
          utf8.push_back('v');
          break;
        case '\\':
          utf8.push_back('\\');
          utf8.push_back('\\');
          break;
        case '\'':
          utf8.push_back('\\');
          utf8.push_back('\'');
          break;
        case '\"':
          utf8.push_back('\\');
          utf8.push_back('\"');
          break;
        default:
          char c0 = static_cast<char>(Mark1 | value);
          utf8.push_back(c0);
          break;
      }
    }
    else if (value <= Max2)
    {
      char c1 = static_cast<char>((value & ValueX) | MarkX);
      value >>= ShiftX;
      char c0 = static_cast<char>(Mark2 | value);
      utf8.push_back(c0);
      utf8.push_back(c1);
    }
    else if (value <= Max3)
    {
      char c2 = static_cast<char>((value & ValueX) | MarkX);
      value >>= ShiftX;
      char c1 = static_cast<char>((value & ValueX) | MarkX);
      value >>= ShiftX;
      char c0 = static_cast<char>(Mark3 | value);
      utf8.push_back(c0);
      utf8.push_back(c1);
      utf8.push_back(c2);
    }
    else if (value <= Max4)
    {
      char c3 = static_cast<char>((value & ValueX) | MarkX);
      value >>= ShiftX;
      char c2 = static_cast<char>((value & ValueX) | MarkX);
      value >>= ShiftX;
      char c1 = static_cast<char>((value & ValueX) | MarkX);
      value >>= ShiftX;
      char c0 = static_cast<char>(Mark4 | value);
      utf8.push_back(c0);
      utf8.push_back(c1);
      utf8.push_back(c2);
      utf8.push_back(c3);
    }
    else
    {
      // bad
      to_utf8(Bad, utf8);
    }
  }

  rego::rune utf8_to_rune(
    std::string_view::const_iterator pos, std::string_view::const_iterator end)
  {
    std::uint8_t c0 = static_cast<std::uint8_t>(pos[0]);
    std::size_t remaining = std::distance(pos, end);
    if (c0 == '\\')
    {
      if (remaining >= 1)
      {
        switch (pos[1])
        {
          case 'f':
            return {'\f', std::string_view(pos, pos + 2)};
          case 'n':
            return {'\n', std::string_view(pos, pos + 2)};
          case 'r':
            return {'\r', std::string_view(pos, pos + 2)};
          case 't':
            return {'\t', std::string_view(pos, pos + 2)};
          case 'v':
            return {'\v', std::string_view(pos, pos + 2)};
          case '\\':
            return {'\\', std::string_view(pos, pos + 2)};
          case '\'':
            return {'\'', std::string_view(pos, pos + 2)};
          case '\"':
            return {'\"', std::string_view(pos, pos + 2)};
        }
      }
      if (remaining >= 3 && pos[1] == 'x')
      {
        std::string hex = std::string(pos + 2, pos + 4);
        std::uint32_t value = std::stoul(hex, nullptr, 16);
        return {value, std::string_view(pos, pos + 4)};
      }
      if (remaining >= 5 && pos[1] == 'u')
      {
        std::string hex = std::string(pos + 2, pos + 6);
        std::uint32_t value = std::stoul(hex, nullptr, 16);
        return {value, std::string_view(pos, pos + 6)};
      }
      if (remaining >= 9 && pos[1] == 'U')
      {
        std::string hex = std::string(pos + 2, pos + 10);
        std::uint32_t value = std::stoul(hex, nullptr, 16);
        return {value, std::string_view(pos, pos + 10)};
      }
    }

    if ((c0 & Mask1) == Mark1)
    {
      std::uint32_t value = c0 & Value1;
      return {value, std::string_view(pos, pos + 1)};
    }

    if ((c0 & Mask2) == Mark2 && remaining >= 2)
    {
      std::uint8_t c1 = static_cast<std::uint8_t>(pos[1]);
      if ((c1 & MaskX) == MarkX)
      {
        std::uint32_t value = c0 & Value2;
        value = (value << ShiftX) | (c1 & ValueX);
        return {value, std::string_view(pos, pos + 2)};
      }
    }
    else if ((c0 & Mask3) == Mark3 && remaining >= 3)
    {
      std::uint8_t c1 = static_cast<std::uint8_t>(pos[1]);
      std::uint8_t c2 = static_cast<std::uint8_t>(pos[2]);
      if ((c1 & MaskX) == MarkX && (c2 & MaskX) == MarkX)
      {
        std::uint32_t value = c0 & Value3;
        value = (value << ShiftX) | (c1 & ValueX);
        value = (value << ShiftX) | (c2 & ValueX);
        return {value, std::string_view(pos, pos + 3)};
      }
    }
    else if ((c0 & Mask4) == Mark4 && remaining >= 4)
    {
      std::uint8_t c1 = static_cast<std::uint8_t>(pos[1]);
      std::uint8_t c2 = static_cast<std::uint8_t>(pos[2]);
      std::uint8_t c3 = static_cast<std::uint8_t>(pos[3]);
      if (
        (c1 & MaskX) == MarkX && (c2 & MaskX) == MarkX && (c3 & MaskX) == MarkX)
      {
        std::uint32_t value = c0 & Value4;
        value = (value << ShiftX) | (c1 & ValueX);
        value = (value << ShiftX) | (c2 & ValueX);
        value = (value << ShiftX) | (c3 & ValueX);
        return {value, std::string_view(pos, pos + 4)};
      }
    }

    // bad
    return {Bad, std::string_view(pos, pos + 1)};
  }

}

namespace rego
{
  std::vector<rune> utf8_to_runes(const std::string_view& utf8)
  {
    std::vector<rune> runes;
    runes.reserve(utf8.size());
    auto pos = utf8.begin();
    auto end = utf8.end();
    while (pos < end)
    {
      rune r = utf8_to_rune(pos, end);
      runes.push_back(r);
      pos = r.source.end();
    }

    return runes;
  }

  runestring utf8_to_runestring(const std::string_view& utf8)
  {
    runestring runes;
    runes.reserve(utf8.size());
    auto pos = utf8.begin();
    auto end = utf8.end();
    while (pos < end)
    {
      rune r = utf8_to_rune(pos, end);
      runes.push_back(r.value);
      pos = r.source.end();
    }
    return runes;
  }

  std::string runestring_to_utf8(const runestring_view& runes)
  {
    std::string utf8;
    utf8.reserve(runes.size());
    for (std::uint32_t r : runes)
    {
      to_utf8(r, utf8);
    }
    return utf8;
  }
}