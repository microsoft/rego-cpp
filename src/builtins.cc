#include "builtins/builtins.h"

#include "internal.hh"
#include "rego.hh"

#include <stdexcept>

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node opa_runtime(const Nodes&)
  {
    return version();
  }

  Node opa_runtime_decl = bi::Decl
    << bi::ArgSeq
    << (bi::Result
        << (bi::Name ^ "output")
        << (bi::Description ^
            "includes a `config` key if OPA was started with a configuration "
            "file; an `env` key containing the environment variables that the "
            "OPA process was started with; includes `version` and `commit` "
            "keys containing the version and build commit of OPA")
        << (bi::Type
            << (bi::DynamicObject << (bi::Type << bi::String)
                                  << (bi::Type << bi::Any))));

  Node print(const Nodes& args)
  {
    for (auto arg : args)
    {
      if (arg->type() == Undefined)
      {
        return Resolver::scalar(false);
      }
    }

    join(
      std::cout,
      args.begin(),
      args.end(),
      " ",
      [](std::ostream& stream, const Node& n) {
        stream << to_key(n);
        return true;
      })
      << std::endl;
    return Resolver::scalar(true);
  }

  Node print_decl = bi::Decl << bi::VarArgs << bi::Void;

  Node walk(const Nodes& args)
  {
    Node x = args[0];

    Node result = NodeDef::create(Array);
    std::deque<std::pair<Nodes, Node>> queue;
    queue.push_back({{}, x});
    while (!queue.empty())
    {
      auto [path_nodes, current] = queue.front();
      queue.pop_front();

      Node path_array = NodeDef::create(Array);
      for (auto& node : path_nodes)
      {
        path_array->push_back(Resolver::to_term(node->clone()));
      }

      result
        << (Term
            << (Array << (Term << path_array)
                      << Resolver::to_term(current->clone())));

      auto maybe_node = unwrap(current, {Array, Set, Object});
      if (!maybe_node.success)
      {
        continue;
      }

      current = maybe_node.node;
      if (current == Array)
      {
        for (size_t i = 0; i < current->size(); i++)
        {
          Node index = Resolver::term(BigInt(i));
          Nodes path(path_nodes);
          path.push_back(index);
          queue.push_back({path, current->at(i)});
        }
      }
      else if (current == Set)
      {
        for (auto child : *current)
        {
          Nodes path(path_nodes);
          path.push_back(child);
          queue.push_back({path, child});
        }
      }
      else if (current == Object)
      {
        for (auto child : *current)
        {
          Nodes path(path_nodes);
          path.push_back(child / Key);
          queue.push_back({path, child / Val});
        }
      }
    }

    return Term << result;
  }

  Node walk_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "x")
                             << (bi::Description ^ "value to walk")
                             << (bi::Type << bi::Any)))
             << (bi::Result
                 << (bi::Name ^ "output")
                 << (bi::Description ^
                     "pairs of `path` and `value`: `path` is an array "
                     "representing the pointer to `value` in `x`. If `path` is "
                     "assigned a wildcard (`_`), the `walk` function will skip "
                     "path creation entirely for faster evaluation.")
                 << (bi::Type
                     << (bi::DynamicArray
                         << (bi::Type
                             << (bi::StaticArray
                                 << (bi::Type
                                     << (bi::DynamicArray
                                         << (bi::Type << bi::Any)))
                                 << (bi::Type << bi::Any))))));

  struct NotAvailable : public BuiltInDef
  {
    NotAvailable(Location name, Node decl, const std::string& message) :
      BuiltInDef(
        name,
        decl,
        [decl, message](const Nodes&) { return call(decl, message); },
        false)
    {}

    static Node call(Node decl, const std::string& message)
    {
      return err(decl, message, EvalBuiltInError);
    }
  };
}

namespace rego
{
  BuiltInDef::BuiltInDef(
    Location name_, Node decl_, BuiltInBehavior behavior_, bool available_)
  {
    WFContext context(bi::wf_decl);
    Node args = decl_ / bi::Args;
    if (args == bi::ArgSeq)
    {
      arity = args->size();
    }
    else if (args == bi::VarArgs)
    {
      arity = AnyArity;
    }
    else
    {
      throw std::runtime_error("Invalid builtin decl");
    }

    name = name_;
    decl = decl_;
    behavior = behavior_;
    available = available_;
  }

  void BuiltInDef::clear() {}

  BuiltIn BuiltInDef::create(
    const Location& name, Node decl, BuiltInBehavior behavior)
  {
    return std::make_shared<BuiltInDef>(name, decl, behavior, true);
  }

