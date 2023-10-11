#include "internal.hh"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  int add_key(Node node)
  {
    if (node->back()->type() != Key)
    {
      node->push_back(Key ^ node->fresh({"rule"}));
      return 1;
    }

    return 0;
  }

  Node convert_data(Node data)
  {
    Node node = data;
    if (node->type() == DataTerm)
    {
      node = node->front();
    }

    if (node->type() == Scalar)
    {
      return node;
    }

    if (node->type() == DataArray)
    {
      Node array = Array ^ node;
      for (auto& child : *node)
      {
        array << (Term << convert_data(child));
      }
      return array;
    }

    if (node->type() == DataSet)
    {
      Node set = Set ^ node;
      for (auto& child : *node)
      {
        set << (Term << convert_data(child));
      }
      return set;
    }

    if (node->type() == DataObject)
    {
      Node object = Object ^ node;
      for (auto& child : *node)
      {
        object << convert_data(child);
      }
      return object;
    }

    if (node->type() == DataObjectItem)
    {
      Node item = ObjectItem ^ node;
      item << (Term << convert_data(node / Key));
      item << (Term << convert_data(node / Val));
      return item;
    }

    return err(data, "Invalid data");
  }

  Node remove_expr(Node node)
  {
    if (node->type() == Expr)
    {
      return remove_expr(node->front());
    }

    if (node->type() == NumTerm)
    {
      return Term << (Scalar << node->front());
    }

    for (std::size_t i = 0; i < node->size(); i++)
    {
      node->replace_at(i, remove_expr(node->at(i)));
    }

    return node;
  }

  Node unwrap_node(Node arg);

  Node unwrap_term(Node term)
  {
    Node node = term;
    if (node->type() == Term)
    {
      node = node->front();
    }

    if (node->type() == Scalar)
    {
      return node;
    }

    // analyze whether these are constant. Only need the functions if there are
    // expressions to analyze.
    if (node->type() == Array)
    {
      if (is_constant(node))
      {
        return remove_expr(node);
      }

      Node argseq = NodeDef::create(ArgSeq);
      for (auto& child : *node)
      {
        argseq << unwrap_node(child);
      }
      return Function << (JSONString ^ "array") << argseq;
    }

    if (node->type() == Set)
    {
      if (is_constant(node))
      {
        return remove_expr(node);
      }

      Node argseq = NodeDef::create(ArgSeq);
      for (auto& child : *node)
      {
        argseq << unwrap_node(child);
      }
      return Function << (JSONString ^ "set") << argseq;
    }

    if (node->type() == Object)
    {
      if (is_constant(node))
      {
        return remove_expr(node);
      }

      Node argseq = NodeDef::create(ArgSeq);
      for (auto& child : *node)
      {
        argseq << unwrap_node(child / Key) << unwrap_node(child / Val);
      }
      return Function << (JSONString ^ "object") << argseq;
    }

    return err(term, "Invalid term");
  }

  Node unwrap_node(Node node)
  {
    Node result = node;
    if (result->type() == DataTerm)
    {
      return convert_data(result);
    }

    if (result->type() == Expr)
    {
      result = result->front();
    }

    if (result->type().in({Term, Scalar, Object, Array, Set}))
    {
      return unwrap_term(result);
    }

    if (result->type() == NumTerm)
    {
      return Scalar << result->front();
    }

    if (result->type() == RefTerm)
    {
      if (result->front()->type() == Var)
      {
        return result->front();
      }
    }

    if (result->type() == Key)
    {
      return Scalar << (JSONString ^ node);
    }

    return result;
  }

  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack);
}
namespace rego
{

