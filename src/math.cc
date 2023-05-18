#include "math.h"

namespace rego
{
  std::int64_t get_int(const Node& node)
  {
    std::string text(node->location().view());
    return std::stoll(text);
  }

  double get_double(const Node& node)
  {
    std::string text(node->location().view());
    return std::stod(text);
  }

  bool get_bool(const Node& node)
  {
    return node->location().view() == "true";
  }

  Node negate(const Node& node)
  {
    if (node->type() == JSONInt)
    {
      int value = get_int(node);
      value *= -1;
      return JSONInt ^ std::to_string(value);
    }
    else if (node->type() == JSONFloat)
    {
      double value = get_double(node);
      value *= -1.0;
      return JSONFloat ^ std::to_string(value);
    }
    else
    {
      return err(node, "Invalid argument for negation");
    }
  }

  Node math(const Node& op, std::int64_t lhs, std::int64_t rhs)
  {
    std::int64_t value;
    if (op->type() == Add)
    {
      value = lhs + rhs;
    }
    else if (op->type() == Subtract)
    {
      value = lhs - rhs;
    }
    else if (op->type() == Multiply)
    {
      value = lhs * rhs;
    }
    else if (op->type() == Divide)
    {
      if (rhs == 0)
      {
        return err(op, "divide by zero");
      }

      value = lhs / rhs;
    }
    else
    {
      return err(op, "unsupported math operation");
    }

    return JSONInt ^ std::to_string(value);
  }

  Node math(const Node& op, double lhs, double rhs)
  {
    double value;
    if (op->type() == Add)
    {
      value = lhs + rhs;
    }
    else if (op->type() == Subtract)
    {
      value = lhs - rhs;
    }
    else if (op->type() == Multiply)
    {
      value = lhs * rhs;
    }
    else if (op->type() == Divide)
    {
      if (rhs == 0.0)
      {
        return err(op, "divide by zero");
      }

      value = lhs / rhs;
    }
    else
    {
      return err(op, "unsupported math operation");
    }

    return JSONFloat ^ std::to_string(value);
  }

  Node compare(const Node& op, std::int64_t lhs, std::int64_t rhs)
  {
    bool value;
    if (op->type() == Equals)
    {
      value = lhs == rhs;
    }
    else if (op->type() == NotEquals)
    {
      value = lhs != rhs;
    }
    else if (op->type() == LessThan)
    {
      value = lhs < rhs;
    }
    else if (op->type() == LessThanOrEquals)
    {
      value = lhs <= rhs;
    }
    else if (op->type() == GreaterThan)
    {
      value = lhs > rhs;
    }
    else if (op->type() == GreaterThanOrEquals)
    {
      value = lhs >= rhs;
    }
    else
    {
      return err(op, "unsupported comparison");
    }

    if (value)
    {
      return JSONTrue ^ "true";
    }
    else
    {
      return JSONFalse ^ "false";
    }
  }

  Node compare(const Node& op, double lhs, double rhs)
  {
    bool value;
    if (op->type() == Equals)
    {
      value = lhs == rhs;
    }
    else if (op->type() == NotEquals)
    {
      value = lhs != rhs;
    }
    else if (op->type() == LessThan)
    {
      value = lhs < rhs;
    }
    else if (op->type() == LessThanOrEquals)
    {
      value = lhs <= rhs;
    }
    else if (op->type() == GreaterThan)
    {
      value = lhs > rhs;
    }
    else if (op->type() == GreaterThanOrEquals)
    {
      value = lhs >= rhs;
    }
    else
    {
      return err(op, "unsupported comparison");
    }

    if (value)
    {
      return JSONTrue ^ "true";
    }
    else
    {
      return JSONFalse ^ "false";
    }
  }
}