#include "builtins/builtins.hh"

#include "rego.hh"

#include <iterator>
#include <stdexcept>

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

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
      arity = bi::AnyArity;
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

  BuiltInsDef::BuiltInsDef() noexcept :
    m_strict_errors(false),
    m_lookup_behavior(BuiltInsDef::LookupBehavior::AllowAll)
  {}

  void BuiltInsDef::clear()
  {
    for (auto& builtin : m_builtins)
    {
      if (builtin.second != nullptr)
      {
        builtin.second->clear();
      }
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

  bool BuiltInsDef::is_builtin(const Location& name)
  {
    return at(name) != nullptr;
  }

  Node BuiltInsDef::decl(const Location& name)
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

    auto& builtin = m_builtins[name];
    if (builtin->arity != bi::AnyArity && builtin->arity != args.size())
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
    return *this;
  }

  BuiltIns BuiltInsDef::create()
  {
    BuiltIns builtins = std::make_shared<BuiltInsDef>();
    return builtins;
  }

  BuiltIn BuiltInsDef::at(const Location& name)
  {
    if (!m_builtins.contains(name))
    {
      logging::Debug() << "Built-in " << name.view()
                       << " not found, looking up in standard library";
      BuiltIn builtin = lookup(name);
      switch (m_lookup_behavior)
      {
        case BuiltInsDef::LookupBehavior::Whitelist:
          if (!m_list.contains(name))
          {
            logging::Debug()
              << "Built-in " << name.view() << " not in whitelist";
            builtin = std::make_shared<NotAvailable>(
              name, builtin->decl, "Built-in not allowed by whitelist");
          }
          break;

        case BuiltInsDef::LookupBehavior::Blacklist:
          if (m_list.contains(name))
          {
            logging::Debug() << "Built-in " << name.view() << " in blacklist";
            builtin = std::make_shared<NotAvailable>(
              name, builtin->decl, "Built-in forbidden by blacklist");
          }
          break;

        case BuiltInsDef::LookupBehavior::AllowAll:
          break;

        default:
          throw std::runtime_error("Unsupported lookup behavior");
      }

      m_builtins[name] = builtin;
    }

    return m_builtins[name];
  }

  BuiltInsDef& BuiltInsDef::allow_all()
  {
    m_lookup_behavior = BuiltInsDef::LookupBehavior::AllowAll;
    m_builtins.clear();
    return *this;
  }

  BuiltIn BuiltInsDef::lookup(const Location& name)
  {
    std::string_view view = name.view();
    auto it = view.find('.');
    if (it == std::string::npos)
    {
      return builtins::core(name);
    }

    view = view.substr(0, it);
    const size_t length = view.size();
    const char first_char = view.front();
    const char last_char = view.back();
    // BEGIN auto-generated btree code
    if (length > 4)
    {
      if (length > 6)
      {
        if (length > 7)
        {
          if (length > 8)
          {
            if (first_char > 'b')
            {
              if (view == "providers")
              {
                return builtins::aws(name);
              }
            }
            if (view == "base64url")
            {
              return builtins::base64url(name);
            }
          }
          if (first_char > 'i')
          {
            if (view == "urlquery")
            {
              return builtins::urlquery(name);
            }
          }
          if (view == "internal")
          {
            return builtins::internal(name);
          }
        }
        if (first_char > 'g')
        {
          if (first_char > 'n')
          {
            if (view == "strings")
            {
              return builtins::strings(name);
            }
          }
          if (view == "numbers")
          {
            return builtins::numbers(name);
          }
        }
        if (view == "graphql")
        {
          return builtins::graphql(name);
        }
      }
      if (length > 5)
      {
        if (first_char > 'b')
        {
          if (first_char > 'c')
          {
            if (first_char > 'o')
            {
              if (view == "semver")
              {
                return builtins::semver(name);
              }
            }
            if (view == "object")
            {
              return builtins::object(name);
            }
          }
          if (view == "crypto")
          {
            return builtins::crypto(name);
          }
        }
        if (view == "base64")
        {
          return builtins::base64(name);
        }
      }
      if (first_char > 'a')
      {
        if (first_char > 'g')
        {
          if (first_char > 'r')
          {
            if (view == "units")
            {
              return builtins::units(name);
            }
          }
          if (view == "regex")
          {
            return builtins::regex(name);
          }
        }
        if (view == "graph")
        {
          return builtins::graph(name);
        }
      }
      if (view == "array")
      {
        return builtins::array(name);
      }
    }
    if (first_char > 'j')
    {
      if (first_char > 'r')
      {
        if (first_char > 't')
        {
          if (first_char > 'u')
          {
            if (view == "yaml")
            {
              return builtins::yaml(name);
            }
          }
          if (view == "uuid")
          {
            return builtins::uuid(name);
          }
        }
        if (view == "time")
        {
          return builtins::time(name);
        }
      }
      if (length > 3)
      {
        if (last_char > 'd')
        {
          if (view == "rego")
          {
            return builtins::rego(name);
          }
        }
        if (view == "rand")
        {
          return builtins::rand(name);
        }
      }
      if (first_char > 'n')
      {
        if (view == "opa")
        {
          return builtins::opa(name);
        }
      }
      if (view == "net")
      {
        return builtins::net(name);
      }
    }
    if (last_char > 'o')
    {
      if (length > 3)
      {
        if (first_char > 'b')
        {
          if (view == "http")
          {
            return builtins::http(name);
          }
        }
        if (view == "bits")
        {
          return builtins::bits(name);
        }
      }
      if (view == "hex")
      {
        return builtins::hex(name);
      }
    }
    if (length > 2)
    {
      if (first_char > 'g')
      {
        if (view == "json")
        {
          return builtins::json(name);
        }
      }
      if (view == "glob")
      {
        return builtins::glob(name);
      }
    }
    if (view == "io")
    {
      return builtins::jwt(name);
    }
    return nullptr;
    // END auto-generated btree code
  }
}