  // Converts all UnifyExpr statements to be of either <var> = <var>,
  // <var> = <term>, or <var> = <function> forms, where <function> is a named
  // function that takes either <var> or <term> arguments.
  PassDef functions()
  {
    PassDef functions{
      "functions",
      wf_pass_functions,
      dir::topdown,
      {
        In(Input) * T(DataTerm)[DataTerm] >>
          [](Match& _) {
            ACTION();
            return Term << convert_data(_(DataTerm));
          },

        In(UnifyExpr) * T(Expr)[Expr] >>
          [](Match& _) {
            ACTION();
            return unwrap_node(_(Expr));
          },

        In(UnifyExpr, ArgSeq) * (T(Enumerate) << T(Expr)[Expr]) >>
          [](Match& _) {
            ACTION();
            return Function << (JSONString ^ "enumerate")
                            << (ArgSeq << unwrap_node(_(Expr)));
          },

        In(UnifyExpr, ArgSeq) *
            (T(Membership)
             << (T(Expr)[Idx] * T(Expr)[Item] * T(Expr)[ItemSeq])) >>
          [](Match& _) {
            ACTION();
            return Function
              << (JSONString ^ "membership-tuple")
              << (ArgSeq << unwrap_node(_(Idx)) << unwrap_node(_(Item))
                         << unwrap_node(_(ItemSeq)));
          },

        In(UnifyExpr, ArgSeq) *
            (T(Membership)
             << (T(Undefined) * T(Expr)[Item] * T(Expr)[ItemSeq])) >>
          [](Match& _) {
            ACTION();
            return Function
              << (JSONString ^ "membership-single")
              << (ArgSeq << unwrap_node(_(Item)) << unwrap_node(_(ItemSeq)));
          },

        In(UnifyExpr, ArgSeq) *
            (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] >>
          [](Match& _) {
            ACTION();
            std::string name = _(Compr)->type().str();
            std::transform(
              name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
              });
            Location temp = _.fresh({name});
            Node argseq = NodeDef::create(ArgSeq);
            for (auto& child : *_(Compr))
            {
              argseq << unwrap_node(child);
            }
            return Function << (JSONString ^ name) << argseq;
          },

        In(UnifyExpr, ArgSeq) *
            (T(Term)
             << (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr)))[Compr] >>
          [](Match& _) {
            ACTION();
            std::string name = _(Compr)->type().str();
            std::transform(
              name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
              });
            Location temp = _.fresh({name});
            Node argseq = NodeDef::create(ArgSeq);
            for (auto& child : *_(Compr))
            {
              argseq << unwrap_node(child);
            }
            return Function << (JSONString ^ name) << argseq;
            ;
          },

        In(UnifyExpr, ArgSeq) * (T(Merge) << T(Var)[Var]) >>
          [](Match& _) {
            ACTION();
            return Function << (JSONString ^ "merge") << (ArgSeq << _(Var));
          },

        In(ArgSeq) * T(Function)[Function] * In(UnifyBody)++ >>
          [](Match& _) {
            ACTION();
            Node seq = NodeDef::create(Seq);
            Location temp = _.fresh({"func"});
            seq->push_back(
              Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
            seq->push_back(
              Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << _(Function)));
            seq->push_back(Var ^ temp);

            return seq;
          },

        In(UnifyExpr, ArgSeq) * (T(Not) << T(Expr)[Expr]) >>
          [](Match& _) {
            ACTION();
            return Function << (JSONString ^ "not")
                            << (ArgSeq << unwrap_node(_(Expr)));
          },

        In(UnifyExpr, ArgSeq) * (T(UnaryExpr) << T(ArithArg)[ArithArg]) >>
          [](Match& _) {
            ACTION();
            return Function << (JSONString ^ "unary")
                            << (ArgSeq << unwrap_node(_(ArithArg)->front()));
          },

        In(UnifyExpr, ArgSeq) *
            (T(ArithInfix)
             << (T(ArithArg)[Lhs] * Any[Op] * T(ArithArg)[Rhs])) >>
          [](Match& _) {
            ACTION();
            return Function << (JSONString ^ "arithinfix")
                            << (ArgSeq << _(Op) << unwrap_node(_(Lhs)->front())
                                       << unwrap_node(_(Rhs)->front()));
          },

        In(UnifyExpr, ArgSeq) *
            (T(BinInfix) << (T(BinArg)[Lhs] * Any[Op] * T(BinArg)[Rhs])) >>
          [](Match& _) {
            ACTION();
            return Function << (JSONString ^ "bininfix")
                            << (ArgSeq << _(Op) << unwrap_node(_(Lhs)->front())
                                       << unwrap_node(_(Rhs)->front()));
          },

