#include "internal.hh"

namespace rego
{
  Location BigInt::Zero = Location("0");
  Location BigInt::One = Location("1");

  BigInt::BigInt() : m_loc(BigInt::Zero) {}

  BigInt::BigInt(const Location& loc) : m_loc(loc)
  {
    assert(is_int(loc));
  }

  BigInt::BigInt(const std::int64_t value) : BigInt(std::to_string(value)) {}

  BigInt::BigInt(const std::size_t value) : BigInt(std::to_string(value)) {}

  const Location& BigInt::loc() const
  {
    return m_loc;
  }

  bool BigInt::is_negative() const
  {
    return m_loc.view()[0] == '-';
  }

  BigInt BigInt::negate() const
  {
    if (is_negative())
    {
      return BigInt(std::string(m_loc.view().substr(1)));
    }

    return BigInt("-" + std::string(m_loc.view()));
  }

  BigInt operator+(const BigInt& lhs, const BigInt& rhs)
  {
    bool negate = false;
    if (lhs.is_negative())
    {
      if (rhs.is_negative())
      {
        negate = true;
      }
      else
      {
        // adding a negative number
        return rhs - lhs.negate();
      }
    }
    else
    {
      if (rhs.is_negative())
      {
        // adding a negative number
        return lhs - rhs.negate();
      }
    }

    std::string sum = BigInt::add(lhs.digits(), rhs.digits(), negate);
    return BigInt(sum);
  }

  std::string BigInt::add(
    const std::string_view& lhs, const std::string_view& rhs, bool negative)
  {
    if (less_than(lhs, rhs))
    {
      return BigInt::add(rhs, lhs, negative);
    }

    if (rhs == "0")
    {
      return std::string(lhs);
    }

    std::string result;
    auto lhs_it = lhs.rbegin();
    auto rhs_it = rhs.rbegin();
    int carry = 0;
    for (; lhs_it != lhs.rend() && rhs_it != rhs.rend(); ++lhs_it, ++rhs_it)
    {
      auto lhs_digit = *lhs_it - '0';
      auto rhs_digit = *rhs_it - '0';
      auto sum = lhs_digit + rhs_digit + carry;
      if (sum > 9)
      {
        carry = 1;
        sum -= 10;
      }
      else
      {
        carry = 0;
      }
      result.push_back(static_cast<char>(sum) + '0');
    }

    // lhs will have the same or more digits
    for (; lhs_it != lhs.rend(); ++lhs_it)
    {
      auto lhs_digit = *lhs_it - '0';
      auto sum = lhs_digit + carry;
      if (sum > 9)
      {
        carry = 1;
        sum -= 10;
      }
      else
      {
        carry = 0;
      }
      result.push_back(static_cast<char>(sum) + '0');
    }

    if (carry > 0)
    {
      result.push_back('1');
    }

    if (negative)
    {
      result.push_back('-');
    }

    std::reverse(result.begin(), result.end());

    return result;
  }

  BigInt operator-(const BigInt& lhs, const BigInt& rhs)
  {
    if (rhs.is_zero())
    {
      return lhs;
    }

    bool negate = false;
    if (lhs.is_negative())
    {
      if (rhs.is_negative())
      {
        // subtracting a negative number
        if (BigInt::greater_than(lhs.digits(), rhs.digits()))
        {
          // we can do the subtraction as if both numbers were positive
          // and then negate it.
          negate = true;
        }
        else
        {
          // reverse the order of the subtraction
          return rhs.negate() - lhs.negate();
        }
      }
      else
      {
        // do this as addition
        return lhs + rhs.negate();
      }
    }
    else
    {
      if (rhs.is_negative())
      {
        // subtracting a negative number
        return lhs + rhs.negate();
      }
      else
      {
        if (BigInt::less_than(lhs.digits(), rhs.digits()))
        {
          // reverse the order of the subtraction
          return (rhs - lhs).negate();
        }
      }
    }

    std::string difference =
      BigInt::subtract(lhs.digits(), rhs.digits(), negate);
    return BigInt(difference);
  }

