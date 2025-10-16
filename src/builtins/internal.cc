#include "builtins.h"

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

  const Node member_2_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "item") << bi::Description
                               << (bi::Type << bi::Any))
                   << (bi::Arg << (bi::Name ^ "itemseq") << bi::Description
                               << (bi::Type << bi::Any)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^
                       "true if `item` is a member of `itemseq`")
                   << (bi::Type << bi::Boolean));

  Node member_3(const Nodes& args)
  {
    Node index = args[0];
    Node item = args[1];
    Node itemseq = args[2];
    return Resolver::membership(index, item, itemseq);
  }

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

  Node print(const Nodes& args)
  {
    // TODO implement this properly
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

  const Node print_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << bi::Name << bi::Description
                             << (bi::Type
                                 << (bi::DynamicArray
                                     << (bi::Type << (bi::Set << bi::Any))))))
             << bi::Void;
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> internal()
    {
      return {
        BuiltInDef::create(
          Location("internal.member_2"), member_2_decl, member_2),
        BuiltInDef::create(
          Location("internal.member_3"), member_3_decl, member_3),
        BuiltInDef::create(Location("internal.print"), print_decl, ::print)};
    }
  }
}