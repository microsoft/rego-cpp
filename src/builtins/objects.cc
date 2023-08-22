#include "errors.h"
#include "register.h"
#include "resolver.h"
#include "errors.h"
#include "utils.h"

namespace
{
  using namespace rego;

  std::vector<std::string> get_keys(const Node& collection)
  {
    std::vector<std::string> keys;
    if (collection->type() == Array || collection->type() == Set)
    {
      for (auto& item : *collection)
      {
        keys.push_back(to_json(item));
      }
    }
    else if (collection->type() == Object)
    {
      for (auto& item : *collection)
      {
        keys.push_back(to_json(item / Key));
      }
    }
    else
    {
      throw std::runtime_error("collection must be an array, set, or object");
    }

    return keys;
  }

  std::set<std::string> get_key_set(const Node& collection)
  {
    std::vector<std::string> keys = get_keys(collection);
    return std::set<std::string>(keys.begin(), keys.end());
  }

  Node filter(const Nodes& args)
  {
    Node object = Resolver::unwrap(
      args[0], Object, "object.filter: operand 1 ", EvalTypeError);
    if (object->type() == Error)
    {
      return object;
    }

    auto maybe_keys = Resolver::maybe_unwrap(args[1], {Array, Set, Object});
    if (!maybe_keys.has_value())
    {
      std::string name = Resolver::type_name(args[1]);
      return err(args[1], "object.filter: operand 2 must be one of {object, set, array} but got " + name, EvalTypeError);
    }

    auto keys = get_key_set(maybe_keys.value());
    Node filtered = NodeDef::create(Object);
    for (auto& item : *object)
    {
      std::string key_str = to_json(item / Key);
      if (keys.contains(key_str))
      {
        filtered->push_back(item->clone());
      }
    }

    return filtered;
  }

  std::optional<Node> get_key(
    const Node& node, const Node& keys, std::size_t index)
  {
    if (index == keys->size())
    {
      return node;
    }

    auto maybe_collection = Resolver::maybe_unwrap(node, {Array, Object});
    if(!maybe_collection.has_value()){
      return std::nullopt;
    }

    Node collection = maybe_collection.value();
    if (collection->type() == Object)
    {
      Node key = keys->at(index);
      std::string key_str = to_json(key);
      for (auto& item : *collection)
      {
        if (to_json(item / Key) == key_str)
        {
          return get_key(item / Val, keys, index + 1);
        }
      }
    }
    else if (collection->type() == Array)
    {
      auto maybe_key = Resolver::maybe_unwrap_int(keys->at(index));
      if(!maybe_key.has_value()){
        return std::nullopt;
      }

      Node key = maybe_key.value();
      std::size_t i = BigInt(key->location()).to_size();
      return get_key(collection->at(i), keys, index + 1);
    }

    return std::nullopt;
  }

  Node get(const Nodes& args)
  {
    Node object =
      Resolver::unwrap(args[0], Object, "object.get: operand 1 ", EvalTypeError);
    if (object->type() == Error)
    {
      return object;
    }

    Node key = args[1];
    if (key->type() != Array)
    {
      key = Array << key;
    }

    auto maybe_value = get_key(object, key, 0);
    if (maybe_value.has_value())
    {
      return maybe_value.value();
    }

    return args[2];
  }

  Node keys(const Nodes& args)
  {
    Node object = Resolver::unwrap(
      args[0], Object, "object.keys: operand 1 ", EvalTypeError);
    if (object->type() == Error)
    {
      return object;
    }

    Node value = NodeDef::create(Set);
    for (auto& item : *object)
    {
      value->push_back(item / Key);
    }

    return value;
  }

  Node remove(const Nodes& args)
  {
    Node object = Resolver::unwrap(
      args[0], Object, "object.remove: operand 1 ", EvalTypeError);
    if (object->type() == Error)
    {
      return object;
    }

    auto maybe_keys = Resolver::maybe_unwrap(args[1], {Array, Set, Object});
    if (!maybe_keys.has_value())
    {
      std::string name = Resolver::type_name(args[1]);
      return err(args[1], "object.remove: operand 2 must be one of {object, set, array} but got " + name, EvalTypeError);
    }

    auto keys = get_key_set(maybe_keys.value());
    Node output = NodeDef::create(Object);
    for (auto& item : *object)
    {
      std::string key_str = to_json(item / Key);
      if (!keys.contains(key_str))
      {
        output->push_back(item->clone());
      }
    }

    return output;
  }

  std::map<std::string, Node> to_map(Node object)
  {
    std::map<std::string, Node> map;
    for (auto& item : *object)
    {
      map[to_json(item / Key)] = (item / Val)->front();
    }

    return map;
  }

  /** This function checks if sub appears contiguously in order within super,
   *  and also does not attempt to recurse.
   */
  bool array_subset(Node& super, Node& sub)
  {
    if (super->size() < sub->size())
    {
      return false;
    }

    auto super_keys = get_keys(super);
    auto sub_keys = get_keys(sub);
    std::string first = sub_keys[0];
    auto pos = std::find(super_keys.begin(), super_keys.end(), first);
    while (pos != super_keys.end())
    {
      auto remaining = std::distance(pos, super_keys.end());
      if (static_cast<std::size_t>(remaining) < sub_keys.size())
      {
        return false;
      }

      bool is_subset = true;
      for (std::size_t i = 0; i < sub_keys.size(); i++)
      {
        if (sub_keys[i] != pos[i])
        {
          is_subset = false;
          break;
        }
      }

      if (is_subset)
      {
        return true;
      }

      pos = std::find(pos + 1, super_keys.end(), first);
    }

    return false;
  }

