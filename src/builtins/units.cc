#include "builtins.h"

namespace
{
  using namespace rego;

  const BigInt kb = static_cast<std::size_t>(1024);
  std::map<std::string, BigInt> bytes = {
    {"ki", kb},
    {"mi", kb* kb},
    {"gi", kb* kb* kb},
    {"ti", kb* kb* kb* kb},
    {"pi", kb* kb* kb* kb* kb},
    {"ei", kb* kb* kb* kb* kb* kb},
  };

  const BigInt k = static_cast<std::size_t>(1000);
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
    if (starts_with(s, "\\\"") && ends_with(s, "\\\""))
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
      else if (!contains(digits, *it))
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
      return Int ^ num_str;
    }

    if (num_decimals == 1)
    {
      return Float ^ num_str;
    }

    return err(x, errors.parse, EvalBuiltInError);
  }

  Node scale(const Node& num, const BigInt& scale, bool round)
  {
    if (num->type() == Int)
    {
      BigInt num_int(num->location());
      return Int ^ (num_int * scale).loc();
    }

    if (num->type() == Float)
    {
      double result = get_double(num) * scale.to_int();
      if (round)
      {
        result = std::round(result);
        Node result_node = Resolver::scalar(result);
        return Int ^ result_node->location();
      }

      return Resolver::scalar(result);
    }

    return err(num, "scale: expected number argument", EvalBuiltInError);
  }

  Node scale(const Node& num, double scale)
  {
    if (num->type() == Float || num->type() == Int)
    {
      return Resolver::scalar(get_double(num) * scale);
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
      if (ends_with(lower, unit))
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
        if (ends_with(lower, unit))
        {
          if (unit == "m" && ends_with(x_str, "M"))
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
      if (ends_with(lower, unit))
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
    Node x =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("units.parse"));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = strip_escaped_quotes(get_string(x));
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
    Node x =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("units.parse_bytes"));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = strip_escaped_quotes(get_string(x));
    if (ends_with(x_str, "b") || ends_with(x_str, "B"))
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