  std::string BigInt::subtract(
    const std::string_view& lhs, const std::string_view& rhs, bool negative)
  {
    assert(!less_than(lhs, rhs));
    std::string result;
    auto lhs_it = lhs.rbegin();
    auto rhs_it = rhs.rbegin();
    bool borrow = false;
    for (; lhs_it != lhs.rend() && rhs_it != rhs.rend(); ++lhs_it, ++rhs_it)
    {
      auto lhs_digit = *lhs_it - '0';
      auto rhs_digit = *rhs_it - '0';
      if (borrow)
      {
        lhs_digit -= 1;
        borrow = false;
      }

      if (lhs_digit < rhs_digit)
      {
        lhs_digit += 10;
        borrow = true;
      }

      auto diff = lhs_digit - rhs_digit;
      result.push_back(static_cast<char>(diff) + '0');
    }

    // lhs will have the same or more digits
    for (; lhs_it != lhs.rend(); ++lhs_it)
    {
      auto lhs_digit = *lhs_it - '0';
      if (borrow)
      {
        lhs_digit -= 1;
        borrow = false;
      }

      if (lhs_digit < 0)
      {
        lhs_digit += 10;
        borrow = true;
      }

      result.push_back(static_cast<char>(lhs_digit) + '0');
    }

    assert(!borrow);

    while (result.size() > 1 && result.back() == '0')
    {
      result.pop_back();
    }

    if (negative && result != "0")
    {
      result.push_back('-');
    }

    std::reverse(result.begin(), result.end());

    return result;
  }

  BigInt operator*(const BigInt& lhs, const BigInt& rhs)
  {
    std::string product = BigInt::multiply(lhs.digits(), rhs.digits());

    if (product == "0")
    {
      return BigInt();
    }

    bool negate = false;
    if (lhs.is_negative())
    {
      negate = true;
    }
    if (rhs.is_negative())
    {
      negate = !negate;
    }

    if (negate)
    {
      product.insert(product.begin(), '-');
    }

    return BigInt(product);
  }

  std::string BigInt::multiply(
    const std::string_view& lhs, const std::string_view& rhs)
  {
    if (lhs == "0" || rhs == "0")
    {
      return "0";
    }

    if (greater_than(lhs, rhs))
    {
      return BigInt::multiply(rhs, lhs);
    }

    std::string result = "0";
    std::string start = "";
    for (auto lhs_it = lhs.rbegin(); lhs_it != lhs.rend(); lhs_it++)
    {
      auto lhs_digit = *lhs_it - '0';
      std::string intermediate = start;
      int carry = 0;
      for (auto rhs_it = rhs.rbegin(); rhs_it != rhs.rend(); rhs_it++)
      {
        auto rhs_digit = *rhs_it - '0';
        auto product = lhs_digit * rhs_digit + carry;
        if (product > 9)
        {
          carry = product / 10;
          product = product % 10;
        }
        else
        {
          carry = 0;
        }
        intermediate.push_back(static_cast<char>(product) + '0');
      }

      if (carry > 0)
      {
        intermediate.push_back(static_cast<char>(carry) + '0');
      }

      std::reverse(intermediate.begin(), intermediate.end());
      start += "0";
      result = BigInt::add(result, intermediate, false);
    }

    return result;
  }

  BigInt operator/(const BigInt& lhs, const BigInt& rhs)
  {
    if (BigInt::less_than(lhs.digits(), rhs.digits()))
    {
      return BigInt();
    }

    if (rhs.is_zero())
    {
      throw std::invalid_argument("division by zero");
    }

    bool negate = false;
    if (lhs.is_negative())
    {
      negate = true;
    }
    if (rhs.is_negative())
    {
      negate = !negate;
    }

    auto quotient = BigInt::divide(lhs.digits(), rhs.digits()).quotient;
    if (negate)
    {
      quotient.insert(quotient.begin(), '-');
    }

    return BigInt(quotient);
  }