        In(UnifyExpr, ArgSeq) *
            (T(BoolInfix) << (T(BoolArg)[Lhs] * Any[Op] * T(BoolArg)[Rhs])) >>
          [](Match& _) {
            ACTION();
            return Function << (JSONString ^ "boolinfix")
                            << (ArgSeq << _(Op) << unwrap_node(_(Lhs)->front())
                                       << unwrap_node(_(Rhs)->front()));
          },

        In(UnifyExpr, ArgSeq) *
            (T(RefTerm)
             << (T(SimpleRef) << (T(Var)[Var] * (T(RefArgDot)[RefArgDot])))) >>
          [](Match& _) {
            ACTION();
            auto defs = _(Var)->lookup();
            if (!defs.empty() && defs.front()->type().in({Submodule, Data}))
            {
              // At this point all possible documents are fully qualified and in
              // the symbol table. As such, a reference such as this, which
              // points to a top-level module or rule, is a dead link and can be
              // replaced.
              Location dead = _.fresh({"dead"});
              return Var ^ dead;
            }

            Location field_name = _(RefArgDot)->front()->location();
            Node arg = Scalar << (JSONString ^ field_name);
            return Function << (JSONString ^ "apply_access")
                            << (ArgSeq << _(Var) << arg);
          },

        In(UnifyExpr, ArgSeq) *
            (T(RefTerm)
             << (T(SimpleRef)
                 << (T(Var)[Var] * (T(RefArgBrack)[RefArgBrack])))) >>
          [](Match& _) {
            ACTION();
            Node seq = NodeDef::create(Seq);
            Node arg = _(RefArgBrack)->front();
            if (arg->type() == RefTerm || arg->type() == Expr)
            {
              return Function << (JSONString ^ "apply_access")
                              << (ArgSeq << _(Var) << unwrap_node(arg));
            }
            else
            {
              auto defs = _(Var)->lookup();
              if (!defs.empty() && defs.front()->type().in({Submodule, Data}))
              {
                // At this point all possible documents are fully qualified and
                // in the symbol table. As such, a reference such as this, which
                // points to a top-level module or rule, is a dead link and can
                // be replaced.
                Location dead = _.fresh({"dead"});
                return Var ^ dead;
              }

              return Function << (JSONString ^ "apply_access")
                              << (ArgSeq << _(Var) << unwrap_node(arg));
            }
          },

        In(UnifyExpr, ArgSeq) *
            (T(ExprCall) << (T(Var)[Var] * T(ArgSeq)[ArgSeq])) >>
          [](Match& _) {
            ACTION();
            Node argseq = ArgSeq << _(Var);
            for (auto& child : *_(ArgSeq))
            {
              argseq << unwrap_node(child);
            }
            return Function << (JSONString ^ "call") << argseq;
          },

        In(RuleComp, RuleFunc, RuleObj, RuleSet, DataItem) *
            T(DataTerm)[DataTerm] >>
          [](Match& _) {
            ACTION();
            return Term << convert_data(_(DataTerm));
          },

        // errors

        In(ObjectItem) * T(Expr)[Expr] >>
          [](Match& _) {
            ACTION();
            return err(_(Expr), "Invalid expression in object");
          },

        In(Expr) * Any[Expr] >>
          [](Match& _) {
            ACTION();
            return err(_(Expr), "Invalid expression");
          },

        In(UnifyExpr, ArgSeq) * (T(RefTerm) << T(Ref)[Ref]) >>
          [](Match& _) {
            ACTION();
            return err(_(Ref), "Invalid reference");
          },

        In(Array) * T(Expr)[Expr] >>
          [](Match& _) {
            ACTION();
            return err(_(Expr), "Invalid expression in array");
          },

        In(Set) * T(Expr)[Expr] >>
          [](Match& _) {
            ACTION();
            return err(_(Expr), "Invalid expression in set");
          },

        In(ArgSeq) * T(Ref)[Ref] >>
          [](Match& _) {
            ACTION();
            return err(_(Ref), "Invalid reference");
          },

        In(ObjectItem) * T(DataModule)[DataModule] >>
          [](Match& _) {
            ACTION();
            return err(
              _(DataModule),
              "Syntax error: module not allowed as object item value");
          },
      }};

    functions.post(RuleComp, add_key);
    functions.post(RuleFunc, add_key);
    functions.post(RuleSet, add_key);
    functions.post(RuleObj, add_key);

    return functions;
  }
}