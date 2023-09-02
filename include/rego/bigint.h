#pragma once

#include <trieste/source.h>

namespace rego
{
  class BigInt
  {
  public:
    BigInt();
    BigInt(const Location& value);
    BigInt(std::int64_t value);
    BigInt(std::size_t value);

    const Location& loc() const;
    std::int64_t to_int() const;
    std::size_t to_size() const;
    bool is_negative() const;
    bool is_zero() const;
    BigInt increment() const;
    BigInt decrement() const;

    static bool is_int(const Location& loc);

    friend BigInt operator+(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator-(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator*(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator/(const BigInt& lhs, const BigInt& rhs);
    friend BigInt operator%(const BigInt& lhs, const BigInt& rhs);

    friend bool operator>(const BigInt& lhs, const BigInt& rhs);
    friend bool operator<(const BigInt& lhs, const BigInt& rhs);
    friend bool operator<=(const BigInt& lhs, const BigInt& rhs);
    friend bool operator>=(const BigInt& lhs, const BigInt& rhs);
    friend bool operator==(const BigInt& lhs, const BigInt& rhs);
    friend bool operator!=(const BigInt& lhs, const BigInt& rhs);

    friend std::ostream& operator<<(std::ostream& os, const BigInt& value);

    BigInt negate() const;

  private:
    struct DivideResult
    {
      std::string quotient;
      std::string remainder;
    };

    static bool less_than(
      const std::string_view& lhs, const std::string_view& rhs);
    static bool greater_than(
      const std::string_view& lhs, const std::string_view& rhs);
    static bool equal(const std::string_view& lhs, const std::string_view& rhs);
    static std::string add(
      const std::string_view& lhs, const std::string_view& rhs, bool negative);
    static std::string subtract(
      const std::string_view& lhs, const std::string_view& rhs, bool negative);
    static DivideResult divide(
      const std::string_view& lhs, const std::string_view& rhs);
    static std::string multiply(
      const std::string_view& lhs, const std::string_view& rhs);
    std::string_view digits() const;
    Location m_loc;
    static Location Zero;
    static Location One;
  };
}