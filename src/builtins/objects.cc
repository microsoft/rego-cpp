#include "builtins.hh"
#include "rego.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  std::vector<std::string> get_keys(const Node& collection)
  {
    std::vector<std::string> keys;
    if (collection->type() == Array || collection->type() == Set)
    {
      for (auto& item : *collection)
      {
        keys.push_back(to_key(item));
      }
    }
    else if (collection->type() == Object)
    {
      for (auto& item : *collection)
      {
        keys.push_back(to_key(item / Key));
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
    Node object =
      unwrap_arg(args, UnwrapOpt(0).func("object.filter").type(Object));
    if (object->type() == Error)
    {
      return object;
    }

    Node keys_node = unwrap_arg(
      args, UnwrapOpt(1).func("object.filter").types({Object, Set, Array}));
    if (keys_node->type() == Error)
    {
      return keys_node;
    }

    auto keys = get_key_set(keys_node);
    Node filtered = NodeDef::create(Object);
    for (auto& item : *object)
    {
      std::string key_str = to_key(item / Key);
      if (keys.contains(key_str))
      {
        filtered->push_back(item->clone());
      }
    }

    return filtered;
  }

  BuiltIn filter_factory()
  {
    const Node filter_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "object")
                      << (bi::Description ^ "object to filter keys")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg
              << (bi::Name ^ "keys")
              << (bi::Description ^ "keys to keep in `object`")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type
                          << (bi::DynamicArray << (bi::Type << bi::Any)))
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any)))
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))))
      << (bi::Result
          << (bi::Name ^ "filtered")
          << (bi::Description ^
              "remaining data from `object` with only keys specified in `keys`")
          << (bi::Type << bi::Any));
    return BuiltInDef::create({"object.filter"}, filter_decl, filter);
  }

  std::optional<Node> get_key(
    const Node& node, const Node& keys, std::size_t index)
  {
    if (index == keys->size())
    {
      return node;
    }

    auto maybe_collection = unwrap(node, {Array, Object});
    if (!maybe_collection.success)
    {
      return std::nullopt;
    }

    Node collection = maybe_collection.node;
    if (collection->type() == Object)
    {
      Node key = keys->at(index);
      std::string key_str = to_key(key);
      for (auto& item : *collection)
      {
        if (to_key(item / Key) == key_str)
        {
          return get_key(item / Val, keys, index + 1);
        }
      }
    }
    else if (collection->type() == Array)
    {
      auto maybe_key = unwrap(keys->at(index), Int);
      if (!maybe_key.success)
      {
        return std::nullopt;
      }

      Node key = maybe_key.node;
      auto maybe_i = BigInt(key->location()).to_size();
      if (!maybe_i.has_value())
      {
        return std::nullopt;
      }
      std::size_t i = maybe_i.value();
      return get_key(collection->at(i), keys, index + 1);
    }

    return std::nullopt;
  }

  Node get(const Nodes& args)
  {
    Node object =
      unwrap_arg(args, UnwrapOpt(0).func("object.get").type(Object));
    if (object->type() == Error)
    {
      return object;
    }

    Node key = args[1]->clone();
    if (key->type() != Array)
    {
      key = Array << key;
    }

    auto maybe_value = get_key(object, key, 0);
    if (maybe_value.has_value())
    {
      return maybe_value.value()->clone();
    }

    return args[2]->clone();
  }

  BuiltIn get_factory()
  {
    const Node get_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "object")
                      << (bi::Description ^ "object to get `key` from")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg << (bi::Name ^ "key")
                      << (bi::Description ^ "key to lookup in `object`")
                      << (bi::Type << bi::Any))
          << (bi::Arg << (bi::Name ^ "default")
                      << (bi::Description ^ "default to use when lookup fails")
                      << (bi::Type << bi::Any)))
      << (bi::Result << (bi::Name ^ "value")
                     << (bi::Description ^
                         "`object[key]` if present, otherwise `default`")
                     << (bi::Type << bi::Any));
    return BuiltInDef::create({"object.get"}, get_decl, get);
  }

  Node keys(const Nodes& args)
  {
    Node object =
      unwrap_arg(args, UnwrapOpt(0).func("object.keys").type(Object));
    if (object->type() == Error)
    {
      return object;
    }

    Node value = NodeDef::create(Set);
    for (auto& item : *object)
    {
      value->push_back((item / Key)->clone());
    }

    return value;
  }

  BuiltIn keys_factory()
  {
    const Node keys_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "object")
                       << (bi::Description ^ "object to get keys from")
                       << (bi::Type
                           << (bi::DynamicObject << (bi::Type << bi::Any)
                                                 << (bi::Type << bi::Any)))))
               << (bi::Result
                   << (bi::Name ^ "value")
                   << (bi::Description ^ "set of `object`'s keys")
                   << (bi::Type << (bi::Set << (bi::Type << bi::Any))));
    return BuiltInDef::create({"object.keys"}, keys_decl, keys);
  }

  Node remove_(const Nodes& args)
  {
    Node object =
      unwrap_arg(args, UnwrapOpt(0).func("object.remove").type(Object));
    if (object->type() == Error)
    {
      return object;
    }

    Node keys_node = unwrap_arg(
      args, UnwrapOpt(1).func("object.remove").types({Object, Set, Array}));
    if (keys_node->type() == Error)
    {
      return keys_node;
    }

    auto keys = get_key_set(keys_node);
    Node output = NodeDef::create(Object);
    for (auto& item : *object)
    {
      std::string key_str = to_key(item / Key);
      if (!keys.contains(key_str))
      {
        output->push_back(item->clone());
      }
    }

    return output;
  }

  BuiltIn remove_factory()
  {
    const Node remove_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "object")
                      << (bi::Description ^ "object to remove keys from")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg
              << (bi::Name ^ "keys")
              << (bi::Description ^ "keys to remove from `object`")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type
                          << (bi::DynamicArray << (bi::Type << bi::Any)))
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any)))
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))))
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "result of removing the specified `keys` from `object`")
          << (bi::Type << bi::Any));
    return BuiltInDef::create({"object.remove"}, remove_decl, remove_);
  }

  std::map<std::string, Node> to_map(Node object)
  {
    std::map<std::string, Node> map;
    for (auto& item : *object)
    {
      map[to_key(item / Key)] = (item / Val)->front();
    }

    return map;
  }

  /// This function checks if sub appears contiguously in order within super,
  /// and also does not attempt to recurse.
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

  /// This function checks if every element of sub is a member of super,
  ///  but does not attempt to recurse
  bool set_subset(Node& super, Node& sub)
  {
    if (super->size() < sub->size())
    {
      return false;
    }
    auto super_keys = get_key_set(super);
    auto sub_keys = get_key_set(sub);
    for (auto& key : sub_keys)
    {
      if (!super_keys.contains(key))
      {
        return false;
      }
    }

    return true;
  }

  /// This function checks if super contains every element of sub with
  ///  no consideration of ordering, and also does not attempt to recurse.
  bool set_array_subset(Node& super, Node& sub)
  {
    if (super->size() < sub->size())
    {
      return false;
    }
    auto super_keys = get_key_set(super);
    auto sub_keys = get_key_set(sub);
    for (auto& key : sub_keys)
    {
      if (!super_keys.contains(key))
      {
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
      return to_key(super) == to_key(sub);
    }

    return false;
  }

  /// Object sub is a subset of object super if and only if every
  /// key in sub is also in super, and for all keys which sub and
  /// super share, they have the same value. The  operation is recursive,
  /// e.g. {"c": {"x": {10, 15, 20}} is a subset of {"a": "b", "c":
  /// {"x": {10, 15, 20, 25}, "y": "z"}
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
    Node super = unwrap_arg(
      args, UnwrapOpt(0).func("object.subset").types({Object, Set, Array}));
    if (super->type() == Error)
    {
      return super;
    }

    Node sub = unwrap_arg(
      args, UnwrapOpt(1).func("object.subset").types({Object, Set, Array}));
    if (sub->type() == Error)
    {
      return sub;
    }

    if (is_subset(super, sub))
    {
      return True ^ "true";
    }

    return False ^ "false";
  }

  BuiltIn subset_factory()
  {
    const Node subset_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg
              << (bi::Name ^ "super")
              << (bi::Description ^ "object to test if sub is a subset of")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type
                          << (bi::DynamicArray << (bi::Type << bi::Any)))
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any)))
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any))))))
          << (bi::Arg
              << (bi::Name ^ "sub")
              << (bi::Description ^ "object to test if super is a superset of")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type
                          << (bi::DynamicArray << (bi::Type << bi::Any)))
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any)))
                      << (bi::Type << (bi::Set << (bi::Type << bi::Any)))))))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "`true` if `sub` is a subset of `super`")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"object.subset"}, subset_decl, subset);
  }

  Node object_union(const Node& lhs, const Node& rhs)
  {
    Node output = rhs->clone();
    auto rhs_keys = get_key_set(rhs);
    for (auto& item : *lhs)
    {
      std::string key_str = to_key(item / Key);
      if (!rhs_keys.contains(key_str))
      {
        output->push_back(item->clone());
      }
    }

    return output;
  }

  Node union_(const Nodes& args)
  {
    Node a = unwrap_arg(args, UnwrapOpt(0).func("object.union").type(Object));
    if (a->type() == Error)
    {
      return a;
    }

    Node b = unwrap_arg(args, UnwrapOpt(1).func("object.union").type(Object));
    if (b->type() == Error)
    {
      return b;
    }

    return object_union(a, b);
  }

  BuiltIn union_factory()
  {
    const Node union_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg
                         << (bi::Name ^ "a")
                         << (bi::Description ^ "left-hand object")
                         << (bi::Type
                             << (bi::DynamicObject << (bi::Type << bi::Any)
                                                   << (bi::Type << bi::Any))))
                     << (bi::Arg
                         << (bi::Name ^ "b")
                         << (bi::Description ^ "right-hand object")
                         << (bi::Type
                             << (bi::DynamicObject << (bi::Type << bi::Any)
                                                   << (bi::Type << bi::Any)))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "a new object which is the result of an asymmetric "
                         "union of two objects where conflicts are resolved by "
                         "choosing the key from the right-hand object `b`")
                     << (bi::Type
                         << (bi::DynamicObject << (bi::Type << bi::Any)
                                               << (bi::Type << bi::Any))));
    return BuiltInDef::create({"object.union"}, union_decl, union_);
  }

  Node union_n(const Nodes& args)
  {
    Node objects =
      unwrap_arg(args, UnwrapOpt(0).func("object.union_n").type(Array));
    if (objects->type() == Error)
    {
      return objects;
    }

    Node output = NodeDef::create(Object);
    for (auto& item : *objects)
    {
      Node object =
        unwrap_arg({item}, UnwrapOpt(0).type(Object).func("object.union_n"));
      if (object->type() == Error)
      {
        return object;
      }

      output = object_union(output, object);
    }

    return output;
  }

  BuiltIn union_n_factory()
  {
    const Node union_n_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "objects")
                      << (bi::Description ^ "list of objects to merge")
                      << (bi::Type
                          << (bi::DynamicArray
                              << (bi::Type
                                  << (bi::DynamicObject
                                      << (bi::Type << bi::Any)
                                      << (bi::Type << bi::Any)))))))
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "asymmetric recursive union of all objects in `objects`, "
              "merged from left to right, where conflicts are resolved "
              "by choosing the key from the right-hand object")
          << (bi::Type
              << (bi::DynamicObject << (bi::Type << bi::Any)
                                    << (bi::Type << bi::Any))));
    return BuiltInDef::create({"object.union_n"}, union_n_decl, union_n);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn object(const Location& name)
    {
      assert(name.view().starts_with("object."));
      std::string_view view = name.view().substr(7); // skip "object."
      if (view == "filter")
      {
        return filter_factory();
      }
      if (view == "get")
      {
        return get_factory();
      }
      if (view == "keys")
      {
        return keys_factory();
      }
      if (view == "remove")
      {
        return remove_factory();
      }
      if (view == "subset")
      {
        return subset_factory();
      }
      if (view == "union")
      {
        return union_factory();
      }
      if (view == "union_n")
      {
        return union_n_factory();
      }

      return nullptr;
    }
  }
}