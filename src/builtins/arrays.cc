#include "register.h"
#include "resolver.h"

namespace
{
  using namespace rego;

  Node concat(const Nodes& args)
  {
    auto maybe_x = Resolver::maybe_unwrap_array(args[0]);
    if(!maybe_x.has_value()){
      std::string x_type = std::string(args[0]->front()->type().str());
      return err(args[1], "array.concat: operand 1 must be array but got " + x_type, "eval_type_error");
    }
    Node x = *maybe_x;

    auto maybe_y = Resolver::maybe_unwrap_array(args[1]);
    if(!maybe_y.has_value()){
      std::string y_type = std::string(args[1]->front()->type().str());
      return err(args[1], "array.concat: operand 2 must be array but got " + y_type, "eval_type_error");
    }
    Node y = *maybe_y;

    Node z = x->clone();
    y = y->clone();
    z->insert(z->end(), y->begin(), y->end());
    return z;
  }

  Node reverse(const Nodes& args)
  {
    auto maybe_arr = Resolver::maybe_unwrap_array(args[0]);
    if(!maybe_arr.has_value()){
      std::string arr_type = std::string(args[0]->front()->type().str());
      return err(args[0], "array.reverse: operand 1 must be array but got " + arr_type, "eval_type_error");
    }

    Node arr = *maybe_arr;
    Node rev = NodeDef::create(Array);
    if(arr->size() == 0){
      return rev;
    }

    for(auto it = arr->rbegin(); it != arr->rend(); ++it){
      Node node = *it;
      rev->push_back(node->clone());
    }
    
    return rev;
  }

  Node slice(const Nodes& args)
  {
    Node arr = args[0];
    auto maybe_start_number = Resolver::maybe_unwrap_number(args[1]);
    auto maybe_end_number = Resolver::maybe_unwrap_number(args[2]);

    if (!maybe_start_number.has_value())
    {
      return err(args[1], "start index must be a number");
    }

    if (!maybe_end_number.has_value())
    {
      return err(args[2], "end index must be a number");
    }

    Node start_number = maybe_start_number.value();
    Node end_number = maybe_end_number.value();

    if (start_number->type() != JSONInt)
    {
      return err(args[1], "start index must be an integer");
    }

    if (end_number->type() != JSONInt)
    {
      return err(args[2], "end index must be an integer");
    }

    std::int64_t raw_start = BigInt(start_number->location()).to_int();
    std::int64_t raw_end = BigInt(end_number->location()).to_int();
    std::size_t start, end;
    if (raw_start < 0)
    {
      start = 0;
    }
    else
    {
      start = std::min(static_cast<std::size_t>(raw_start), arr->size());
    }

    if (raw_end < 0)
    {
      end = 0;
    }
    else
    {
      end = std::min(static_cast<std::size_t>(raw_end), arr->size());
    }

    Node array = NodeDef::create(Array);

    if (start >= end)
    {
      return array;
    }

    auto start_it = arr->begin() + start;
    auto end_it = arr->begin() + end;
    for (auto it = start_it; it != end_it; ++it)
    {
      Node node = *it;
      array->push_back(node->clone());
    }

    return array;
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> arrays()
    {
      return {
        BuiltInDef::create(Location("array.concat"), 2, concat),
        BuiltInDef::create(Location("array.reverse"), 1, reverse),
        BuiltInDef::create(Location("array.slice"), 3, slice),
      };
    }
  }
}