#include "builtins.h"
#include "rego.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  const BigInt kb = static_cast<std::size_t>(1024);
  std::map<std::string, BigInt> bytes = {
    {"ki", kb},
    {"mi", kb* kb},
    {"gi", kb* kb* kb},
    {"ti", kb* kb* kb* kb},
    {"pi", kb* kb* kb* kb* kb},
    {"ei", kb* kb* kb* kb* kb* kb},
  };

  const BigInt kd = static_cast<std::size_t>(1000);
  std::map<std::string, BigInt> big_units = {
    {"k", kd},
    {"m", kd* kd},
    {"g", kd* kd* kd},
    {"t", kd* kd* kd* kd},
    {"p", kd* kd* kd* kd* kd},
    {"e", kd* kd* kd* kd* kd* kd},
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

    auto it = num_str.begin();
    auto end = num_str.end();
    if (*it == '-' || *it == '+')
    {
      ++it;
    }

    const std::set<char> digits = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int num_decimals = 0;
    int num_exponent = 0;

    for (; it != end; ++it)
    {
      if (*it == '.')
      {
        num_decimals++;
      }
      else if (*it == 'e' || *it == 'E')
      {
        num_exponent++;
        if (it + 1 == end)
        {
          return err(x, errors.no_amount, EvalBuiltInError);
        }

        if (it[1] == '+' || it[1] == '-')
        {
          ++it;
        }
      }
      else if (!digits.contains(*it))
      {
        return err(x, errors.no_amount, EvalBuiltInError);
      }
    }

    if (num_exponent > 1 || num_decimals > 1)
    {
      return err(x, errors.parse, EvalBuiltInError);
    }

    if (num_decimals == 1 || num_exponent == 1)
    {
      return Float ^ num_str;
    }

    return Int ^ num_str;
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

  Node parse_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the unit to parse")
                             << (bi::Type << bi::String)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^ "the parsed number")
                            << (bi::Type << bi::Number));

  Node parse_bytes(const Nodes& args)
  {
    Node x =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("units.parse_bytes"));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_str = strip_escaped_quotes(get_string(x));
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

  Node parse_bytes_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "the byte unit to parse")
                             << (bi::Type << bi::String)))
             << (bi::Result << (bi::Name ^ "y")
                            << (bi::Description ^ "the parsed number")
                            << (bi::Type << bi::Number));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> units()
    {
      return {
        BuiltInDef::create(Location("units.parse"), parse_decl, parse),
        BuiltInDef::create(
          Location("units.parse_bytes"), parse_bytes_decl, parse_bytes)};
    }
  }
}