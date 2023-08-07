#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;

  Node merge(Node dst, Node src)
  {
    if (dst->type() == DataItemSeq && src->type() == DataItem)
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
    else if (dst->type() == Module && src->type() == Module)
    {
      for (auto& child : *src)
      {
        Location key = child->front()->location();
        auto target = std::find_if(dst->begin(), dst->end(), [key](auto& item) {
          return item->front()->location() == key;
        });
        if (target == dst->end())
        {
          dst->push_back(child);
        }
        else
        {
          Node dst_submodule = (*target)->back();
          Node src_submodule = child->back();
          if (dst_submodule->type() != Module)
          {
            return err(
              dst_submodule, "Cannot merge into a rule with the same name");
          }
          if (src_submodule->type() != Module)
          {
            return err(
              dst_submodule,
              "Cannot merge a rule with a submodule of the same name");
          }

          Node result = merge(dst_submodule, src_submodule);
          if (result->type() == Error)
          {
            return result;
          }
        }
      }

      return dst;
    }
    else
    {
      return err(src, "Unsupported merge");
    }
  }
}

namespace rego
{
  // Merges all the Modules in ModuleSeq into the Data node.
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
          Node module = Module << *_[Policy];
          while (args->size() > 0)
          {
            Node arg = args->back();
            args->pop_back();
            if (arg->type() == RefArgDot)
            {
              module = Module << (Submodule << (Key ^ arg->front()) << module);
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
              Location loc = idx->location();
              loc.pos += 1;
              loc.len -= 2;
              module = Module << (Submodule << (Key ^ loc) << module);
            }
            else
            {
              return err(arg, "Unsupported package ref arg type");
            }
          }

          return Lift << Rego << (DataItem << (Key ^ _(Var)) << module);
        },

      In(Rego) * (T(ModuleSeq) << End) >> ([](Match&) -> Node { return {}; }),

      In(Rego) *
          ((T(Data) << (T(Var)[Var] * T(DataItemSeq)[DataItemSeq])) *
           T(DataItem)[DataItem]) >>
        [](Match& _) {
          return Data << _(Var) << merge(_(DataItemSeq), _(DataItem));
        },

      // errors

      In(ModuleSeq) * (T(Module) << T(Package)[Package]) >>
        [](Match& _) { return err(_(Package), "Invalid package reference."); },

      In(Rego) * (T(ModuleSeq)[ModuleSeq] << T(Error)) >>
        [](Match& _) { return _(Error); },
    };
  }
}