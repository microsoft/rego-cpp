#include "passes.h"
#include "errors.h"
#include "utils.h"


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
    else if (dst->type() == DataModule && src->type() == DataModule)
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
          if (dst_submodule->type() != DataModule)
          {
            return err(
              dst_submodule, "Cannot merge into a rule with the same name");
          }
          if (src_submodule->type() != DataModule)
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

          // now that the module is built, update all the names going
          // back down the tree.
          std::ostringstream path;
          path << "data." << _(Var)->location().view();
          std::string base = path.str();

          if(module->size() == 0){
            return Lift << Rego << (DataItem << (Key ^ base) << module);
          }

          Node submodule = module->front();
          while (submodule != nullptr)
          {
            if(submodule->type() != Submodule){
              break;
            }
            auto key = submodule / Key;
            auto key_str = key->location().view();
            if (!key_str.starts_with("["))
            {
              path << ".";
            }
            path << key_str;
            submodule->replace(key, Key ^ path.str());
            if (submodule->size() > 0)
            {
              submodule = submodule->back();
              if(submodule->size() > 0){
                submodule = submodule->front();
              }else{
                break;
              }
            }else{
              break;
            }
          }

          return Lift << Rego << (DataItem << (Key ^ base) << module);
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

      In(DataModule) * T(Import)[Import] >> [](Match& _) { return err(_(Import), "Invalid import"); }
    };
  }
}