  /** This function checks if every element of sub is a member of super,
   *  but does not attempt to recurse
   */
  bool set_subset(Node& super, Node& sub)
  {
    if (super->size() < sub->size())
    {
      return false;
    }
    auto super_keys = get_key_set(super);
    auto sub_keys = get_key_set(sub);
    for(auto& key : sub_keys){
      if(!super_keys.contains(key)){
        return false;
      }
    }
    
    return true;
  }

  /** This function checks if super contains every element of sub with
   *  no consideration of ordering, and also does not attempt to recurse.
   */
  bool set_array_subset(Node& super, Node& sub)
  {
    if (super->size() < sub->size())
    {
      return false;
    }
    auto super_keys = get_key_set(super);
    auto sub_keys = get_key_set(sub);
    for(auto& key : sub_keys){
      if(!super_keys.contains(key)){
        return false;
      }
    }
    
    return true;
  }

  bool object_subset(Node& super, Node& sub);

  bool is_subset(Node& super, Node& sub)
  {
      if (sub->type() == Object && super->type() == Object)
      {
        return object_subset(super, sub);
      }
      
      if (sub->type() == Array && super->type() == Array)
      {
        return array_subset(super, sub);
      }
      
      if (sub->type() == Set)
      {
        if (super->type() == Set)
        {
          return set_subset(super, sub);
        }
        else if (super->type() == Array)
        {
          return set_array_subset(super, sub);
        }
        else
        {
          return false;
        }
      }

      if (super->type() == sub->type())
      {
        return to_json(super) == to_json(sub);
      }

      return false;
  }

  /** Object sub is a subset of object super if and only if every
   *  key in sub is also in super, and for all keys which sub and
   *  super share, they have the same value. The  operation is recursive,
   *  e.g. {"c": {"x": {10, 15, 20}} is a subset of {"a": "b", "c":
   *  {"x": {10, 15, 20, 25}, "y": "z"}
   */
  bool object_subset(Node& super, Node& sub)
  {
    if (super->size() < sub->size())
    {
      return false;
    }

    auto super_map = to_map(super);
    auto sub_map = to_map(sub);

    for (auto& [key, value] : sub_map)
    {
      if (!super_map.contains(key))
      {
        return false;
      }

      Node super_value = super_map[key];
      if (!is_subset(super_value, value))
      {
        return false;
      }
    }

    return true;
  }

  Node subset(const Nodes& args)
  {
    auto maybe_super = Resolver::maybe_unwrap(args[0], {Array, Set, Object});
    if (!maybe_super.has_value())
    {
      std::string name = Resolver::type_name(args[0]);
      return err(args[0], "object.subset: operand 1 must be one of {object, set, array} but got " + name, EvalTypeError);
    }

    auto maybe_sub = Resolver::maybe_unwrap(args[1], {Array, Set, Object});
    if (!maybe_sub.has_value())
    {
      std::string name = Resolver::type_name(args[0]);
      return err(args[1], "object.subset: operand 2 must be one of {object, set, array} but got " + name, EvalTypeError);
    }

    Node super = *maybe_super;
    Node sub = *maybe_sub;

    if (is_subset(super, sub))
    {
      return JSONTrue ^ "true";
    }

    return JSONFalse ^ "false";
  }

  Node object_union(const Node& lhs, const Node& rhs)
  {
    Node output = rhs->clone();
    auto rhs_keys = get_key_set(rhs);
    for (auto& item : *lhs)
    {
      std::string key_str = to_json(item / Key);
      if (!rhs_keys.contains(key_str))
      {
        output->push_back(item->clone());
      }
    }

    return output;
  }

  Node union_(const Nodes& args)
  {
    Node a = Resolver::unwrap(
      args[0], Object, "object.union: operand 1 ", EvalTypeError);
    if (a->type() == Error)
    {
      return a;
    }

    Node b = Resolver::unwrap(
      args[1], Object, "object.union: operand 2 ", EvalTypeError);
    if (b->type() == Error)
    {
      return b;
    }

    return object_union(a, b);
  }

  Node union_n(const Nodes& args)
  {
    Node objects = Resolver::unwrap(
      args[0], Array, "object.union_n: operand 1 ", EvalTypeError);
    if (objects->type() == Error)
    {
      return objects;
    }

    Node output = NodeDef::create(Object);
    for (auto& item : *objects)
    {
      Node object = Resolver::unwrap(
        item, Object, "object.union_n: operand 1 ", EvalTypeError);
      if (object->type() == Error)
      {
        return object;
      }

      output = object_union(output, object);
    }

    return output;
  }
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> objects()
    {
      return {
        BuiltInDef::create(Location("object.filter"), 2, filter),
        BuiltInDef::create(Location("object.get"), 3, get),
        BuiltInDef::create(Location("object.keys"), 1, keys),
        BuiltInDef::create(Location("object.remove"), 2, remove),
        BuiltInDef::create(Location("object.subset"), 2, subset),
        BuiltInDef::create(Location("object.union"), 2, union_),
        BuiltInDef::create(Location("object.union_n"), 1, union_n)};
    };
  }
}