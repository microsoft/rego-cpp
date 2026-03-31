#include "builtins.hh"
#include "trieste/json.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node member_2(const Nodes& args)
  {
    Node item = args[0];
    Node itemseq = args[1];
    return Resolver::membership(item, itemseq);
  }

  BuiltIn member_2_factory()
  {
    const Node member_2_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "item") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "itemseq") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if `item` is a member of `itemseq`")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"internal.member_2"}, member_2_decl, member_2);
  }

  Node member_3(const Nodes& args)
  {
    Node index = args[0];
    Node item = args[1];
    Node itemseq = args[2];
    return Resolver::membership(index, item, itemseq);
  }

  BuiltIn member_3_factory()
  {
    const Node member_3_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "index") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "item") << bi::Description
                                 << (bi::Type << bi::Any))
                     << (bi::Arg << (bi::Name ^ "itemseq") << bi::Description
                                 << (bi::Type << bi::Any)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if (`index`, `item`) is a member of `itemseq`")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"internal.member_3"}, member_3_decl, member_3);
  }

  Node print(const Nodes& args)
  {
    // Simplified print: does not wrap arguments in set comprehensions or
    // support cross-product expansion. See GitHub issue #209 for full
    // OPA-equivalent implementation.
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

  BuiltIn print_factory()
  {
    const Node print_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << bi::Name << bi::Description
                               << (bi::Type
                                   << (bi::DynamicArray
                                       << (bi::Type << (bi::Set << bi::Any))))))
               << bi::Void;
    return BuiltInDef::create({"internal.print"}, print_decl, ::print);
  }

  std::string stringify_value(const Node& node)
  {
    Node inner = node;
    if (inner == Scalar)
    {
      inner = inner->front();
    }

    if (inner->type() == JSONString)
    {
      return get_string(inner);
    }

    std::string key = to_key(inner, SetFormat::Rego, false, ", ");
    // to_key renders strings with surrounding quotes (e.g. "foo") inside
    // compound values. Those quotes must be escaped so the template result
    // can be stored as a valid JSONString via Resolver::scalar.
    std::ostringstream result;
    for (char c : key)
    {
      if (c == '"')
      {
        result << '\\';
      }
      result << c;
    }
    return result.str();
  }

  // Helper: append a string to an output stream, escaping control characters
  // for valid JSON storage via Resolver::scalar. Backslashes and quotes from
  // stringify_value are already correct and must not be re-escaped.
  void escape_append(std::ostringstream& out, const std::string& s)
  {
    static const char hex_chars[] = "0123456789abcdef";
    for (char c : s)
    {
      switch (c)
      {
        case '\n':
          out << '\\' << 'n';
          break;
        case '\r':
          out << '\\' << 'r';
          break;
        case '\t':
          out << '\\' << 't';
          break;
        case '\b':
          out << '\\' << 'b';
          break;
        case '\f':
          out << '\\' << 'f';
          break;
        default:
          if (static_cast<unsigned char>(c) < 0x20)
          {
            // Escape remaining control characters as \uXXXX
            out << "\\u00"
                << hex_chars[(static_cast<unsigned char>(c) >> 4) & 0x0F]
                << hex_chars[static_cast<unsigned char>(c) & 0x0F];
          }
          else
          {
            out << c;
          }
          break;
      }
    }
  }

  Node template_string(const Nodes& args)
  {
    Node arr = unwrap_arg(
      args, UnwrapOpt(0).type(Array).func("internal.template_string"));
    if (arr->type() == Error)
    {
      return arr;
    }

    std::ostringstream buf;
    for (const auto& elem : *arr)
    {
      // Array elements are Terms; access the inner node directly.
      Node item = elem->front();

      if (item == rego::Set)
      {
        if (item->size() == 0)
        {
          buf << "<undefined>";
        }
        else if (item->size() == 1)
        {
          escape_append(buf, stringify_value(item->front()->front()));
        }
        else
        {
          return err(
            arr,
            "eval_conflict_error: template-strings must not produce multiple "
            "outputs",
            EvalConflictError);
        }
      }
      else
      {
        escape_append(buf, stringify_value(item));
      }
    }

    return Resolver::scalar(buf.str());
  }

  BuiltIn template_string_factory()
  {
    const Node template_string_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg
                       << (bi::Name ^ "parts")
                       << (bi::Description ^ "array of template string parts")
                       << (bi::Type
                           << (bi::DynamicArray << (bi::Type << bi::Any)))))
               << (bi::Result << (bi::Name ^ "output")
                              << (bi::Description ^ "composed template string")
                              << (bi::Type << bi::String));
    return BuiltInDef::create(
      {"internal.template_string"}, template_string_decl, template_string);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn internal(const Location& name)
    {
      assert(name.view().starts_with("internal."));
      std::string_view view = name.view().substr(9); // skip "internal."
      if (view == "member_2")
      {
        return member_2_factory();
      }
      else if (view == "member_3")
      {
        return member_3_factory();
      }
      else if (view == "print")
      {
        return print_factory();
      }
      else if (view == "template_string")
      {
        return template_string_factory();
      }

      return nullptr;
    }
  }
}