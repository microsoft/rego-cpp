#pragma once

#include "unifier.h"

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace rego
{
  std::ostream& operator<<(std::ostream& os, const std::set<Location>& locs);
  std::ostream& operator<<(std::ostream& os, const std::vector<Location>& locs);

//#define REGOCPP_LOGGING
#ifdef REGOCPP_LOGGING
  namespace logging
  {

    static std::string indent = "";

    inline void increase_print_indent()
    {
      indent += "  ";
    }

    inline void decrease_print_indent()
    {
      indent = indent.substr(0, indent.size() - 2);
    }

    template <typename T>
    inline void print(const T& value)
    {
      std::cout << value << std::endl;
    }

    template <typename T, typename... Types>
    void print(T head, Types... tail)
    {
      std::cout << head;

      print(tail...);
    }

    template <typename T>
    inline void print_vector_inline(const std::vector<T>& values)
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

    template <typename T>
    inline void print_vector_custom(
      const std::vector<T>& values,
      std::function<std::string(const T&)> to_string)
    {
      for (auto& value : values)
      {
        print(indent, to_string(value));
      }
    }

    template <typename K, typename V>
    inline void print_map_values(const std::map<K, V>& values)
    {
      print(indent, "{");
      for (auto& [_, value] : values)
      {
        print(indent, "  ", value);
      }
      print(indent, "}");
    }
  }

#  define LOG(...) logging::print(logging::indent, __VA_ARGS__)
#  define LOG_HEADER(message, header) \
    logging::print(logging::indent, (header), (message), (header))
#  define LOG_VECTOR(vector) logging::print_vector_inline((vector))
#  define LOG_VECTOR_CUSTOM(vector, to_string) \
    logging::print_vector_custom((vector), (to_string))
#  define LOG_MAP_VALUES(map) logging::print_map_values((map))
#  define LOG_INDENT() logging::increase_print_indent()
#  define LOG_UNINDENT() logging::decrease_print_indent()

#else

#  define LOG(...)
#  define LOG_HEADER(message, header)
#  define LOG_VECTOR(vector)
#  define LOG_VECTOR_CUSTOM(vector, to_string)
#  define LOG_MAP_VALUES(map)
#  define LOG_INDENT()
#  define LOG_UNINDENT()

#endif
}