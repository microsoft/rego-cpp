#include "bigint.h"
#include "errors.h"
#include "register.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  const BigInt kb = 1024ul;
  std::map<std::string, BigInt> bytes = {
    {"ki", kb},
    {"mi", kb* kb},
    {"gi", kb* kb* kb},
    {"ti", kb* kb* kb* kb},
    {"pi", kb* kb* kb* kb* kb},
    {"ei", kb* kb* kb* kb* kb* kb},
  };

  const BigInt k = 1000ul;
  std::map<std::string, BigInt> big_units = {
    {"k", k},
    {"m", k* k},
    {"g", k* k* k},
    {"t", k* k* k* k},
    {"p", k* k* k* k* k},
    {"e", k* k* k* k* k* k},
  };

  std::map<std::string, double> small_units = {
    {"d", 0.1},
    {"c", 0.01},
    {"m", 0.001},
    {"u", 0.000001},
    {"n", 0.000000001},
  };

  std::string strip_escaped_quotes(const std::string& s)
  {
    if (s.starts_with("\\\"") && s.ends_with("\\\""))
    {
      return s.substr(2, s.size() - 4);
    }

    return s;
  }

  int is_number(const Location& loc)
  {
    std::set<char> digits = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    auto it = loc.view().begin();
    auto end = loc.view().end();
    if (*it == '-')
    {
      ++it;
    }

    int num_decimals = 0;

    for (; it != end; ++it)
    {
      if (*it == '.')
      {
        num_decimals++;
      }
      else if (!digits.contains(*it))
      {
        return -1;
      }
    }

    return num_decimals;
  }

  struct UnitsErrors
  {
    std::string no_amount;
    std::string parse;
    std::string spaces;
  };

  Node parse_number(
    const UnitsErrors& errors, const Node& x, const std::string& num_str)
  {
    if (num_str.size() == 0)
    {
      return err(x, errors.no_amount, EvalBuiltInError);
    }

    int num_decimals = is_number(num_str);
    if (num_decimals == -1)
    {
      return err(x, errors.no_amount, EvalBuiltInError);
    }

    if (num_decimals == 0)
    {
      return JSONInt ^ num_str;
    }

    if (num_decimals == 1)
    {
      return JSONFloat ^ num_str;
    }

    return err(x, errors.parse, EvalBuiltInError);
  }

  Node scale(const Node& num, const BigInt& scale, bool round)
  {
    if (num->type() == JSONInt)
    {
      BigInt num_int(num->location());
      return JSONInt ^ (num_int * scale).loc();
    }

    if (num->type() == JSONFloat)
    {
      double result = Resolver::get_double(num) * scale.to_int();
      if (round)
      {
        result = std::round(result);
        Node result_node = Resolver::scalar(result);
        return JSONInt ^ result_node->location();
      }

      return Resolver::scalar(result);
    }

    return err(num, "scale: expected number argument", EvalBuiltInError);
  }

  Node scale(const Node& num, double scale)
  {
    if (num->type() == JSONFloat || num->type() == JSONInt)
    {
      return Resolver::scalar(Resolver::get_double(num) * scale);
    }

    return err(num, "scale: expected number argument", EvalBuiltInError);
  }

  Node do_parse(
    const UnitsErrors& errors,
    const Node& x,
    const std::string& x_str,
    bool include_small,
    bool round)
  {
    if (x_str.size() == 0)
    {
      return err(x, errors.no_amount, EvalBuiltInError);
    }

    auto has_space = std::find_if(
      x_str.begin(), x_str.end(), [](char c) { return std::isspace(c); });
    if (has_space != x_str.end())
    {
      return err(x, errors.spaces, EvalBuiltInError);
    }

    std::string lower;
    std::transform(
      x_str.begin(), x_str.end(), std::back_inserter(lower), [](char c) {
        return static_cast<char>(std::tolower(c));
      });

    for (auto [unit, value] : bytes)
    {
      if (lower.ends_with(unit))
      {
        std::string num_str = lower.substr(0, lower.size() - unit.size());
        Node num = parse_number(errors, x, num_str);
        if (num->type() == Error)
        {
          return num;
        }

        return scale(num, value, round);
      }
    }

    if (include_small)
    {
      for (auto [unit, value] : small_units)
      {
        if (lower.ends_with(unit))
        {
          if (unit == "m" && x_str.ends_with("M"))
          {
            break;
          }
          std::string num_str = lower.substr(0, lower.size() - unit.size());
          Node num = parse_number(errors, x, num_str);
          if (num->type() == Error)
          {
            return num;
          }

          return scale(num, value);
        }
      }
    }

    for (auto [unit, value] : big_units)
    {
      if (lower.ends_with(unit))
      {
        std::string num_str = lower.substr(0, lower.size() - unit.size());
        Node num = parse_number(errors, x, num_str);
        if (num->type() == Error)
        {
          return num;
        }

        return scale(num, value, round);
      }
    }

    return parse_number(errors, x, x_str);
  }

  Node parse(const Nodes& args)
  {
    Node x = Resolver::unwrap(
      args[0], JSONString, "units.parse: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = strip_escaped_quotes(Resolver::get_string(x));
    return do_parse(
      {"units.parse: no amount provided",
       "units.parse: could not parse amount to a number",
       "units.parse: spaces not allowed in resource strings"},
      x,
      x_str,
      true,
      false);
  }

  Node parse_bytes(const Nodes& args)
  {
    Node x = Resolver::unwrap(
      args[0], JSONString, "units.parse_bytes: operand 1 ", EvalTypeError);
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = strip_escaped_quotes(Resolver::get_string(x));
    if (x_str.ends_with("b") || x_str.ends_with("B"))
    {
      x_str = x_str.substr(0, x_str.size() - 1);
    }
    return do_parse(
      {"units.parse_bytes: no byte amount provided",
       "units.parse_bytes: could not parse byte amount to a number",
       "units.parse_bytes: spaces not allowed in resource strings"},
      x,
      x_str,
      false,
      true);
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> units()
    {
      return {
        BuiltInDef::create(Location("units.parse"), 1, parse),
        BuiltInDef::create(Location("units.parse_bytes"), 1, parse_bytes)};
    }
  }
}