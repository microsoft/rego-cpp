#include "internal.hh"

namespace
{
  using namespace rego;

  Node merge(Node dst, Node src)
  {
    if (dst->type() == DataModule && contains(RuleTypes, src->type()))
    {
      Location key = src->front()->location();
      auto target = std::find_if(dst->begin(), dst->end(), [key](auto& item) {
        return item->front()->location() == key;
      });

      if (target == dst->end())
      {
        dst->push_back(src);
      }
      else
      {
        Node dst_rule = *target;
        if (dst_rule->type() != DataRule)
        {
          // NB virtual doc rules are only merged if there is a name conflict
          // with a non-virtual doc rule. This seems counter-intuitive but
          // is correct as of v0.55.0 of Rego.
          dst->push_back(src);
        }
      }

      return dst;
    }
    else if (dst->type() == DataModule && src->type() == Submodule)
    {
      Location key = src->front()->location();
      auto target = std::find_if(dst->begin(), dst->end(), [key](auto& item) {
        return item->front()->location() == key;
      });

      if (target == dst->end())
      {
        dst->push_back(src);
      }
      else
      {
        Node dst_module = (*target)->back();
        Node src_module = src->back();
        Node result = merge(dst_module, src_module);
        if (result->type() == Error)
        {
          return result;
        }
      }

      return dst;
    }
    else if (dst->type() == DataModule && src->type() == DataModule)
    {
      for (auto& child : *src)
      {
        merge(dst, child);
      }

      return dst;
    }
    else
    {
      std::cout << dst << std::endl;
      std::cout << src << std::endl;
      return err(src, "Unsupported merge");
    }
  }
}

namespace rego
{
  // Merges all the Modules in ModuleSeq into the Data node. As part of this it
  // will create submodules and preserve a single hierarhical structure, such
  // that two packages a.b.c and a.b.d will be merged into a single module b,
  // which is a child of a.
  PassDef merge_modules()
  {
    return {
      In(ModuleSeq) *
          (T(Module)
           << ((
                 T(Package)
                 << (T(Ref)
                     << ((T(RefHead) << T(Var)[Var]) *
                         T(RefArgSeq)[RefArgSeq]))) *
               T(Policy)[Policy])) >>
        [](Match& _) {
          Node args = _(RefArgSeq);
          Node module = DataModule << *_[Policy];
          while (args->size() > 0)
          {
            Node arg = args->back();
            args->pop_back();
            if (arg->type() == RefArgDot)
            {
              module = DataModule
                << (Submodule << (Key ^ arg->front()) << module);
            }
            else if (arg->type() == RefArgBrack)
            {
              if (arg->front()->type() != Scalar)
              {
                return err(arg, "Invalid package ref index");
              }

              Node idx = arg->front()->front();
              if (idx->type() != JSONString)
              {
                return err(idx, "Invalid package ref index");
              }

              std::string key = strip_quotes(idx->location().view());
              if (!all_alnum(key))
              {
                key = "[\"" + key + "\"]";
              }
              module = DataModule << (Submodule << (Key ^ key) << module);
            }
            else
            {
              return err(arg, "Unsupported package ref arg type");
            }
          }

          return Lift << Rego << (Submodule << (Key ^ _(Var)) << module);
        },

      In(Rego) * (T(ModuleSeq) << End) >> ([](Match&) -> Node { return {}; }),

      In(Rego) *
          ((T(Data) << (T(Key)[Key] * T(DataModule)[DataModule])) *
           T(Submodule)[Submodule]) >>
        [](Match& _) {
          return Data << _(Key) << merge(_(DataModule), _(Submodule));
        },

      // errors

      In(ModuleSeq) * (T(Module) << T(Package)[Package]) >>
        [](Match& _) { return err(_(Package), "Invalid package reference."); },

      In(Rego) * (T(ModuleSeq) << T(Error)[Error]) >>
        [](Match& _) { return _(Error); },

      In(DataModule) * T(Import)[Import] >>
        [](Match& _) { return err(_(Import), "Invalid import"); }};
  }
}