  BuiltIn BuiltInDef::create(
    const Location& name, size_t arity, BuiltInBehavior behavior)
  {
    Node argseq = NodeDef::create(bi::ArgSeq);
    for (size_t i = 0; i < arity; ++i)
    {
      argseq << bi::Arg << bi::Name << bi::Description << (bi::Type << bi::Any);
    }

    Node decl = bi::Decl << argseq
                         << (bi::Result << bi::Name << bi::Description
                                        << (bi::Type << bi::Any));
    return std::make_shared<BuiltInDef>(name, decl, behavior, true);
  }

  BuiltIn BuiltInDef::placeholder(
    const Location& name, Node decl, const std::string& message)
  {
    auto ptr =
      std::make_shared<NotAvailable>(NotAvailable(name, decl, message));
    return ptr;
  }

  BuiltInsDef::BuiltInsDef() noexcept : m_strict_errors(false) {}

  void BuiltInsDef::clear()
  {
    for (auto& builtin : m_builtins)
    {
      builtin.second->clear();
    }
  }

  bool BuiltInsDef::strict_errors() const
  {
    return m_strict_errors;
  }

  BuiltInsDef& BuiltInsDef::strict_errors(bool strict_errors)
  {
    m_strict_errors = strict_errors;
    return *this;
  }

  bool BuiltInsDef::is_builtin(const Location& name) const
  {
    return m_builtins.contains(name);
  }

  Node BuiltInsDef::decl(const Location& name) const
  {
    if (!is_builtin(name))
    {
      logging::Warn() << "Built-in not found: " << name.view();
      throw std::runtime_error("Built-in not found");
    }

    return m_builtins.at(name)->decl;
  }

  bool BuiltInsDef::is_deprecated(
    const Location& version, const Location& name) const
  {
    std::vector<std::string> deprecated = {
      "any",
      "all",
      "re_match",
      "net.cidr_overlap",
      "set_diff",
      "cast_array",
      "cast_set",
      "cast_string",
      "cast_boolean",
      "cast_null",
      "cast_object"};

    return std::find_if(
             deprecated.begin(),
             deprecated.end(),
             [name](const std::string& n) { return n == name.view(); }) !=
      deprecated.end();
  }

  Node BuiltInsDef::call(
    const Location& name, const Location& version, const Nodes& args)
  {
    if (!is_builtin(name))
    {
      return err(args[0], "unknown builtin");
    }

    if (is_deprecated(version, name))
    {
      std::ostringstream err_buff;
      err_buff << "deprecated built-in function calls in expression: "
               << name.view();
      return err(args[0], err_buff.str(), RegoTypeError);
    }

    auto& builtin = m_builtins.at(name);
    if (builtin->arity != AnyArity && builtin->arity != args.size())
    {
      return err(args[0], "wrong number of arguments");
    }

    for (auto& arg : args)
    {
      if (arg->type() == Error)
      {
        return arg;
      }
    }

    Node result = builtin->behavior(args);
    if (result->type() == Error)
    {
      if (m_strict_errors)
      {
        return result;
      }
      else
      {
        return Undefined;
      }
    }

    return result;
  }

  BuiltInsDef& BuiltInsDef::register_builtin(const BuiltIn& built_in)
  {
    m_builtins[built_in->name] = built_in;
    return *this;
  }

  BuiltInsDef& BuiltInsDef::register_standard_builtins()
  {
    register_builtins<std::initializer_list<BuiltIn>>({
      BuiltInDef::create(Location("print"), print_decl, ::print),
      BuiltInDef::create(
        Location("opa.runtime"), opa_runtime_decl, ::opa_runtime),
      BuiltInDef::create(Location("walk"), walk_decl, ::walk),
    });

    register_builtins(builtins::aggregates());
    register_builtins(builtins::arrays());
    register_builtins(builtins::bits());
    register_builtins(builtins::comparison());
    register_builtins(builtins::conversions());
    register_builtins(builtins::encoding());
    register_builtins(builtins::graph());
    register_builtins(builtins::internal());
    register_builtins(builtins::numbers());
    register_builtins(builtins::objects());
    register_builtins(builtins::regex());
    register_builtins(builtins::sets());
    register_builtins(builtins::semver());
    register_builtins(builtins::strings());
    register_builtins(builtins::time());
    register_builtins(builtins::types());
    register_builtins(builtins::units());
    register_builtins(builtins::uuid());

    return *this;
  }

  BuiltIns BuiltInsDef::create()
  {
    BuiltIns builtins = std::make_shared<BuiltInsDef>();
    builtins->register_standard_builtins();
    return builtins;
  }

  std::map<Location, BuiltIn>::const_iterator BuiltInsDef::begin() const
  {
    return m_builtins.begin();
  }

  std::map<Location, BuiltIn>::const_iterator BuiltInsDef::end() const
  {
    return m_builtins.end();
  }

  const BuiltIn& BuiltInsDef::at(const Location& name) const
  {
    return m_builtins.at(name);
  }
}