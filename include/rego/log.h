// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <trieste/source.h>

namespace rego
{
  std::ostream& operator<<(std::ostream& os, const std::set<Location>& locs);
  std::ostream& operator<<(std::ostream& os, const std::vector<Location>& locs);

  struct Logger
  {
    static bool enabled;
    static std::string indent;

    static inline void increase_print_indent()
    {
      indent += "  ";
    }

    static inline void decrease_print_indent()
    {
      indent = indent.substr(0, indent.size() - 2);
    }

    template <typename T>
    static inline void print(const T& value)
    {
      if (enabled)
      {
        std::cout << value << std::endl;
      }
    }

    template <typename T, typename... Types>
    static void print(T head, Types... tail)
    {
      if (enabled)
      {
        std::cout << head;
        print(tail...);
      }
    }

    template <typename T>
    static inline void print_vector_inline(const std::vector<T>& values)
    {
      if (enabled)
      {
        std::cout << indent << "[";
        std::string sep = "";
        for (auto& value : values)
        {
          std::cout << sep << value;
          sep = ", ";
        }
        std::cout << "]" << std::endl;
      }
    }

    template <typename T, typename P>
    static inline void print_vector_custom(
      const std::vector<T>& values, P (*transform)(const T&))
    {
      if (enabled)
      {
        for (auto& value : values)
        {
          print(indent, transform(value));
        }
      }
    }

    template <typename K, typename V>
    static inline void print_map_values(const std::map<K, V>& values)
    {
      if (enabled)
      {
        print(indent, "{");
        for (auto& [_, value] : values)
        {
          print(indent, "  ", value);
        }
        print(indent, "}");
      }
    }
  };

// clang-format off
#  define LOG(...) Logger::print(Logger::indent, __VA_ARGS__)
#  define LOG_HEADER(message, header) Logger::print(Logger::indent, (header), (message), (header))
#  define LOG_VECTOR(vector) Logger::print_vector_inline((vector))
#  define LOG_VECTOR_CUSTOM(vector, transform) Logger::print_vector_custom((vector), (transform))
#  define LOG_MAP_VALUES(map) Logger::print_map_values((map))
#  define LOG_INDENT() Logger::increase_print_indent()
#  define LOG_UNINDENT() Logger::decrease_print_indent()
  // clang-format on
}