  BigInt operator%(const BigInt& lhs, const BigInt& rhs)
  {
    if (BigInt::less_than(lhs.digits(), rhs.digits()))
    {
      return lhs;
    }

    if (rhs.is_zero())
    {
      throw std::invalid_argument("modulo by zero");
    }

    auto remainder = BigInt::divide(lhs.digits(), rhs.digits()).remainder;
    if (lhs.is_negative() && remainder != "0")
    {
      remainder.insert(remainder.begin(), '-');
    }

    return BigInt(remainder);
  }

  BigInt::DivideResult BigInt::divide(
    const std::string_view& lhs, const std::string_view& rhs)
  {
    std::string remainder;

    bool leading = true;
    std::string quotient;
    for (auto it = lhs.begin(); it != lhs.end(); ++it)
    {
      int digit = 0;
      remainder.push_back(*it);
      while (!less_than(remainder, rhs))
      {
        remainder = BigInt::subtract(remainder, rhs, false);
        digit++;
      }

      if (digit == 0 && leading)
      {
        continue;
      }

      leading = false;
      quotient.push_back(static_cast<char>(digit) + '0');
    }

    return DivideResult{quotient, remainder};
  }

  bool BigInt::less_than(
    const std::string_view& lhs, const std::string_view& rhs)
  {
    if (lhs.size() < rhs.size())
    {
      return true;
    }

    if (lhs.size() > rhs.size())
    {
      return false;
    }

    return lhs < rhs;
  }

  bool BigInt::greater_than(
    const std::string_view& lhs, const std::string_view& rhs)
  {
    if (lhs.size() > rhs.size())
    {
      return true;
    }

    if (lhs.size() < rhs.size())
    {
      return false;
    }

    return lhs > rhs;
  }

  bool operator<(const BigInt& lhs, const BigInt& rhs)
  {
    if (lhs.is_negative())
    {
      if (rhs.is_negative())
      {
        return BigInt::greater_than(lhs.digits(), rhs.digits());
      }
      else
      {
        return true;
      }
    }
    else
    {
      if (rhs.is_negative())
      {
        return false;
      }
      else
      {
        return BigInt::less_than(lhs.digits(), rhs.digits());
      }
    }
  }

  bool operator>(const BigInt& lhs, const BigInt& rhs)
  {
    if (lhs.is_negative())
    {
      if (rhs.is_negative())
      {
        return BigInt::less_than(lhs.digits(), rhs.digits());
      }
      else
      {
        return false;
      }
    }
    else
    {
      if (rhs.is_negative())
      {
        return true;
      }
      else
      {
        return BigInt::greater_than(lhs.digits(), rhs.digits());
      }
    }
  }

  bool operator<=(const BigInt& lhs, const BigInt& rhs)
  {
    return !(lhs > rhs);
  }

  bool operator>=(const BigInt& lhs, const BigInt& rhs)
  {
    return !(lhs < rhs);
  }

  bool operator==(const BigInt& lhs, const BigInt& rhs)
  {
    return lhs.m_loc == rhs.m_loc;
  }

  bool operator!=(const BigInt& lhs, const BigInt& rhs)
  {
    return lhs.m_loc != rhs.m_loc;
  }

  std::string_view BigInt::digits() const
  {
    if (is_negative())
    {
      return m_loc.view().substr(1);
    }

    return m_loc.view();
  }

  bool BigInt::is_zero() const
  {
    return m_loc.view() == "0";
  }

  std::int64_t BigInt::to_int() const
  {
    return std::stoll(std::string(m_loc.view()));
  }

  std::size_t BigInt::to_size() const
  {
    return std::stoul(std::string(m_loc.view()));
  }

  std::ostream& operator<<(std::ostream& os, const BigInt& bigint)
  {
    os << bigint.m_loc.view();
    return os;
  }

  BigInt BigInt::increment() const
  {
    return *this + One;
  }

  BigInt BigInt::decrement() const
  {
    return *this - One;
  }

  bool BigInt::is_int(const Location& loc)
  {
    if (loc.len == 0)
    {
      return false;
    }

    std::set<char> digits = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    auto it = loc.view().begin();
    auto end = loc.view().end();
    if (*it == '-')
    {
      ++it;
    }

    for (; it != end; ++it)
    {
      if (!contains(digits, *it))
      {
        return false;
      }
    }

    return true;
  }
}