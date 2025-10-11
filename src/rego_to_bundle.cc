#include "internal.hh"
#include "rego.hh"
#include "trieste/json.h"
#include "trieste/logging.h"
#include "trieste/rewrite.h"
#include "trieste/token.h"

#include <initializer_list>
#include <sstream>
#include <string>

namespace
{
  using namespace rego;

  inline const auto DataDocument =
    TokenDef("rego-datadocument", flag::lookup | flag::symtab);
  inline const auto RuleHeadQuery = TokenDef("rego-ruleheadquery");
  inline const auto LhsBlock = TokenDef("rego-lhsblock");
  inline const auto RhsBlock = TokenDef("rego-rhsblock");
  inline const auto Number = TokenDef("rego-number", flag::print);

  // clang-format off
  inline const auto wf_bundle_refheads =
    wf_bundle_input
    | (RuleHead <<= RuleRef * (RuleHeadType >>= RuleHeadSet | RuleHeadSetDynamic | RuleHeadObj | RuleHeadObjDynamic | RuleHeadFunc | RuleHeadComp))
    | (RuleHeadObjDynamic <<= ExprSeq * Expr)
    | (RuleHeadSetDynamic <<= ExprSeq * Expr)
    | (Scalar <<= JSONString | Int | Float | True | False | Null)
    | (Membership <<= (Key >>= Expr | Undefined) * (Val >>= Expr) * Expr)
    ;
  // clang-format on

  bool is_temporary(Node node)
  {
    return node->location().view().find('$') != std::string::npos;
  }

  bool refarg_is_varref(const Node& refarg)
  {
    if (refarg != RefArgBrack)
    {
      return false;
    }

    const Node& expr = refarg->front();
    if (expr != Expr)
    {
      return false;
    }

    const Node& term = expr->front();
    if (term != Term)
    {
      return false;
    }

    return term->front() == Var || term->front() == Ref;
  }

  Node term_to_expr(const Node& node)
  {
    return Expr << (Term << node);
  }

  Node refarg_to_object_key(const Node& node)
  {
    if (node == RefArgDot)
    {
      std::string key_str = add_quotes(node->front()->location().view());
      return term_to_expr(Scalar << (JSONString ^ key_str));
    }
    else if (node->front() == Expr)
    {
      return node->front();
    }
    else if (node->front() == Placeholder)
    {
      return Expr << (Term << (Var ^ node->fresh(node->location())));
    }
    else
    {
      return err(node, "Cannot convert refarg to object key");
    }
  }

  bool refargs_contain_varref(const Node& node)
  {
    Node refargseq = node;
    if (node == Ref)
    {
      refargseq = node / RefArgSeq;
    }

    bool exists =
      std::find_if(refargseq->begin(), refargseq->end(), refarg_is_varref) !=
      refargseq->end();
    return exists;
  }

  PassDef refheads()
  {
    PassDef pass = {
      "refheads",
      wf_bundle_refheads,
      dir::bottomup | dir::once,
      {
        In(Scalar) * (T(String) << T(JSONString)[JSONString]) >>
          [](Match& _) { return JSONString ^ _(JSONString); },

        In(Scalar) * (T(String) << T(RawString)[RawString]) >>
          [](Match& _) {
            std::string raw_string(_(RawString)->location().view());
            raw_string = raw_string.substr(1, raw_string.size() - 2);
            std::string json_string = json::escape(raw_string);
            return JSONString ^ json_string;
          },

        In(Membership) * (T(ExprSeq) << (T(Expr)[Val] * End)) >>
          [](Match& _) { return Seq << Undefined << _(Val); },

        In(Membership) * (T(ExprSeq) << (T(Expr)[Key] * T(Expr)[Val] * End)) >>
          [](Match& _) { return Seq << _(Key) << _(Val); },

        In(Membership) * T(ExprSeq)[ExprSeq] >>
          [](Match& _) {
            return err(_(ExprSeq), "Too many membership arguments");
          },

        In(RuleHead) *
            ((T(RuleRef) << T(Ref)[Ref]) *
             (T(RuleHeadComp) << T(Expr)[Expr])) >>
          [](Match& _) -> Node {
          ACTION();

          if (!refargs_contain_varref(_(Ref)))
          {
            return NoChange;
          }

          Node refargseq = _(Ref) / RefArgSeq;

          auto start = std::find_if(
            refargseq->begin(), refargseq->end(), refarg_is_varref);

          Node key = refarg_to_object_key(refargseq->back());
          Node val = _(Expr);

          Node exprseq = NodeDef::create(ExprSeq);
          for (auto it = start; it != refargseq->end(); ++it)
          {
            Node arg = *it;
            if (arg == RefArgDot)
            {
              exprseq
                << (Expr << (Term << (Scalar << (JSONString ^ arg->front()))));
            }
            else if (arg == RefArgBrack)
            {
              if (arg->front() == Placeholder)
              {
                exprseq << err(arg->front(), "Invalid rule ref argument");
              }
              else
              {
                exprseq << arg->front();
              }
            }
            else
            {
              exprseq << err(arg, "Invalid refarg");
            }
          }

          // everything before the varref is a valid rule ref
          refargseq->erase(start, refargseq->end());
          Node ref = _(Ref);
          if (refargseq->empty())
          {
            ref = (_(Ref) / RefHead)->front();
            if (ref != Var)
            {
              ref = err(ref, "Invalid rule head");
            }
          }

          return Seq << (RuleRef << ref)
                     << (RuleHeadObjDynamic << exprseq << val);
        },

        In(RuleHead) *
            ((T(RuleRef) << T(Ref)[Ref]) *
             (T(RuleHeadObj) << (T(Expr)[Key] * T(Expr)[Val]))) >>
          [](Match& _) -> Node {
          ACTION();

          if (!refargs_contain_varref(_(Ref)))
          {
            return NoChange;
          }

          Node refargseq = _(Ref) / RefArgSeq;

          auto start = std::find_if(
            refargseq->begin(), refargseq->end(), refarg_is_varref);

          Node val = _(Val);

          Node exprseq = NodeDef::create(ExprSeq);
          for (auto it = start; it != refargseq->end(); ++it)
          {
            Node arg = *it;
            if (arg == RefArgDot)
            {
              exprseq
                << (Expr << (Term << (Scalar << (JSONString ^ arg->front()))));
            }
            else if (arg == RefArgBrack)
            {
              if (arg->front() == Placeholder)
              {
                exprseq << err(arg->front(), "Invalid rule ref argument");
              }
              else
              {
                exprseq << arg->front();
              }
            }
            else
            {
              exprseq << err(arg, "Invalid refarg");
            }
          }

          exprseq << _(Key);

          // everything before the varref is a valid rule ref
          refargseq->erase(start, refargseq->end());
          Node ref = _(Ref);
          if (refargseq->empty())
          {
            ref = (_(Ref) / RefHead)->front();
            if (ref != Var)
            {
              ref = err(ref, "Invalid rule head");
            }
          }

          return Seq << (RuleRef << ref)
                     << (RuleHeadObjDynamic << exprseq << val);
        },

        In(RuleHead) *
            ((T(RuleRef) << T(Ref)[Ref]) * (T(RuleHeadSet) << T(Expr)[Expr])) >>
          [](Match& _) -> Node {
          ACTION();

          if (!refargs_contain_varref(_(Ref)))
          {
            return NoChange;
          }

          Node refargseq = _(Ref) / RefArgSeq;

          auto start = std::find_if(
            refargseq->begin(), refargseq->end(), refarg_is_varref);

          Node exprseq = NodeDef::create(ExprSeq);
          for (auto it = start; it != refargseq->end(); ++it)
          {
            Node arg = *it;
            if (arg == RefArgDot)
            {
              exprseq
                << (Expr << (Term << (Scalar << (JSONString ^ arg->front()))));
            }
            else if (arg == RefArgBrack)
            {
              if (arg->front() == Placeholder)
              {
                exprseq << err(arg->front(), "Invalid rule ref argument");
              }
              else
              {
                exprseq << arg->front();
              }
            }
            else
            {
              exprseq << err(arg, "Invalid refarg");
            }
          }

          // everything before the varref is a valid rule ref
          refargseq->erase(start, refargseq->end());
          Node ref = _(Ref);
          if (refargseq->empty())
          {
            ref = (_(Ref) / RefHead)->front();
            if (ref != Var)
            {
              ref = err(ref, "Invalid rule head");
            }
          }

          return Seq << (RuleRef << ref)
                     << (RuleHeadSetDynamic << exprseq << _(Expr));
        },
      }};

    return pass;
  }

  // clang-format off
  inline const auto wf_bundle_rules =
    wf_bundle_refheads
    | (RegoBundle <<= EntryPointSeq * BaseDocument * ModuleSeq * IRQuery)
    | (BaseDocument <<= Ident * BaseObject)[Ident]
    | (BaseObject <<= BaseObjectItem++)
    | (BaseObjectItem <<= Ident * DataTerm)[Ident]
    | (DataTerm <<= BaseObject | DataArray | DataSet | Scalar)
    | (IRQuery <<= (IRString|Undefined))
    | (ModuleSeq <<= Module++)
    | (Module <<= Ident * Package * Version * ImportSeq * Policy)[Ident]
    | (Import <<= Ref * Var)[Var]
    | (Policy <<= Rule++)
    | (RuleHead <<= RuleHeadSet | RuleHeadSetDynamic | RuleHeadObj | RuleHeadObjDynamic | RuleHeadFunc | RuleHeadComp | RuleHeadQuery)
    | (RuleHeadQuery <<= Query)
    | (RuleHeadComp <<= Term)
    | (RuleHeadObj <<= Ident * (Key >>= Term) * (Val >>= Term))
    | (RuleHeadObjDynamic <<= TermSeq * Term)
    | (RuleHeadFunc <<= RuleArgs * Term)
    | (RuleHeadSet <<= Term)
    | (RuleHeadSetDynamic <<= TermSeq * Term)
    | (RuleArgs <<= Var++)
    | (Else <<= Term * Query)
    | (Rule <<= Ident * Ref * LocalSeq * (Default >>= True | False) * RuleHead * RuleBodySeq)[Ident]
    | (InfixOperator <<= BoolOperator | ArithOperator | BinOperator)
    | (ExprScan <<= Expr * (Key >>= Local) * (Val >>= Local))
    | (ExprAssign <<= (Lhs >>= AssignVar) * (Rhs >>= Expr))[Lhs]
    | (ExprEvery <<= LocalSeq * Var * (Key >>= Placeholder | Var) * (Val >>= Var) * Query)
    | (ArrayCompr <<= LocalSeq * (Val >>= Expr) * Query)
    | (ObjectCompr <<= LocalSeq * (Key >>= Expr) * (Val >>= Expr) * Query)
    | (SetCompr <<= LocalSeq * (Val >>= Expr) * Query)
    | (Alias <<= Term * Expr)
    | (ExprUnify <<= (Lhs >>= Expr) * (Rhs >>= FuncArgVar | Expr))
    | (Literal <<= (Expr >>= ExprAssign | ExprUnify | ExprScan | ExprEvery | Local | Expr | NotExpr) * WithSeq)
    | (Expr <<= (Term | ExprCall | ExprInfix | UnaryExpr | ExprParens))
    | (LocalSeq <<= (Local | EveryLocal)++)
    | (Local <<= Ident)[Ident]
    | (EveryLocal <<= Ident)[Ident]
    | (TermSeq <<= Term++)
    ;
  // clang-format on

  std::optional<std::string> ref_to_string(Node ref)
  {
    if (ref == Var)
    {
      return std::string(ref->location().view());
    }

    Node head = ref / RefHead;
    if (head->front() != Var)
    {
      return std::nullopt;
    }

    std::ostringstream buf;
    buf << head->front()->location().view();
    for (Node arg : *(ref / RefArgSeq))
    {
      std::string part;
      if (arg == RefArgDot)
      {
        part = std::string(arg->front()->location().view());
      }
      else
      {
        Node expr = arg->front();
        if (expr != Expr)
        {
          return std::nullopt;
        }

        Node term = expr->front();
        auto maybe_name = unwrap(term, JSONString);
        if (maybe_name.success)
        {
          part = strip_quotes(maybe_name.node->location().view());
        }
        else
        {
          return std::nullopt;
        }
      }

      if (part.find('.') == std::string::npos)
      {
        buf << "." << part;
      }
      else
      {
        buf << "[" << part << "]";
      }
    }

    return buf.str();
  }

  void find_vars(std::set<Location>& names, Node node)
  {
    Nodes frontier({node});
    while (!frontier.empty())
    {
      Node current = frontier.back();
      frontier.pop_back();

      if (current == Var)
      {
        names.insert(current->location());
        continue;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }
  }

  bool is_func_arg(Node var)
  {
    Node rule = var->parent(Rule);
    if (rule == nullptr)
    {
      return false;
    }

    if (rule / RuleHead / RuleHeadType != RuleHeadFunc)
    {
      return false;
    }

    std::set<Location> possible_vars;
    find_vars(possible_vars, rule / RuleHead / RuleHeadType);
    return possible_vars.find(var->location()) != possible_vars.end();
  }

  /**
   *  This pass adds rules and modules to the symbol table, so that they
   *  can be used for telling the difference between rule references and
   *  undeclared unification variables.
   */
  PassDef rules()
  {
    auto func_argnames =
      std::make_shared<std::map<std::string, std::vector<Location>>>();
    PassDef pass = {
      "rules",
      wf_bundle_rules,
      dir::bottomup | dir::once,
      {
        T(Expr)[Expr]
            << (T(ExprInfix)
                << (T(Expr) *
                    (T(InfixOperator) << (T(AssignOperator) << T(Assign))) *
                    T(Expr))) >>
          [](Match& _) -> Node {
          if (_(Expr)->parent() == Literal)
          {
            return NoChange;
          }

          return err(_(Expr), "Invalid assignment");
        },

        T(Expr)[Expr]
            << (T(ExprInfix)
                << (T(Expr)[Lhs] *
                    (T(InfixOperator) << (T(AssignOperator) << T(Unify))) *
                    T(Expr)[Rhs])) >>
          [](Match& _) -> Node {
          if (_(Expr)->parent() == Literal)
          {
            return NoChange;
          }

          return Expr
            << (ExprInfix << _(Lhs)
                          << (InfixOperator << (BoolOperator << Equals))
                          << _(Rhs));
        },

        In(Literal) *
            (T(Expr)
             << (T(ExprInfix)
                 << ((T(Expr) << (T(Term) << T(Var)[Lhs])) *
                     (T(InfixOperator) << (T(AssignOperator) << T(Assign))) *
                     (T(Expr)[Rhs])))) >>
          [](Match& _) -> Node {
          return ExprAssign << (AssignVar ^ _(Lhs)) << _(Rhs);
        },

        In(Literal) *
            (T(Expr)
             << (T(ExprInfix)
                 << ((T(Expr)[Lhs] << (T(Term) << T(Var)[Var])) *
                     (T(InfixOperator) << (T(AssignOperator) << T(Unify))) *
                     T(Expr)[Rhs]))) >>
          [](Match& _) -> Node {
          if (!is_constant(_(Rhs)) || is_func_arg(_(Var)))
          {
            return ExprUnify << _(Lhs) << _(Rhs);
          }

          return ExprAssign << (AssignVar ^ _(Var)) << _(Rhs);
        },

        In(Literal) *
            (T(Expr)
             << (T(ExprInfix)
                 << (T(Expr)[Lhs] *
                     (T(InfixOperator) << (T(AssignOperator) << T(Unify))) *
                     (T(Expr)[Rhs] << (T(Term) << T(Var)[Var]))))) >>
          [](Match& _) -> Node {
          if (!is_constant(_(Lhs)) || is_func_arg(_(Var)))
          {
            return ExprUnify << _(Lhs) << _(Rhs);
          }

          return ExprAssign << (AssignVar ^ _(Var)) << _(Lhs);
        },

        In(Literal) *
            (T(Expr)
             << (T(ExprInfix)
                 << (T(Expr)[Lhs] *
                     (T(InfixOperator) << (T(AssignOperator) << T(Unify))) *
                     T(Expr)[Rhs]))) >>
          [](Match& _) -> Node {
          if (is_constant(_(Lhs)) && is_constant(_(Rhs)))
          {
            return Expr
              << (ExprInfix << _(Lhs)
                            << (InfixOperator << (BoolOperator << Equals))
                            << _(Rhs));
          }

          return ExprUnify << _(Lhs) << _(Rhs);
        },

        In(Literal) *
            (T(Expr)
             << (T(ExprInfix)
                 << (T(Expr)[Lhs] *
                     (T(InfixOperator) << (T(AssignOperator) << T(Assign))) *
                     (T(Expr)[Rhs])))) >>
          [](Match& _) -> Node { return ExprUnify << _(Lhs) << _(Rhs); },

        In(Query) *
            (T(Literal)[Literal]
             << ((T(SomeDecl)
                  << ((T(ExprSeq)
                       << ((T(Expr) << (T(Term) << T(Var)[Val])) * End)) *
                      T(Expr)[Expr])) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) {
            Location scanindex_name = _.fresh({"scanindex"});
            Location scanvalue_name = _.fresh({"scanvalue"});
            return Seq << (Literal
                           << (ExprScan << _(Expr)
                                        << (Local << (Ident ^ scanindex_name))
                                        << (Local << (Ident ^ scanvalue_name)))
                           << _(WithSeq))
                       << ((Literal ^ _(Literal))
                           << (ExprAssign
                               << (AssignVar ^ _(Val))
                               << (Expr << (Term << (Var ^ scanvalue_name))))
                           << WithSeq);
          },

        In(Query) *
            (T(Literal)[Literal]
             << ((T(SomeDecl)
                  << ((T(ExprSeq) << (T(Expr)[Val] * End)) * T(Expr)[Expr])) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) {
            Location scanindex_name = _.fresh({"scanindex"});
            Location scanvalue_name = _.fresh({"scanvalue"});
            return Seq << (Literal
                           << (ExprScan << _(Expr)
                                        << (Local << (Ident ^ scanindex_name))
                                        << (Local << (Ident ^ scanvalue_name)))
                           << _(WithSeq))
                       << ((Literal ^ _(Literal))
                           << (ExprUnify
                               << _(Val)
                               << (Expr << (Term << (Var ^ scanvalue_name))))
                           << WithSeq);
          },

        In(Query) *
            (T(Literal)[Literal]
             << ((T(SomeDecl)
                  << ((T(ExprSeq)
                       << ((T(Expr) << (T(Term) << T(Var)[Key])) *
                           (T(Expr) << (T(Term) << T(Var)[Val])) * End)) *
                      T(Expr)[Expr])) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) -> Node {
          Location scanindex_name = _.fresh({"scanindex"});
          Location scanvalue_name = _.fresh({"scanvalue"});
          return Seq
            << (Literal << (ExprScan << _(Expr)
                                     << (Local << (Ident ^ scanindex_name))
                                     << (Local << (Ident ^ scanvalue_name)))
                        << _(WithSeq))
            << (Literal << (ExprAssign
                            << (AssignVar ^ _(Key))
                            << (Expr << (Term << (Var ^ scanindex_name))))
                        << WithSeq)
            << ((Literal ^ _(Literal))
                << (ExprAssign << (AssignVar ^ _(Val))
                               << (Expr << (Term << (Var ^ scanvalue_name))))
                << WithSeq);
        },

        In(Query) *
            (T(Literal)[Literal]
             << ((T(SomeDecl)
                  << ((T(ExprSeq) << (T(Expr)[Key] * T(Expr)[Val] * End)) *
                      T(Expr)[Expr])) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) -> Node {
          Location scanindex_name = _.fresh({"scanindex"});
          Location scanvalue_name = _.fresh({"scanvalue"});
          return Seq
            << (Literal << (ExprScan << _(Expr)
                                     << (Local << (Ident ^ scanindex_name))
                                     << (Local << (Ident ^ scanvalue_name)))
                        << _(WithSeq))
            << (Literal << (ExprUnify
                            << _(Key)
                            << (Expr << (Term << (Var ^ scanindex_name))))
                        << WithSeq)
            << ((Literal ^ _(Literal))
                << (ExprUnify << _(Val)
                              << (Expr << (Term << (Var ^ scanvalue_name))))
                << WithSeq);
        },

        In(Query) *
            (T(Literal)
             << (T(SomeDecl)
                 << ((T(ExprSeq)[ExprSeq]
                      << ((T(Expr) << (T(Term) << T(Var)))++ * End)) *
                     T(Undefined)))) >>
          [](Match& _) -> Node { return nullptr; },

        In(Query) * (T(Literal) << T(SomeDecl)[SomeDecl]) >>
          [](Match& _) { return err(_(SomeDecl), "Invalid some statement"); },

        In(Query) *
            (T(Literal)[Literal]
             << ((T(Expr)
                  << (T(ExprEvery)
                      << (T(VarSeq)[VarSeq] *
                          T(Term, ExprCall, ExprInfix)[Expr] *
                          T(Query)[Query]))) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) {
            Location every_name = _.fresh({"every"});
            Node value = _(VarSeq)->back();
            Node key;
            if (_(VarSeq)->size() > 1)
            {
              key = _(VarSeq)->front();
            }
            else
            {
              key = Placeholder ^ "_";
            }

            Node localseq = NodeDef::create(LocalSeq);
            for (Node var : *_(VarSeq))
            {
              localseq << (EveryLocal << (Ident ^ var));
            }

            return Seq << (Literal << (ExprAssign << (AssignVar ^ every_name)
                                                  << (Expr << _(Expr)))
                                   << _(WithSeq)->clone())
                       << ((Literal ^ _(Literal))
                           << (ExprEvery << localseq << (Var ^ every_name)
                                         << key << value << _(Query))
                           << _(WithSeq));
          },

        In(ArrayCompr, SetCompr) * (T(Expr)[Val] * T(Query)[Query]) >>
          [](Match& _) { return Seq << LocalSeq << _(Val) << _(Query); },

        In(ObjectCompr) * (T(Expr)[Key] * T(Expr)[Val] * T(Query)[Query]) >>
          [](Match& _) {
            return Seq << LocalSeq << _(Key) << _(Val) << _(Query);
          },

        In(Policy) *
            (T(Rule)
             << (T(True, False)[Default] *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Ref, Var)[Ident]) *
                      (T(RuleHeadComp)[RuleHead] << T(Expr)[Expr]))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            Node ref = _(Ident);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            auto maybe_string = ref_to_string(ref);
            std::string ident;
            if (maybe_string.has_value())
            {
              ident = maybe_string.value();
            }
            else
            {
              return err(ref, "Cannot form valid identifier from reference");
            }

            Node rulehead;
            Node value_literal;
            if (is_constant(_(Expr)))
            {
              rulehead = RuleHeadComp << _(Expr)->front();
            }
            else
            {
              Location compvalue_name = _.fresh({"compvalue"});
              value_literal =
                (Literal << (ExprAssign << (AssignVar ^ compvalue_name)
                                        << _(Expr))
                         << WithSeq);
              rulehead = RuleHeadComp << (Term << (Var ^ compvalue_name));
            }

            Node rulebodyseq = _(RuleBodySeq);
            if (rulebodyseq->empty())
            {
              rulebodyseq << (Query << value_literal);
            }
            else
            {
              for (Node body : *rulebodyseq)
              {
                Node query = body;
                if (body == Else)
                {
                  if (is_constant(body / Expr))
                  {
                    value_literal = nullptr;
                    body->replace_at(0, (body / Expr)->front());
                  }
                  else
                  {
                    Location elsevalue_name = _.fresh({"elsevalue"});
                    value_literal =
                      (Literal << (ExprAssign << (AssignVar ^ elsevalue_name)
                                              << body / Expr)
                               << WithSeq);
                    query = body / Query;
                    body->replace_at(0, Term << (Var ^ elsevalue_name));
                  }
                }

                if (value_literal)
                {
                  query << value_literal->clone();
                }
              }
            }

            return Rule << (Ident ^ ident) << ref << LocalSeq << _(Default)
                        << (RuleHead << rulehead) << rulebodyseq;
          },

        In(Policy) *
            (T(Rule)
             << (T(True, False)[Default] *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Ref, Var)[Ident]) *
                      (T(RuleHeadFunc)[RuleHead]))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [func_argnames](Match& _) {
            Node ref = _(Ident);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            auto maybe_string = ref_to_string(ref);
            std::string ident;
            if (maybe_string.has_value())
            {
              ident = maybe_string.value();
            }
            else
            {
              return err(ref, "Cannot form valid identifier from reference");
            }

            Node ruleargs = _(RuleHead) / RuleArgs;
            if (func_argnames->find(ident) == func_argnames->end())
            {
              func_argnames->insert({ident, {}});
              for (auto i = 0; i < ruleargs->size(); ++i)
              {
                func_argnames->at(ident).push_back(_.fresh({"funcarg"}));
              }
            }

            std::vector<Location>& argnames = func_argnames->at(ident);
            Nodes unify_literals;
            Node new_ruleargs = NodeDef::create(RuleArgs);
            Node localseq = NodeDef::create(LocalSeq);
            for (size_t i = 0; i < ruleargs->size(); ++i)
            {
              Location funcarg_name = argnames[i];
              unify_literals.push_back(
                Literal << (ExprUnify << (Expr << ruleargs->at(i))
                                      << (FuncArgVar ^ funcarg_name))
                        << WithSeq);
              new_ruleargs << (Var ^ funcarg_name);

              Nodes locals;
              Nodes frontier({ruleargs->at(i)});
              while (!frontier.empty())
              {
                Node current = frontier.back();
                frontier.pop_back();
                if (current == Var)
                {
                  locals.push_back(current);
                }

                frontier.insert(
                  frontier.end(), current->begin(), current->end());
              }

              localseq << (Local << (Ident ^ funcarg_name));
              for (Node local : locals)
              {
                localseq << (Local << (Ident ^ local));
              }
            }

            Node rulehead;
            Node value_literal;
            if (is_constant(_(RuleHead) / Expr))
            {
              rulehead = RuleHeadFunc << new_ruleargs
                                      << (_(RuleHead) / Expr)->front();
            }
            else
            {
              Location funcvalue_name = _.fresh({"funcvalue_name"});
              value_literal =
                (Literal << (ExprAssign << (AssignVar ^ funcvalue_name)
                                        << _(RuleHead) / Expr)
                         << WithSeq);

              rulehead = RuleHeadFunc << new_ruleargs
                                      << (Term << (Var ^ funcvalue_name));
            }

            Node rulebodyseq = _(RuleBodySeq);
            if (rulebodyseq->empty())
            {
              rulebodyseq << (Query << unify_literals << value_literal);
            }
            else
            {
              for (Node body : *rulebodyseq)
              {
                Node query = body;
                if (body == Else)
                {
                  if (is_constant(body / Expr))
                  {
                    value_literal = nullptr;
                    body->replace_at(0, (body / Expr)->front());
                  }
                  else
                  {
                    Location elsevalue_name = _.fresh({"elsevalue"});
                    value_literal =
                      (Literal << (ExprAssign << (AssignVar ^ elsevalue_name)
                                              << body / Expr)
                               << WithSeq);
                    body->replace_at(0, Term << (Var ^ elsevalue_name));
                  }

                  query = body / Query;
                }

                for (Node literal : unify_literals)
                {
                  query->insert(query->begin(), literal->clone());
                }

                if (value_literal)
                {
                  query << value_literal->clone();
                }
              }
            }

            return Rule << (Ident ^ ident) << ref << localseq << _(Default)
                        << (RuleHead << rulehead) << rulebodyseq;
          },

        In(Policy) *
            (T(Rule)
             << (T(True, False)[Default] *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Ref, Var)[Ident]) *
                      (T(RuleHeadObj)[RuleHead]
                       << (T(Expr)[Key] * T(Expr)[Val])))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            Node ref = _(Ident);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            auto maybe_string = ref_to_string(ref);
            std::string ident;
            if (maybe_string.has_value())
            {
              ident = maybe_string.value();
            }
            else
            {
              return err(ref, "Cannot form valid identifier from reference");
            }

            bool dynamic_key = false;
            Node key = _(Key);
            Node key_term;
            Node key_literal;
            Node key_constant = to_constant_term(_(Key));
            if (key_constant)
            {
              key_term = key->front();
            }
            else
            {
              if (_(RuleBodySeq)->size() > 1)
              {
                return err(_(RuleBodySeq), "only one body for dynamic keys");
              }

              dynamic_key = true;
              Location objkey_name = _.fresh({"objkey"});
              key_literal = Literal
                << (ExprAssign << (AssignVar ^ objkey_name) << key) << WithSeq;
              key_term = Term << (Var ^ objkey_name);
            }

            Node val = _(Val);
            Node val_term;
            Node val_literal;
            if (is_constant(val))
            {
              val_term = val->front();
            }
            else
            {
              Location objval_name = _.fresh({"objval"});
              val_literal = Literal
                << (ExprAssign << (AssignVar ^ objval_name) << val) << WithSeq;
              val_term = Term << (Var ^ objval_name);
            }

            Node rulehead;
            if (dynamic_key)
            {
              rulehead = RuleHeadObjDynamic << (TermSeq << key_term)
                                            << val_term;
            }
            else
            {
              rulehead = RuleHeadObj << (Ident ^ to_key(key_constant))
                                     << key_term << val_term;
            }

            Node rulebodyseq = _(RuleBodySeq);
            if (rulebodyseq->empty())
            {
              rulebodyseq << (Query << key_literal << val_literal);
            }
            else
            {
              for (Node body : *rulebodyseq)
              {
                Node query = body;
                if (body == Else)
                {
                  if (is_constant(body / Expr))
                  {
                    val_literal = nullptr;
                    body->replace_at(0, (body / Expr)->front());
                  }
                  else
                  {
                    Location elsevalue_name = _.fresh({"elsevalue"});
                    val_literal =
                      (Literal << (ExprAssign << (AssignVar ^ elsevalue_name)
                                              << body / Expr)
                               << WithSeq);
                    query = body / Query;
                    body->replace_at(0, Term << (Var ^ elsevalue_name));
                  }
                }

                if (key_literal)
                {
                  query << key_literal->clone();
                }

                if (val_literal)
                {
                  query << val_literal->clone();
                }
              }
            }

            return Rule << (Ident ^ ident) << ref << LocalSeq << _(Default)
                        << (RuleHead << rulehead) << rulebodyseq;
          },

        In(Policy) *
            (T(Rule)
             << (T(True, False)[Default] *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Ref, Var)[Ident]) *
                      (T(RuleHeadObjDynamic)[RuleHead]
                       << (T(ExprSeq)[ExprSeq] * T(Expr)[Expr])))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            Node ref = _(Ident);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            auto maybe_string = ref_to_string(ref);
            std::string ident;
            if (maybe_string.has_value())
            {
              ident = maybe_string.value();
            }
            else
            {
              return err(ref, "Cannot form valid identifier from reference");
            }

            Nodes term_literals;

            Node exprseq = _(ExprSeq);
            Node termseq = NodeDef::create(TermSeq);

            for (Node expr : *exprseq)
            {
              if (is_constant(expr))
              {
                termseq << expr->front();
              }
              else
              {
                Location pathpart_name = _.fresh({"pathpart"});
                Node pathpart_literal = Literal
                  << (ExprAssign << (AssignVar ^ pathpart_name) << expr)
                  << WithSeq;
                term_literals.push_back(pathpart_literal);
                termseq << (Term << (Var ^ pathpart_name));
              }
            }

            Node expr = _(Expr);
            Node term;

            if (is_constant(expr))
            {
              term = expr->front();
            }
            else
            {
              Location objvalue_name = _.fresh({"objvalue"});
              Node objvalue_literal = Literal
                << (ExprAssign << (AssignVar ^ objvalue_name) << expr)
                << WithSeq;
              term_literals.push_back(objvalue_literal);
              term = Term << (Var ^ objvalue_name);
            }

            Node rulehead = RuleHeadObjDynamic << termseq << term;

            Node rulebodyseq = _(RuleBodySeq);
            if (rulebodyseq->empty())
            {
              rulebodyseq << (Query << term_literals);
            }
            else
            {
              for (Node body : *rulebodyseq)
              {
                Node query = body;
                if (body == Else)
                {
                  return err(body, "Invalid else in obj rule");
                }

                for (Node literal : term_literals)
                {
                  query << literal->clone();
                }
              }
            }

            return Rule << (Ident ^ ident) << ref << LocalSeq << _(Default)
                        << (RuleHead << rulehead) << rulebodyseq;
          },

        In(Policy) *
            (T(Rule)
             << (T(True, False)[Default] *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Ref, Var)[Ident]) *
                      (T(RuleHeadSet) << T(Expr)[Expr]))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            Node ref = _(Ident);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            auto maybe_string = ref_to_string(ref);
            std::string ident;
            if (maybe_string.has_value())
            {
              ident = maybe_string.value();
            }
            else
            {
              return err(ref, "Cannot form valid identifier from reference");
            }

            Node rulehead;
            Node expr = _(Expr);
            Node value_literal;

            Node termseq = NodeDef::create(TermSeq);

            if (is_constant(expr))
            {
              rulehead = RuleHeadSet << expr->front();
            }
            else
            {
              Location setvalue_name = _.fresh({"setvalue"});
              value_literal = Literal
                << (ExprAssign << (AssignVar ^ setvalue_name) << expr)
                << WithSeq;
              rulehead = RuleHeadSet << (Term << (Var ^ setvalue_name));
            }

            Node rulebodyseq = _(RuleBodySeq);
            if (rulebodyseq->empty())
            {
              rulebodyseq << (Query << value_literal);
            }
            else
            {
              for (Node body : *rulebodyseq)
              {
                Node query = body;
                if (body == Else)
                {
                  return err(body, "Invalid else in set rule");
                }

                if (value_literal)
                {
                  query << value_literal->clone();
                }
              }
            }

            return Rule << (Ident ^ ident) << ref << LocalSeq << _(Default)
                        << (RuleHead << rulehead) << rulebodyseq;
          },

        In(Policy) *
            (T(Rule)
             << (T(True, False)[Default] *
                 (T(RuleHead)
                  << ((T(RuleRef) << T(Ref, Var)[Ident]) *
                      (T(RuleHeadSetDynamic)
                       << (T(ExprSeq)[ExprSeq] * T(Expr)[Expr])))) *
                 T(RuleBodySeq)[RuleBodySeq])) >>
          [](Match& _) {
            Node ref = _(Ident);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            auto maybe_string = ref_to_string(ref);
            std::string ident;
            if (maybe_string.has_value())
            {
              ident = maybe_string.value();
            }
            else
            {
              return err(ref, "Cannot form valid identifier from reference");
            }

            Nodes term_literals;

            Node exprseq = _(ExprSeq);
            Node termseq = NodeDef::create(TermSeq);

            for (Node expr : *exprseq)
            {
              if (is_constant(expr))
              {
                termseq << expr->front();
              }
              else
              {
                Location pathpart_name = _.fresh({"pathpart"});
                Node pathpart_literal = Literal
                  << (ExprAssign << (AssignVar ^ pathpart_name) << expr)
                  << WithSeq;
                term_literals.push_back(pathpart_literal);
                termseq << (Term << (Var ^ pathpart_name));
              }
            }

            Node expr = _(Expr);
            Node term;

            if (is_constant(expr))
            {
              term = expr->front();
            }
            else
            {
              Location setvalue_name = _.fresh({"setvalue"});
              Node setvalue_literal = Literal
                << (ExprAssign << (AssignVar ^ setvalue_name) << expr)
                << WithSeq;
              term_literals.push_back(setvalue_literal);
              term = Term << (Var ^ setvalue_name);
            }

            Node rulehead = RuleHeadSetDynamic << termseq << term;

            Node rulebodyseq = _(RuleBodySeq);
            if (rulebodyseq->empty())
            {
              rulebodyseq << (Query << term_literals);
            }
            else
            {
              for (Node body : *rulebodyseq)
              {
                Node query = body;
                if (body == Else)
                {
                  return err(body, "Invalid else in set rule");
                }

                for (Node literal : term_literals)
                {
                  query << literal->clone();
                }
              }
            }

            return Rule << (Ident ^ ident) << ref << LocalSeq << _(Default)
                        << (RuleHead << rulehead) << rulebodyseq;
          },

        In(Module) * (T(Package)[Package] << T(Ref)[Ref]) >>
          [](Match& _) {
            auto maybe_string = ref_to_string(_(Ref));
            std::string ident;
            if (maybe_string.has_value())
            {
              ident = maybe_string.value();
            }
            else
            {
              return err(_(Ref), "Cannot form valid identifier from reference");
            }

            return Seq << (Ident ^ ident) << _(Package);
          },

        In(ModuleSeq) * T(Module)[Module] >>
          [](Match& _) { return Module << *_[Module]; },

        In(DataObject) *
            (T(DataObjectItem)
             << ((T(DataTerm) << (T(Scalar) << T(JSONString)[Ident])) *
                 T(DataTerm)[Val])) >>
          [](Match& _) {
            return BaseObjectItem << (Ident ^ _(Ident)) << _(Val);
          },

        In(DataObject) * T(DataObjectItem)[DataObjectItem] >>
          [](Match& _) {
            return err(_(DataObjectItem), "Invalid base document");
          },

        In(DataTerm) * T(DataObject)[DataObject] >>
          [](Match& _) { return BaseObject << *_[DataObject]; },

        In(DataSeq) *
            ((T(Data) << (T(DataTerm) << T(BaseObject)[Lhs])) *
             (T(Data) << (T(DataTerm) << T(BaseObject)[Rhs]))) >>
          [](Match& _) {
            return Data << (DataTerm << (BaseObject << *_[Lhs] << *_[Rhs]));
          },

        T(RefArgBrack)
            << (T(Expr) << (T(Term) << (T(Scalar) << T(JSONString)[Var]))) >>
          [](Match& _) {
            return RefArgDot << (Var ^ strip_quotes(_(Var)->location().view()));
          },

        In(RegoBundle) *
            (T(EntryPointSeq)[EntryPointSeq] *
             (T(DataSeq) << (~T(Data)[Data] * End)) * T(ModuleSeq)[ModuleSeq] *
             T(Query)[Query]) >>
          [](Match& _) -> Node {
          Node dataobj = _(Data);
          if (dataobj == nullptr)
          {
            dataobj = NodeDef::create(BaseObject);
          }
          else
          {
            dataobj = dataobj->front()->front();
          }

          if (dataobj != BaseObject)
          {
            return err(
              dataobj, "Invalid base data document (must be a DataObject)");
          }

          Node query = IRQuery << Undefined;
          if (!_(Query)->empty())
          {
            Location querymodule_name = _.fresh({"querymodule"});
            Location queryrule_name = _.fresh({"query"});
            _(ModuleSeq)
              << (Module << (Ident ^ querymodule_name)
                         << (Package
                             << (Ref << (RefHead << (Var ^ querymodule_name))
                                     << RefArgSeq))
                         << (Version ^ DefaultVersion) << ImportSeq
                         << (Policy
                             << (Rule
                                 << (Ident ^ queryrule_name)
                                 << (Ref << (RefHead << (Var ^ queryrule_name))
                                         << RefArgSeq)
                                 << LocalSeq << False
                                 << (RuleHead << (RuleHeadQuery << _(Query)))
                                 << RuleBodySeq)));

            std::ostringstream query_entrypoint;
            query_entrypoint << querymodule_name.view() << "/"
                             << queryrule_name.view();
            _(EntryPointSeq) << (IRString ^ query_entrypoint.str());
            query = IRQuery << (IRString ^ query_entrypoint.str());
          }

          Node basedoc = BaseDocument << (Ident ^ "data") << dataobj;
          return Seq << _(EntryPointSeq) << basedoc << _(ModuleSeq) << query;
        },

        // errors

        In(RegoBundle) * (T(DataSeq) << T(Data)[Data]) >> [](Match& _) -> Node {
          if (_(Data)->front()->front() == BaseObject)
          {
            return NoChange;
          }

          return err(_(Data), "Invalid data base document");
        },

        In(RegoBundle) * (T(DataSeq) << (T(Data) * T(Data)[Data])) >>
          [](Match& _) -> Node {
          return err(_(Data), "unable to merge base documents");
        },

        In(Expr) * T(ExprEvery)[ExprEvery] >> [](Match& _) -> Node {
          if (_(ExprEvery)->parent()->parent() == Literal)
          {
            return NoChange;
          }

          return err(_(ExprEvery), "Invalid every expression");
        },
      }};

    return pass;
  }

  // clang-format off
  inline const auto wf_bundle_locals =
    wf_bundle_rules
    | (Expr <<= (Term | ExprCall | ExprInfix | UnaryExpr))
    ;
  // clang-format on

  PassDef locals()
  {
    auto rule_locals = std::make_shared<NodeMap<std::set<Location>>>();
    PassDef pass = {
      "locals",
      wf_bundle_locals,
      dir::bottomup | dir::once,
      {
        T(Expr) << (T(ExprParens) << T(Expr)[Expr]) >>
          [](Match& _) { return _(Expr); },

        In(Term) * T(Var)[Var] * In(RuleHead)++ >>
          [rule_locals](Match& _) -> Node {
          if (is_temporary(_(Var)))
          {
            return NoChange;
          }

          Nodes results = _(Var)->lookup();
          if (!results.empty())
          {
            return NoChange;
          }

          Node rule = _(Var)->parent(Rule);
          if (rule_locals->find(rule) == rule_locals->end())
          {
            rule_locals->insert({rule, {}});
          }

          rule_locals->at(rule).insert(_(Var)->location());
          return NoChange;
        },

        In(Rule) * T(LocalSeq)[LocalSeq] >>
          [rule_locals](Match& _) {
            Node rule = _(LocalSeq)->parent();
            auto it = rule_locals->find(rule);
            if (it == rule_locals->end())
            {
              return _(LocalSeq);
            }

            for (auto& name : it->second)
            {
              _(LocalSeq) << (Local << (Ident ^ name));
            }

            return _(LocalSeq);
          },
      }};

    pass.pre([rule_locals](Node) {
      rule_locals->clear();
      return 0;
    });

    return pass;
  }

  // clang-format off
  inline const auto wf_bundle_implicit_scans =
    wf_bundle_locals
    | (ExprIsArray <<= Var * Int)
    | (ExprIsObject <<= Var * Int)
    | (ExprAssignFromArray <<= AssignVar * Var * Int)
    | (ExprAssignFromObject <<= AssignVar * Var * Expr)
    | (Literal <<= (Expr >>= ExprAssignFromArray | ExprAssignFromObject | ExprIsArray | ExprIsObject | ExprAssign | ExprUnify | ExprScan | ExprEvery | Local | Expr | NotExpr) * WithSeq)
    | (Term <<= (UnifyVar | Ref | Var | Scalar | Array | Object | Set | Membership | ArrayCompr | ObjectCompr | SetCompr))
    ;
  // clang-format on

  Nodes lookup_var(Node var)
  {
    // check if something already exists in scope
    Nodes results = var->lookup();
    if (!results.empty())
    {
      return results;
    }

    // check if this name exists in a different module with the same package
    // name
    Node module = var->parent(Module);
    Node ident = module / Ident;
    Nodes modules = ident->lookup();
    if (modules.size() < 2)
    {
      return results;
    }

    for (Node other_module : modules)
    {
      if (other_module == module)
      {
        continue;
      }

      results = other_module->lookdown(var->location());
      if (!results.empty())
      {
        break;
      }
    }

    return results;
  }

  bool find_locals(Node node, bool in_query)
  {
    Nodes frontier({node});
    while (!frontier.empty())
    {
      Node current = frontier.back();

      if (current->in({Expr, Term}))
      {
        current = current->front();
      }

      if (current == UnifyVar)
      {
        return true;
      }

      frontier.pop_back();
      frontier.insert(frontier.end(), current->begin(), current->end());

      if (current != Var)
      {
        continue;
      }

      if (current->parent()->in({RefHead, RefArgDot}))
      {
        continue;
      }

      if (is_temporary(current))
      {
        continue;
      }

      if (in_query)
      {
        return true;
      }

      Nodes results = lookup_var(current);
      if (!results.empty() && results.front() != Local)
      {
        // not a local variable
        continue;
      }

      return true;
    }

    return false;
  }

  Node to_unifyvar(Node expr)
  {
    Nodes frontier({expr});
    while (!frontier.empty())
    {
      Node current = frontier.back();

      if (current->in({Expr, Term}))
      {
        current = current->front();
      }

      frontier.pop_back();

      if (current == UnifyVar)
      {
        continue;
      }

      if (
        current == Var && !current->parent()->in({RefHead, RefArgDot}) &&
        !is_temporary(current) && current->location().view() != "input")
      {
        Nodes results = current->lookup();
        if (results.empty() || results.front() == Local)
        {
          current->parent()->replace(current, UnifyVar ^ current);
        }

        continue;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }

    return expr;
  }

  Node ref_to_scan(Node seq, Node ref, Node withseq)
  {
    Node refhead = (ref / RefHead)->clone();
    Node argseq = ref / RefArgSeq;

    size_t start = 0;
    Node args = NodeDef::create(RefArgSeq);
    Node rule = ref->parent(Rule);
    if (rule == nullptr)
    {
      return err(ref, "Potential scan reference outside of a rule context");
    }

    bool in_query = (rule / Ident)->location().view().starts_with("query$");
    bool with_applied = false;
    for (size_t i = 0; i < argseq->size(); ++i)
    {
      Node arg = argseq->at(i);
      if (arg == RefArgDot)
      {
        args << arg->clone();
        continue;
      }

      if (arg->front() == Placeholder || find_locals(arg, in_query))
      {
        Location scanindex_name = ref->fresh({"scanindex"});
        Location scanvalue_name = ref->fresh({"scanvalue"});
        seq
          << (Literal << (ExprScan
                          << (Expr << (Term << (Ref << refhead << args)))
                          << (Local << (Ident ^ scanindex_name))
                          << (Local << (Ident ^ scanvalue_name)))
                      << (with_applied ? WithSeq : withseq));
        with_applied = true;
        if (arg->front() == Expr)
        {
          seq
            << (Literal << (ExprUnify
                            << to_unifyvar(arg->front())->clone()
                            << (Expr << (Term << (UnifyVar ^ scanindex_name))))
                        << WithSeq);
        }

        refhead = RefHead << (Var ^ scanvalue_name);
        args = NodeDef::create(RefArgSeq);
      }
      else
      {
        args << arg->clone();
      }
    }

    if (args->empty())
    {
      if (ref->parent(ExprUnify))
      {
        return UnifyVar ^ refhead->front();
      }

      return refhead->front();
    }

    return Ref << refhead << args;
  }

  /**
   * Find the implicit scans hiding as refs, and establish the rule-level
   * locals. This needs to go before the document merge, because that destroys
   * the file-level imports which can only be resolved after the rule locals
   * have been established.
   */
  PassDef implicit_scans()
  {
    auto scope_locals = std::make_shared<NodeMap<std::set<std::string>>>();
    PassDef pass = {
      "implicit_scans",
      wf_bundle_implicit_scans,
      dir::bottomup | dir::once,
      {
        In(Term) * T(Var)[Var] * In(ExprUnify)++ >> [](Match& _) -> Node {
          Nodes results = lookup_var(_(Var));
          if (results.empty() || results[0] == Local)
          {
            if (_(Var)->location().view() == "input")
            {
              return NoChange;
            }

            return UnifyVar ^ _(Var);
          }

          for (Node node : results)
          {
            if (node != ExprAssign)
            {
              return NoChange;
            }

            Node common_parent = _(Var)->common_parent(node);
            if (common_parent == RuleBodySeq)
            {
              // the assignment is in a different rule body
              continue;
            }

            if (common_parent != node)
            {
              // this var is not being assigned in this statement
              return NoChange;
            }
          }

          return UnifyVar ^ _(Var);
        },

        In(Ref) * (T(RefHead)[RefHead] << T(Var, "data")[Var]) *
            T(RefArgSeq)[RefArgSeq] >>
          [](Match& _) -> Node {
          Nodes results = _(Var)->lookup();
          assert(results.size() == 1);
          Node current = results.front() / BaseObject;
          Node refargseq = _(RefArgSeq);
          for (size_t i = 0; i < refargseq->size(); ++i)
          {
            Node arg = refargseq->at(i);
            if (arg == RefArgDot)
            {
              results = current->lookdown(arg->front()->location());
              if (results.empty())
              {
                break;
              }

              assert(results.size() == 1);
              current = results.front() / DataTerm;
            }
            else if (arg == RefArgBrack)
            {
              if (arg->front() == Placeholder || arg->front()->front() != Term)
              {
                break;
              }

              Node term = arg->front()->front();
              if (!is_constant(term))
              {
                break;
              }

              std::string key = to_key(term);
              results = current->lookdown({key});
              if (results.empty())
              {
                break;
              }

              assert(results.size() == 1);
              refargseq->replace_at(i, RefArgDot << (Var ^ key));
              current = results.front() / DataTerm;
            }
            else
            {
              return err(arg, "Unsupported arg type");
            }

            if (current->front() != BaseObject)
            {
              break;
            }

            current = current->front();
          }

          return NoChange;
        },

        In(Expr) *
            (T(ExprInfix)[ExprInfix]
             << ((T(Expr) << (T(Term) << T(Ref)[Ref])) *
                 T(InfixOperator)[InfixOperator] * T(Expr)[Rhs])) >>
          [](Match& _) -> Node {
          Node ref = _(Ref);
          if ((ref / RefArgSeq)->empty())
          {
            return NoChange;
          }

          Node literals = NodeDef::create(Seq);
          ref = ref_to_scan(literals, ref, NodeDef::create(WithSeq));

          if (literals->empty())
          {
            return NoChange;
          }

          Node seq = NodeDef::create(Seq);
          for (Node literal : *literals)
          {
            seq << (Lift << Query << literal);
          }

          seq
            << ((ExprInfix ^ _(ExprInfix))
                << (Expr << (Term << ref)) << _(InfixOperator) << _(Rhs));
          return seq;
        },

        In(Query) *
            (T(Literal)[Literal]
             << ((T(Expr)
                  << (T(ExprInfix)
                      << (T(Expr)[Lhs] * T(InfixOperator)[InfixOperator] *
                          (T(Expr) << (T(Term) << T(Ref)[Ref]))))) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) -> Node {
          Node ref = _(Ref);
          if ((ref / RefArgSeq)->empty())
          {
            return NoChange;
          }

          Node seq = NodeDef::create(Seq);
          ref = ref_to_scan(seq, ref, _(WithSeq));

          if (seq->empty())
          {
            return NoChange;
          }

          seq
            << ((Literal ^ _(Literal)) << (Expr
                                           << (ExprInfix
                                               << _(Lhs) << _(InfixOperator)
                                               << (Expr << (Term << ref))))
                                       << WithSeq);
          return seq;
        },

        In(Query) *
            (T(Literal)[Literal]
             << (T(ExprAssign)
                 << (T(AssignVar)[Lhs] *
                     (T(Expr) << (T(Term) << T(Ref)[Ref])))) *
               T(WithSeq)[WithSeq]) >>
          [](Match& _) -> Node {
          Node ref = _(Ref);
          if ((ref / RefArgSeq)->empty())
          {
            return NoChange;
          }

          Node seq = NodeDef::create(Seq);
          ref = ref_to_scan(seq, ref, _(WithSeq));

          if (seq->empty())
          {
            return NoChange;
          }

          seq
            << ((Literal ^ _(Literal))
                << (ExprAssign << _(Lhs) << (Expr << (Term << ref)))
                << WithSeq);
          return seq;
        },

        In(Query) *
            (T(Literal)[Literal]
             << (T(ExprUnify)
                 << ((T(Expr)[Lhs] << (T(Term) << T(Ref))) *
                     (T(Expr)[Rhs] << (T(Term) << T(Ref))))) *
               T(WithSeq)[WithSeq]) >>
          [](Match& _) -> Node {
          Node lhsref = _(Lhs)->front()->front();
          Node seq = NodeDef::create(Seq);
          bool with_applied = false;
          if (!(lhsref / RefArgSeq)->empty())
          {
            Node ref = ref_to_scan(seq, lhsref, _(WithSeq));
            if (!seq->empty())
            {
              lhsref = ref;
              with_applied = true;
            }
          }

          Node rhsref = _(Rhs)->front()->front();

          if (!(rhsref / RefArgSeq)->empty())
          {
            Node ref = ref_to_scan(
              seq,
              rhsref,
              with_applied ? NodeDef::create(WithSeq) : _(WithSeq));
            if (!seq->empty())
            {
              rhsref = ref;
              with_applied = true;
            }
          }

          if (seq->empty())
          {
            return NoChange;
          }

          seq
            << ((Literal ^ _(Literal))
                << (ExprUnify << (Expr << (Term << lhsref))
                              << (Expr << (Term << rhsref)))
                << WithSeq);
          return seq;
        },

        In(Query) *
            (T(Literal)[Literal]
             << (T(ExprUnify)
                 << (T(Expr)[Lhs] * (T(Expr) << (T(Term) << T(Ref)[Ref])))) *
               T(WithSeq)[WithSeq]) >>
          [](Match& _) -> Node {
          Node ref = _(Ref);
          if ((ref / RefArgSeq)->empty())
          {
            return NoChange;
          }

          Node seq = NodeDef::create(Seq);
          ref = ref_to_scan(seq, ref, _(WithSeq));

          if (seq->empty())
          {
            return NoChange;
          }

          seq
            << ((Literal ^ _(Literal))
                << (ExprUnify << _(Lhs) << (Expr << (Term << ref))) << WithSeq);
          return seq;
        },

        In(Query) *
            (T(Literal)[Literal]
             << (T(ExprUnify)
                 << ((T(Expr) << (T(Term) << T(Ref)[Ref])) * T(Expr)[Rhs])) *
               T(WithSeq)[WithSeq]) >>
          [](Match& _) -> Node {
          Node ref = _(Ref);
          if ((ref / RefArgSeq)->empty())
          {
            return NoChange;
          }

          Node seq = NodeDef::create(Seq);
          ref = ref_to_scan(seq, ref, _(WithSeq));

          if (seq->empty())
          {
            return NoChange;
          }

          seq
            << ((Literal ^ _(Literal))
                << (ExprUnify << (Expr << (Term << ref)) << _(Rhs))

                << WithSeq);
          return seq;
        },

        In(Query) *
            (T(Literal)
             << ((T(Expr) << (T(Term) << T(Ref)[Ref])) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) -> Node {
          Node ref = _(Ref);
          if ((ref / RefArgSeq)->empty())
          {
            return NoChange;
          }

          Node seq = NodeDef::create(Seq);
          ref = ref_to_scan(seq, ref, _(WithSeq));

          if (seq->empty())
          {
            return NoChange;
          }

          seq << (Literal << (Expr << (Term << ref)) << WithSeq);
          return seq;
        },

        In(Query) *
            (T(Literal)[Literal]
             << ((T(ExprScan)
                  << ((T(Expr) << (T(Term) << T(Ref)[Ref])) * T(Local)[Key] *
                      T(Local)[Val])) *
                 T(WithSeq)[WithSeq])) >>
          [](Match& _) -> Node {
          Node ref = _(Ref);
          if ((ref / RefArgSeq)->empty())
          {
            return NoChange;
          }

          Node seq = NodeDef::create(Seq);
          ref = ref_to_scan(seq, ref, _(WithSeq));

          if (seq->empty())
          {
            return NoChange;
          }

          seq
            << ((Literal ^ _(Literal))
                << (ExprScan << (Expr << (Term << ref)) << _(Key) << _(Val))
                << WithSeq);
          return seq;
        },

        In(Query) *
            (T(Literal)[Literal]
             << (T(Expr)
                 << (T(ExprCall) << (T(Ref)[Ref]
                                     << ((T(RefHead) << (T(Var, "walk"))) *
                                         (T(RefArgSeq) << End))) *
                       (T(ExprSeq) << (T(Expr)[Rhs] * T(Expr)[Lhs])))) *
               T(WithSeq)[WithSeq]) >>
          [](Match& _) {
            Location walk_name = _.fresh({"walk"});
            Location scanindex_name = _.fresh({"scanindex"});
            Location scanvalue_name = _.fresh({"scanvalue"});
            return Seq
              << (Literal
                  << (ExprAssign
                      << (AssignVar ^ walk_name)
                      << (Expr << (ExprCall << _(Ref) << (ExprSeq << _(Rhs)))))
                  << _(WithSeq)->clone())
              << (Literal << (ExprScan << (Expr << (Term << (Var ^ walk_name)))
                                       << (Local << (Ident ^ scanindex_name))
                                       << (Local << (Ident ^ scanvalue_name)))
                          << _(WithSeq)->clone())
              << (Literal << (ExprUnify
                              << _(Lhs)
                              << (Expr << (Term << (Var ^ scanvalue_name))))
                          << WithSeq);
          },

        In(Query) *
            (T(Literal)[Literal]
             << (T(ExprUnify)
                 << (T(Expr)[Lhs] *
                     (T(Expr)
                      << (T(ExprCall) << (T(Ref)[Ref]
                                          << ((T(RefHead) << (T(Var, "walk"))) *
                                              (T(RefArgSeq) << End))) *
                            (T(ExprSeq) << T(Expr)[Rhs]))))) *
               T(WithSeq)[WithSeq]) >>
          [](Match& _) {
            Location walk_name = _.fresh({"walk"});
            Location scanindex_name = _.fresh({"scanindex"});
            Location scanvalue_name = _.fresh({"scanvalue"});
            return Seq
              << (Literal
                  << (ExprAssign
                      << (AssignVar ^ walk_name)
                      << (Expr << (ExprCall << _(Ref) << (ExprSeq << _(Rhs)))))
                  << _(WithSeq)->clone())
              << (Literal << (ExprScan << (Expr << (Term << (Var ^ walk_name)))
                                       << (Local << (Ident ^ scanindex_name))
                                       << (Local << (Ident ^ scanvalue_name)))
                          << _(WithSeq)->clone())
              << (Literal << (ExprUnify
                              << _(Lhs)
                              << (Expr << (Term << (Var ^ scanvalue_name))))
                          << WithSeq);
          },

        In(Term) * T(Var)[Var] *
            In(
              RuleHeadComp,
              RuleHeadFunc,
              RuleHeadObj,
              RuleHeadObjDynamic,
              RuleHeadSet,
              RuleHeadSetDynamic)++ >>
          [scope_locals](Match& _) -> Node {
          Nodes results = lookup_var(_(Var));
          if (!results.empty() || _(Var)->parent(RefArgDot))
          {
            return NoChange;
          }

          Node scope = _(Var)->scope();

          std::string name(_(Var)->location().view());
          results = scope->look({name});
          if (!results.empty())
          {
            return NoChange;
          }

          if (scope_locals->find(scope) == scope_locals->end())
          {
            scope_locals->insert({scope, {}});
          }

          scope_locals->at(scope).insert(name);
          return NoChange;
        },

        In(Term) * T(Var)[Var] * In(ExprUnify)++ >>
          [scope_locals](Match& _) -> Node {
          Nodes results = lookup_var(_(Var));
          if (!results.empty())
          {
            return NoChange;
          }

          Node scope = _(Var)->scope();
          results = scope->look(_(Var)->location());
          if (!results.empty())
          {
            return NoChange;
          }

          if (scope_locals->find(scope) == scope_locals->end())
          {
            scope_locals->insert({scope, {}});
          }

          scope_locals->at(scope).insert(
            std::string(_(Var)->location().view()));

          return NoChange;
        },

        In(Rule, ExprEvery, ArrayCompr, ObjectCompr, SetCompr) *
            T(LocalSeq)[LocalSeq] >>
          [scope_locals](Match& _) {
            Node localseq = _(LocalSeq);
            Node rule = localseq->parent();
            if (scope_locals->find(rule) == scope_locals->end())
            {
              return localseq;
            }

            for (auto name : scope_locals->at(rule))
            {
              if (name == "input")
              {
                continue;
              }

              localseq << (Local << (Ident ^ name));
            }

            return localseq;
          },
      }};

    pass.pre([scope_locals](Node) {
      scope_locals->clear();
      return 0;
    });

    return pass;
  }

  inline const auto EntryPoint = TokenDef("rego-entrypoint");

  // clang-format off
  inline const auto wf_ir_merge =
    wf_bundle_implicit_scans
    | (RegoBundle <<= BaseDocument * VirtualDocument * (Query >>= VirtualDocument | Undefined) * Policy * ModuleFileSeq)
    | (ModuleFileSeq <<= ModuleFile++)
    | (ModuleFile <<= IRString * IRString)
    | (Policy <<= Static * EntryPointSeq * IRQuery)
    | (EntryPointSeq <<= EntryPoint++)
    | (EntryPoint <<= Ident * Ref)
    | (Static <<= PathSeq)
    | (PathSeq <<= IRString++)
    | (VirtualDocument <<= Ident * RuleSeq * DocumentSeq)[Ident]
    | (RuleSeq <<= Rule++)
    | (Rule <<= Ident * LocalSeq * (Default >>= True | False) * RuleHead * RuleBodySeq)[Ident]
    | (DocumentSeq <<= VirtualDocument++)
    | (RefArgBrack <<= Expr)
    ;
  // clang-format on

  Node add_or_find_child_document(Node parent, const std::string& child_name)
  {
    Node children = parent / DocumentSeq;
    for (Node child : *children)
    {
      Node ident = child->front();
      if (ident->location().view() == child_name)
      {
        return child;
      }
    }

    Node child = VirtualDocument << (Ident ^ child_name) << RuleSeq
                                 << DocumentSeq;
    children->push_back(child);
    return child;
  }

  std::optional<std::pair<Node, Node>> merge_documents(
    const std::vector<Node>& documents)
  {
    Node docseq = NodeDef::create(DocumentSeq);
    Node querydoc = Undefined;
    std::map<std::string, Node> docs;
    for (Node node : documents)
    {
      Node ref = node->front();
      Node refhead = ref / RefHead;
      Node refargseq = ref / RefArgSeq;

      std::string name(refhead->front()->location().view());
      if (refargseq->size() == 0)
      {
        if (docs.find(name) == docs.end())
        {
          docs[name] = node;
          node->replace_at(0, Ident ^ name);
          if (name.starts_with("querymodule$"))
          {
            querydoc = node;
          }
          else
          {
            docseq << node;
          }
        }
        else
        {
          docs[name] / RuleSeq << *(node / RuleSeq);
        }
        continue;
      }

      if (docs.find(name) == docs.end())
      {
        docs[name] = VirtualDocument << (Ident ^ name) << RuleSeq
                                     << DocumentSeq;
        if (name.starts_with("querymodule$"))
        {
          querydoc = docs[name];
        }
        else
        {
          docseq << docs[name];
        }
      }

      Node doc = docs[name];
      for (Node node : *refargseq)
      {
        std::string child_name;
        if (node == RefArgDot)
        {
          child_name = std::string(node->front()->location().view());
        }
        else
        {
          Node expr = node->front();
          if (expr != Expr)
          {
            return std::nullopt;
          }

          Node term = expr->front();
          auto maybe_name = unwrap(term, JSONString);
          if (maybe_name.success)
          {
            child_name = strip_quotes(maybe_name.node->location().view());
          }
          else
          {
            return std::nullopt;
          }
        }

        doc = add_or_find_child_document(doc, child_name);
      }

      doc / RuleSeq << *(node / RuleSeq);
    }

    return std::make_pair(
      VirtualDocument << (Ident ^ "data") << RuleSeq << docseq, querydoc);
  }

  Node to_stringseq(Token token, const std::map<Location, size_t>& strings)
  {
    Node seq = NodeDef::create(token);
    std::vector<Location> strings_vec(strings.size());
    for (const auto& [str, id] : strings)
    {
      strings_vec[id] = str;
    }

    for (const auto& str : strings_vec)
    {
      seq << (IRString ^ str);
    }

    return seq;
  }

  PassDef merge()
  {
    auto files = std::make_shared<std::map<Location, Source>>();
    auto documents = std::make_shared<Nodes>();

    PassDef pass =
      {
        "merge",
        wf_ir_merge,
        dir::bottomup | dir::once,
        {
          In(Term) * T(Var)[Var] >> [](Match& _) -> Node {
            Nodes lookup = _(Var)->lookup();
            if (!lookup.empty() && lookup.front() == Import)
            {
              return lookup.front() / Ref;
            }

            return NoChange;
          },

          In(Ref) * ((T(RefHead) << T(Var)[Var]) * T(RefArgSeq)[RefArgSeq]) *
              In(Expr)++ >>
            [](Match& _) -> Node {
            Nodes lookup = _(Var)->lookup();
            if (!lookup.empty() && lookup.front() == Import)
            {
              Node ref = (lookup.front() / Ref)->clone();
              Node refargseq = (ref / RefArgSeq) << *_(RefArgSeq);
              return Seq << (ref / RefHead) << refargseq;
            }

            return NoChange;
          },

          T(Module)[Module]
              << (T(Ident) * (T(Package) << T(Ref)[Ref]) * T(Version) *
                  T(ImportSeq) * T(Policy)[Policy]) >>
            [files, documents](Match& _) -> Node {
            Location loc = _(Module)->location();
            if (loc.source)
            {
              std::string src = loc.source->origin();
              if (!src.empty())
              {
                files->insert({src, loc.source});
              }
            }

            documents->push_back(
              VirtualDocument << _(Ref) << (RuleSeq << *_[Policy])
                              << DocumentSeq);
            return nullptr;
          },

          In(Rule) *
              (T(Ident)[Ident] *
               (T(Ref) << ((T(RefHead) << T(Var)) * (T(RefArgSeq) << End)))) >>
            [](Match& _) { return _(Ident); },

          In(Policy) *
              (T(Rule)[Rule]
               << (T(Ident)[Ident] *
                   (T(Ref)[Ref]
                    << ((T(RefHead) << T(Var)[RefHead]) *
                        T(RefArgSeq)[RefArgSeq])))) >>
            [documents](Match& _) -> Node {
            // ref rules
            Node ref = (_(Ident)->parent(Module) / Package)->front()->clone();

            Node ident = Ident ^ _(RefArgSeq)->back()->front();
            _(RefArgSeq)->pop_back();

            ref / RefArgSeq << (RefArgDot << _(RefHead));
            ref / RefArgSeq << *_(RefArgSeq);

            Node rule = _(Rule)->clone();
            rule->replace_at(0, ident);
            rule->replace_at(1, nullptr);
            documents->push_back(
              VirtualDocument << ref << (RuleSeq << rule) << DocumentSeq);
            return nullptr;
          },

          In(Policy) * (T(Rule)[Rule] << (T(Ident)[Ident] * T(Ref)[Ref])) >>
            [](Match& _) { return err(_(Ref), "Invalid rule refhead"); },

          In(EntryPointSeq) * T(IRString)[IRString] >>
            [](Match& _) {
              std::string entry_point(_(IRString)->location().view());
              size_t start = 0;
              size_t end = entry_point.find('/');

              Nodes parts;
              while (end != std::string::npos)
              {
                parts.push_back(Var ^ entry_point.substr(start, end - start));
                start = end + 1;
                end = entry_point.find('/', start);
              }

              std::string last_part = entry_point.substr(start);
              if (!last_part.empty())
              {
                parts.push_back(Var ^ last_part);
              }

              if (entry_point.starts_with("querymodule$"))
              {
                return EntryPoint
                  << (Ident ^ _(IRString))
                  << (Ref << (RefHead << parts[0])
                          << (RefArgSeq << (RefArgDot << parts[1])));
              }

              Node refargseq = NodeDef::create(RefArgSeq);
              for (Node part : parts)
              {
                refargseq << (RefArgDot << part);
              }

              return EntryPoint
                << (Ident ^ _(IRString))
                << (Ref << (RefHead << (Var ^ "data")) << refargseq);
            },

          In(RegoBundle) *
              (T(EntryPointSeq)[EntryPointSeq] * T(BaseDocument)[BaseDocument] *
               T(ModuleSeq)[ModuleSeq] * T(IRQuery)[IRQuery]) >>
            [files, documents](Match& _) -> Node {
            Node pathseq = NodeDef::create(PathSeq);
            Node fileseq = NodeDef::create(ModuleFileSeq);
            for (auto& [name, source] : *files)
            {
              pathseq << (IRString ^ name);
              fileseq
                << (ModuleFile
                    << (IRString ^ name)
                    << (IRString ^ Location(source, 0, source->view().size())));
            }

            auto maybe_merge = merge_documents(*documents);
            if (!maybe_merge.has_value())
            {
              return err(_(ModuleSeq), "Unable to merge virtual documents");
            }

            auto [virtualdoc, querydoc] = maybe_merge.value();
            return Seq << _(BaseDocument) << virtualdoc << querydoc
                       << (Policy << (Static << pathseq) << _(EntryPointSeq)
                                  << _(IRQuery))
                       << fileseq;
          },

          // errors

          In(RefArgBrack) * T(Placeholder)[Placeholder] >>
            [](Match& _) {
              return err(_(Placeholder), "Invalid bracket argument");
            },
        }};

    pass.pre([documents, files](Node) {
      documents->clear();
      files->clear();
      return 0;
    });

    return pass;
  }

  // clang-format off
  inline const auto wf_ir_unify =
    wf_ir_merge
    | (ExprScan <<= Expr * (Key >>= Local) * (Val >>= Local) * Query)
    | (ExprAssign <<= (Lhs >>= AssignVar) * (Rhs >>= Expr))
    | (Literal <<= (Expr >>= ExprIsArray | ExprIsObject | ExprAssignFromArray | ExprAssignFromObject | ExprAssign | ExprScan | ExprEvery | Local | Expr | NotExpr) * WithSeq)
    | (LocalSeq <<= Local++)
    ;
  // clang-format on

  Node lookup_rule(Node ref)
  {
    assert(ref == Ref);
    Node head = (ref / RefHead)->front();
    if (head != Var)
    {
      return nullptr;
    }

    Nodes results = head->lookup();
    if (results.empty())
    {
      return nullptr;
    }

    if (results.front() == Rule)
    {
      return results.front();
    }

    Node doc;
    if (head->location().view() == "data")
    {
      if (results[0] == VirtualDocument)
      {
        doc = results[0];
      }
      else
      {
        doc = results[1];
      }
    }
    else
    {
      doc = results[0];
    }

    Node result;
    Node refargseq = ref / RefArgSeq;
    for (size_t i = 0; i < refargseq->size(); ++i)
    {
      assert(doc == VirtualDocument);

      Node arg = refargseq->at(i);
      if (arg == RefArgBrack)
      {
        return nullptr;
      }

      results = doc->lookdown(arg->front()->location());
      if (results.empty())
      {
        return nullptr;
      }

      if (i == refargseq->size() - 1)
      {
        if (results.front() == Rule)
        {
          return results.front();
        }

        auto it =
          std::find_if(results.begin(), results.end(), [](const Node& node) {
            return node == Rule;
          });

        if (it == results.end())
        {
          return nullptr;
        }

        return *it;
      }

      doc = results.front();
      if (doc != VirtualDocument)
      {
        auto it =
          std::find_if(results.begin(), results.end(), [](const Node& node) {
            return node == VirtualDocument;
          });
        if (it == results.end())
        {
          return nullptr;
        }
        doc = *it;
      }
    }

    return nullptr;
  }

  PassDef unify(const BuiltIns& builtins)
  {
    auto scope_locals = std::make_shared<NodeMap<std::set<std::string>>>();
    PassDef pass = {
      "unify",
      wf_ir_unify,
      dir::bottomup | dir::once,
      {
        In(Term) * T(Var)[Var] * In(ExprUnify)++ >> [](Match& _) -> Node {
          Nodes results = _(Var)->lookup();
          if (!results.empty())
          {
            return NoChange;
          }

          if (_(Var)->location().view() == "input")
          {
            return NoChange;
          }

          return UnifyVar ^ _(Var);
        },

        In(ExprUnify) * T(FuncArgVar)[Var] >>
          [](Match& _) { return Expr << (Term << (Var ^ _(Var))); },

        In(Literal, NotExpr) *
            (T(Expr)[Expr] << ((
               T(ExprCall)[ExprCall]
               << (T(Ref)[Ref] << (T(RefHead) << T(Var)[Var]))))) >>
          [builtins](Match& _) -> Node {
          Node rulefunc = lookup_rule(_(Ref));
          size_t arity = 0;
          if (rulefunc)
          {
            Node ruleheadfunc = (rulefunc / RuleHead)->front();
            assert(ruleheadfunc == RuleHeadFunc);
            Node ruleargs = ruleheadfunc / RuleArgs;
            arity = ruleargs->size();
          }
          else
          {
            auto maybe_string = ref_to_string(_(Ref));
            if (!maybe_string.has_value())
            {
              return NoChange;
            }

            Location name(maybe_string.value());
            if (!builtins->is_builtin(name))
            {
              return NoChange;
            }

            auto builtin = builtins->at(name);
            arity = builtin->arity;
          }

          Node exprcall = _(ExprCall);
          Node args = exprcall / ExprSeq;

          if (args->size() != arity + 1)
          {
            return NoChange;
          }

          // an older calling convention for functions allowed an extra
          // argument to be provided that is then unified with
          // the result of the call.

          Node expr = args->back();
          args->pop_back();
          if (_(Expr)->parent() == NotExpr)
          {
            return Expr
              << (ExprInfix << expr
                            << (InfixOperator << (BoolOperator << Equals))
                            << (Expr << _(ExprCall)));
          }

          return ExprUnify << to_unifyvar(expr) << (Expr << _(ExprCall));
        },

        In(Query) * (T(Literal)++[Literal]) >>
          [scope_locals](Match& _) {
            Node scope = _[Literal].front()->scope();
            DependencyGraph graph(scope, _[Literal]);
            Node error = graph.sort();
            if (error)
            {
              return error;
            }

            if (scope_locals->find(scope) == scope_locals->end())
            {
              scope_locals->insert({scope, {}});
            }

            graph.merge_locals(scope_locals->at(scope));
            return graph.orderedseq();
          },

        In(LocalSeq) * (T(EveryLocal) << T(Ident)[Ident]) >>
          [](Match& _) { return Local << _(Ident); },

        In(Rule, ExprEvery, ArrayCompr, ObjectCompr, SetCompr) *
            T(LocalSeq)[LocalSeq] >>
          [scope_locals](Match& _) -> Node {
          Node scope = _(LocalSeq)->parent();
          auto it = scope_locals->find(scope);
          if (it == scope_locals->end())
          {
            return NoChange;
          }

          std::set<std::string> existing;
          for (Node local : *_(LocalSeq))
          {
            existing.insert(std::string(local->front()->location().view()));
          }

          for (auto& name : it->second)
          {
            if (existing.find(name) == existing.end())
            {
              _(LocalSeq) << (Local << (Ident ^ name));
            }
          }

          return NoChange;
        },

        // errors

        In(ExprScan) * (T(Local) << T(Ident)[Ident]) >> [](Match& _) -> Node {
          std::string_view name = _(Ident)->location().view();
          if (name.starts_with("scan"))
          {
            return NoChange;
          }

          return err(_(Ident), "Invalid scan index/value ident");
        },
      }};

    pass.post([scope_locals](Node) {
      scope_locals->clear();
      return 0;
    });

    return pass;
  }

  inline const auto ResultExprSeq = TokenDef("rego-resultexprseq");
  inline const auto ResultExpr = TokenDef("rego-resultexpr");
  inline const auto wf_ir_expr_to_opblock_stmts =
    (wf_ir_statements | BuiltInCallStmt | WithCallStmt) - ResultSetAddStmt;

  // clang-format off
  inline const auto wf_ir_expr_to_opblock =
    wf_ir_unify
    | (VirtualDocument <<= Ident * RuleSeq * DocumentSeq)[Ident]
    | (EntryPoint <<= Ident * OpBlock)
    | (RuleHeadComp <<= OpBlock)
    | (RuleHeadObj <<= Ident * (Key >>= OpBlock) * (Val >>= OpBlock))
    | (RuleHeadObjDynamic <<= OpBlockSeq * OpBlock)
    | (RuleHeadFunc <<= RuleArgs * OpBlock)
    | (RuleHeadSet <<= OpBlock)
    | (RuleHeadSetDynamic <<= OpBlockSeq * OpBlock)
    | (Else <<= OpBlock * Query)
    | (OpBlock <<= Operand * Block)
    | (OpBlockSeq <<= OpBlock++[1])
    | (RuleHeadQuery <<= Query * ResultExprSeq)
    | (ResultExprSeq <<= ResultExpr++)
    | (ResultExpr <<= IRString * Operand)
    | (Literal <<= (Expr >>= AssignBlock | OpBlock | ScanStmt | NotStmt | WithStmt))
    | (OpBlock <<= Operand * Block)
    | (AssignBlock <<= LocalRef * Block)
    | (Block <<= wf_ir_expr_to_opblock_stmts++)
    | (RefHead <<= Var)
    | (ArrayAppendStmt <<= (Array >>= LocalRef) * (Val >>= Operand))
    | (AssignIntStmt <<= (Val >>= Int64) * (Target >>= LocalRef))
    | (AssignVarOnceStmt <<= (Src >>= Operand) * (Target >>= LocalRef))
    | (AssignVarStmt <<= (Src >>= Operand) * (Target >>= LocalRef))
    | (BlockStmt <<= (Blocks >>= BlockSeq))
    | (BlockSeq <<= Block++)
    | (BreakStmt <<= (Idx >>= UInt32))
    | (CallStmt <<= (Func >>= Ref) * (Args >>= OperandSeq) * (rego::Result >>= LocalRef))
    | (CallDynamicStmt <<= (Func >>= OperandSeq) * (Args >>= OperandSeq) * (rego::Result >>= LocalRef))
    | (DotStmt <<= (Src >>= Operand) * (Key >>= Operand) * (Target >>= LocalRef))
    | (EqualStmt <<= (Lhs >>= Operand) * (Rhs >>= Operand))
    | (IsArrayStmt <<= (Src >>= Operand))
    | (IsDefinedStmt <<= (Src >>= LocalRef))
    | (IsObjectStmt <<= (Src >>= Operand))
    | (IsSetStmt <<= (Src >>= Operand))
    | (IsUndefinedStmt <<= (Src >>= LocalRef))    
    | (LenStmt <<= (Src >>= Operand) * (Target >>= LocalRef))
    | (MakeArrayStmt <<= (Capacity >>= Int32) * (Target >>= LocalRef))
    | (MakeNullStmt <<= (Target >>= LocalRef))
    | (MakeNumberIntStmt <<= (Val >>= Int64) * (Target >>= LocalRef))
    | (MakeNumberRefStmt <<= (Idx >>= IRString) * (Target >>= LocalRef))
    | (MakeObjectStmt <<= (Target >>= LocalRef))
    | (MakeSetStmt <<= (Target >>= LocalRef))
    | (NotEqualStmt <<= (Lhs >>= Operand) * (Rhs >>= Operand))
    | (NotStmt <<= Block)
    | (ObjectInsertOnceStmt <<= (Key >>= Operand) * (Val >>= Operand) * (Object >>= LocalRef))
    | (ObjectInsertStmt <<= (Key >>= Operand) * (Val >>= Operand) * (Object >>= LocalRef))
    | (ObjectMergeStmt <<= (Lhs >>= LocalRef) * (Rhs >>= LocalRef) * (Target >>= LocalRef))
    | (ResetLocalStmt <<= (Target >>= LocalRef))
    | (ReturnLocalStmt <<= (Src >>= LocalRef))
    | (ScanStmt <<= (Src >>= OpBlock) * (Key >>= LocalRef) * (Val >>= LocalRef) * Block)
    | (SetAddStmt <<= (Val >>= Operand) * (Set >>= LocalRef))
    | (WithStmt <<= LocalRef * (Path >>= StringSeq) * (Val >>= Operand) * (Expr >>= AssignBlock | OpBlock | ScanStmt | NotStmt | WithStmt))
    | (With <<= (Key >>= Ref | LocalRef | Var) * (Val >>= Ref | LocalRef | OpBlock))
    | (BuiltInCallStmt <<= (Func >>= IRString) * (Args >>= OperandSeq) * (rego::Result >>= LocalRef))
    | (WithCallStmt <<= (Func >>= Ref) * (Args >>= OperandSeq) * (rego::Result >>= LocalRef) * WithSeq)
    | (RefArgBrack <<= OpBlock)
    | (Operand <<= LocalRef | Boolean | IRString)
    | (OperandSeq <<= Operand++)
    | (StringSeq <<= IRString++)
    ;
  // clang-format on

  std::string withref_to_string(Node ref)
  {
    if (ref == LocalRef)
    {
      return std::string(ref->location().view());
    }

    if (ref == Var)
    {
      auto full_ref = to_absolute_path(ref);
      if (full_ref == nullptr || full_ref == Var)
      {
        return std::string(ref->location().view());
      }

      ref = full_ref;
    }

    Node head = (ref / RefHead)->front();
    Node argseq = ref / RefArgSeq;

    std::string_view head_view = head->location().view();

    std::ostringstream buf;
    char delim = '/';
    if (head_view == "data" || head_view == "input")
    {
      delim = '.';
    }

    buf << head->location().view();
    for (Node arg : *argseq)
    {
      if (arg == RefArgDot)
      {
        buf << delim << arg->front()->location().view();
      }
      else
      {
        logging::Warn() << "Invalid with reference argument: " << arg;
        buf << delim << "<error>";
      }
    }

    return buf.str();
  }

  void process_withcalls(
    Node node, Node withseq, const std::set<std::string>& lookups)
  {
    Nodes frontier({node});

    Location loc = withseq->parent()->location();

    std::map<std::string, Node> withrefs;
    for (Node with : *withseq)
    {
      if (with / Val == OpBlock)
      {
        continue;
      }

      std::string key = withref_to_string(with / Key);
      Node val = with / Val;
      if (val == Var)
      {
        val = Ref << (RefHead << val) << RefArgSeq;
      }

      withrefs[key] = val;
    }

    while (!frontier.empty())
    {
      Node current = frontier.back();
      frontier.pop_back();

      if (current == CallStmt)
      {
        Node ref = current / Func;
        std::string ref_string = withref_to_string(ref);
        if (lookups.find(ref_string) != lookups.end())
        {
          // this call is replaced by a constant value
          Node parent = current->parent();
          Nodes stmts;

          Node operands = current / Args;
          for (size_t i = 2; i < operands->size(); ++i)
          {
            Node op = operands->at(i);
            if (op->front() == LocalRef)
            {
              stmts.push_back((IsDefinedStmt ^ loc) << op->front()->clone());
            }
          }

          if (!ref_string.starts_with("data"))
          {
            stmts.push_back(
              (AssignVarStmt ^ loc) << (Operand << (LocalRef ^ ref_string))
                                    << (current / rego::Result));
          }
          else
          {
            Location with_name("data");
            Node head = (ref / RefHead)->front();
            assert(head->location().view() == "data");
            Node argseq = ref / RefArgSeq;
            for (size_t i = 0; i < argseq->size(); ++i)
            {
              Node dot = argseq->at(i);
              assert(dot == RefArgDot);
              Location child_name = withseq->fresh({"with"});
              stmts.push_back(
                (DotStmt ^ loc) << (Operand << (LocalRef ^ with_name))
                                << (Operand << (IRString ^ dot->front()))
                                << (LocalRef ^ child_name));
              with_name = child_name;
            }
            stmts.push_back(
              (AssignVarStmt ^ loc)
              << (Operand << (LocalRef ^ with_name)) << current / rego::Result);
          }

          // we need to replace the call with the statements above
          // which assign the constant value
          auto it = parent->find(current);
          parent->erase(it, it + 1);
          parent->insert(it, stmts.begin(), stmts.end());
          continue;
        }

        if (withrefs.find(ref_string) == withrefs.end())
        {
          // not directly replaced, so we need to ensure the call
          // inherits the withseq at the call site. The next pass
          // will reify this new version of the function.

          if (ref_string.starts_with("data."))
          {
            current->parent()->replace(
              current, (WithCallStmt ^ loc) << *current << withseq->clone());
            continue;
          }

          // built-in call
          continue;
        }

        // simple replacement
        current / Func = withrefs[ref_string];
        continue;
      }

      if (current->empty())
      {
        continue;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }
  }

  Node expand_withseq(Node expr, Node withseq)
  {
    if (withseq->empty())
    {
      return expr;
    }

    Location loc = withseq->parent()->location();

    std::set<std::string> lookups;

    Node block = NodeDef::create(Block);
    Node withstmt;
    Node innerwithstmt;
    for (Node with : *withseq)
    {
      if (!(with / Val)->in({OpBlock, LocalRef}) && with / Key != LocalRef)
      {
        continue;
      }

      auto maybe_ref = unwrap(with / Key, {Ref, LocalRef, Var});
      if (!maybe_ref.success)
      {
        return err(with / Term, "Unable to unwrap with term");
      }

      Node ref = maybe_ref.node;
      Node head;
      Node stringseq = NodeDef::create(StringSeq);

      if (ref == Ref)
      {
        ref = to_absolute_path(ref);
        if (ref == nullptr || ref == Var)
        {
          logging::Warn() << "Invalid with reference: " << ref;
          return err(with, "Invalid with reference");
        }

        std::ostringstream path_buf;
        Node refhead = ref / RefHead;
        Node refargs = ref / RefArgSeq;
        if (refhead->front() != Var)
        {
          return err(ref, "No variable-head references for with statement");
        }

        std::string_view head_view = refhead->front()->location().view();
        if (head_view == "data" || head_view == "input")
        {
          head = LocalRef ^ refhead->front();
          path_buf << head_view;
          for (Node arg : *refargs)
          {
            if (arg == RefArgDot)
            {
              stringseq << (IRString ^ arg->front());
              path_buf << '.' << arg->front()->location().view();
            }
            else
            {
              return err(arg, "No bracket expressions for a with statement");
            }
          }
        }
        else
        {
          // built-in
          path_buf << head_view;
          for (Node arg : *refargs)
          {
            if (arg == RefArgDot)
            {
              path_buf << '/' << arg->front()->location().view();
            }
            else
            {
              return err(arg, "No bracket expressions for a with statement");
            }
          }
          head = LocalRef ^ path_buf.str();
        }

        lookups.insert(path_buf.str());
      }
      else if (ref == LocalRef)
      {
        head = ref;
      }
      else
      {
        logging::Warn() << "Invalid with reference: " << ref;
        return err(ref, "Invalid with reference");
      }

      Node val_operand;
      if (with / Val == OpBlock)
      {
        Node opblock = with / Val;
        block << *(opblock / Block);
        val_operand = opblock / Operand;
      }
      else if (with / Val == LocalRef)
      {
        val_operand = Operand << with / Val;
      }
      else if (with / Ref == LocalRef)
      {
        Node opblock = ref_to_opblock(with / Ref);
        block << *(opblock / Block);
        val_operand = opblock / Operand;
      }
      else
      {
        return err(val_operand, "Invalid with value");
      }

      Node stmt = (WithStmt ^ loc) << head << stringseq << val_operand << Block;
      if (withstmt)
      {
        innerwithstmt / Expr = stmt;
      }
      else
      {
        withstmt = stmt;
      }

      innerwithstmt = stmt;
    }

    process_withcalls(expr, withseq, lookups);

    if (innerwithstmt)
    {
      innerwithstmt / Expr = expr;
      block << withstmt;
      if (expr == OpBlock)
      {
        return OpBlock << (expr / Operand)->clone() << block;
      }

      Location with_name = withseq->fresh({"with"});

      get_inner(block)
        << ((AssignVarStmt ^ loc)
            << (Operand << (Boolean ^ "true")) << (LocalRef ^ with_name));
      return OpBlock << (Operand << (LocalRef ^ with_name)) << block;
    }

    return expr;
  }

  bool contains_token(Node node, Token token)
  {
    Nodes frontier({node});
    while (!frontier.empty())
    {
      Node current = frontier.back();
      frontier.pop_back();

      if (current->type() == token)
      {
        return true;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }

    return false;
  }

  PassDef expr_to_opblock(const BuiltIns& builtins)
  {
    auto rule_locals = std::make_shared<NodeMap<Nodes>>();
    auto resultexprs = std::make_shared<Nodes>();
    PassDef pass =
      {
        "expr_to_opblock",
        wf_ir_expr_to_opblock,
        dir::bottomup | dir::once,
        {
          In(RefHead) * T(Array)[Array] >>
            [](Match& _) { return ArgVal << array_to_opblock(_(Array)); },

          In(RefHead) * T(Object)[Object] >>
            [](Match& _) { return ArgVal << object_to_opblock(_(Object)); },

          In(RefHead) * T(Set)[Set] >>
            [](Match& _) { return ArgVal << set_to_opblock(_(Set)); },

          In(RefHead) * T(ArrayCompr)[ArrayCompr] >>
            [](Match& _) {
              return ArgVal << arraycompr_to_opblock(_(ArrayCompr));
            },

          In(RefHead) * T(ObjectCompr)[ObjectCompr] >>
            [](Match& _) {
              return ArgVal << objectcompr_to_opblock(_(ObjectCompr));
            },

          In(RefHead) * T(SetCompr)[SetCompr] >>
            [](Match& _) { return ArgVal << setcompr_to_opblock(_(SetCompr)); },

          In(RefHead) *
              (T(ExprCall)[ExprCall] << (T(Ref) << (T(RefHead) << (T(Var))))) >>
            [](Match& _) { return ArgVal << call_to_opblock(_(ExprCall)); },

          In(Else, ObjectItem, Array, Set, RefArgBrack, With) *
              (T(Expr) << T(OpBlock)[OpBlock]) >>
            [](Match& _) { return _(OpBlock); },

          In(
            RuleHeadComp,
            RuleHeadFunc,
            RuleHeadObj,
            RuleHeadObjDynamic,
            RuleHeadSet,
            RuleHeadSetDynamic,
            Else,
            TermSeq) *
              T(Term)[Term] >>
            [](Match& _) { return term_to_opblock(_(Term)); },

          In(RuleHeadObjDynamic, RuleHeadSetDynamic) * T(TermSeq)[TermSeq] >>
            [](Match& _) {
              if (_(TermSeq)->empty())
              {
                return err(_(TermSeq), "Invalid path");
              }

              return OpBlockSeq << *_[TermSeq];
            },

          In(Expr) *
              (T(ExprInfix)
               << ((T(Expr) << T(OpBlock, Term)[Lhs]) *
                   (T(InfixOperator) << T(ArithOperator)[Op]) *
                   (T(Expr) << T(OpBlock, Term)[Rhs]))) >>
            [](Match& _) {
              Location infix_name = _.fresh({"infix"});
              Node op = _(Op)->front();
              std::string builtin_name;
              if (op == Add)
              {
                builtin_name = "plus";
              }
              else if (op == Subtract)
              {
                builtin_name = "minus";
              }
              else if (op == Multiply)
              {
                builtin_name = "mul";
              }
              else if (op == Divide)
              {
                builtin_name = "div";
              }
              else if (op == Modulo)
              {
                builtin_name = "rem";
              }
              else
              {
                return err(_(Op), "Unsupported arithmetic operator");
              }

              Node block = NodeDef::create(Block);
              Node lhs = to_operand(block, _(Lhs));
              Node rhs = to_operand(block, _(Rhs));
              block
                << (BuiltInCallStmt << (IRString ^ builtin_name)
                                    << (OperandSeq << lhs << rhs)
                                    << (LocalRef ^ infix_name));

              return OpBlock << (Operand << (LocalRef ^ infix_name)) << block;
            },

          In(Expr) *
              (T(ExprInfix)
               << ((T(Expr) << T(OpBlock, Term)[Lhs]) *
                   (T(InfixOperator) << T(BoolOperator)[Op]) *
                   (T(Expr) << T(OpBlock, Term)[Rhs]))) >>
            [](Match& _) {
              Location infix_name = _.fresh({"infix"});
              Node op = _(Op)->front();
              std::string builtin_name;
              Node block = NodeDef::create(Block);
              Node lhs = to_operand(block, _(Lhs));
              Node rhs = to_operand(block, _(Rhs));

              if (op == Equals)
              {
                if (_(Op)->parent(ExprAssign) == nullptr)
                {
                  block << (EqualStmt << lhs << rhs)
                        << (AssignVarStmt << (Operand << (Boolean ^ "true"))
                                          << (LocalRef ^ infix_name));
                  return OpBlock << (Operand << (LocalRef ^ infix_name))
                                 << block;
                }

                builtin_name = "equal";
              }
              else if (op == GreaterThan)
              {
                builtin_name = "gt";
              }
              else if (op == GreaterThanOrEquals)
              {
                builtin_name = "gte";
              }
              else if (op == LessThan)
              {
                builtin_name = "lt";
              }
              else if (op == LessThanOrEquals)
              {
                builtin_name = "lte";
              }
              else if (op == NotEquals)
              {
                builtin_name = "neq";
              }
              else
              {
                return err(_(Op), "Unsupported operator");
              }

              block
                << (BuiltInCallStmt << (IRString ^ builtin_name)
                                    << (OperandSeq << lhs << rhs)
                                    << (LocalRef ^ infix_name));

              return OpBlock << (Operand << (LocalRef ^ infix_name)) << block;
            },

          In(Expr) *
              (T(ExprInfix)
               << ((T(Expr) << T(OpBlock, Term)[Lhs]) *
                   (T(InfixOperator) << T(BinOperator)[Op]) *
                   (T(Expr) << T(OpBlock, Term)[Rhs]))) >>
            [](Match& _) {
              Location infix_name = _.fresh({"infix"});
              Node op = _(Op)->front();
              std::string builtin_name;
              Node block = NodeDef::create(Block);
              Node lhs = to_operand(block, _(Lhs));
              Node rhs = to_operand(block, _(Rhs));

              if (op == And)
              {
                builtin_name = "and";
              }
              else if (op == Or)
              {
                builtin_name = "or";
              }
              else
              {
                return err(_(Op), "Unsupported operator");
              }

              block
                << (BuiltInCallStmt << (IRString ^ builtin_name)
                                    << (OperandSeq << lhs << rhs)
                                    << (LocalRef ^ infix_name));

              return OpBlock << (Operand << (LocalRef ^ infix_name)) << block;
            },

          T(Literal) << T(Local)[Local] >> [rule_locals](Match& _) -> Node {
            Node rule = _(Local)->parent(Rule);
            if (rule_locals->find(rule) == rule_locals->end())
            {
              rule_locals->insert({rule, {}});
            }

            rule_locals->at(rule).push_back(_(Local));
            return nullptr;
          },

          In(Literal) *
              ((T(ExprAssign) << (T(AssignVar)[Lhs] * (T(Expr)[Rhs]))) *
               T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              Location term_name = _.fresh({"term"});
              Node block = NodeDef::create(Block);
              Node rhs = to_operand(block, _(Rhs));
              block << (AssignVarStmt << rhs << (LocalRef ^ _(Lhs)))
                    << (AssignVarStmt << (Operand << (Boolean ^ "true"))
                                      << (LocalRef ^ term_name));
              return expand_withseq(
                AssignBlock << (LocalRef ^ term_name) << block, _(WithSeq));
            },

          In(Literal) *
              ((T(ExprIsArray) << (T(Var)[Var] * T(Int)[Int])) *
               T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              if (!_(WithSeq)->empty())
              {
                logging::Warn()
                  << "Anomalous WithSeq on ExprIsArray literal: " << _(WithSeq);
              }

              Location isarray_name = _.fresh({"isarray"});
              Location len_name = _.fresh({"len"});
              Location size_name = _.fresh({"size"});
              Node block = NodeDef::create(Block);
              Node ref = to_operand(block, _(Var));
              block << (IsArrayStmt << ref)
                    << (LenStmt << ref->clone() << (LocalRef ^ len_name))
                    << (MakeNumberIntStmt << (Int64 ^ _(Int))
                                          << (LocalRef ^ size_name))
                    << (EqualStmt << (Operand << (LocalRef ^ len_name))
                                  << (Operand << (LocalRef ^ size_name)))
                    << (AssignVarStmt << (Operand << (Boolean ^ "true"))
                                      << (LocalRef ^ isarray_name));
              return OpBlock << (Operand << (LocalRef ^ isarray_name)) << block;
            },

          In(Literal) *
              ((T(ExprIsObject) << (T(Var)[Var] * T(Int)[Int])) *
               T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              if (!_(WithSeq)->empty())
              {
                logging::Warn() << "Anomalous WithSeq on ExprIsObject literal: "
                                << _(WithSeq);
              }

              Location isobject_name = _.fresh({"isobject"});
              Location len_name = _.fresh({"len"});
              Location size_name = _.fresh({"size"});
              Node block = NodeDef::create(Block);
              Node ref = to_operand(block, _(Var));
              block << (IsObjectStmt << ref->clone())
                    << (LenStmt << ref->clone() << (LocalRef ^ len_name))
                    << (MakeNumberIntStmt << (Int64 ^ _(Int))
                                          << (LocalRef ^ size_name))
                    << (EqualStmt << (Operand << (LocalRef ^ len_name))
                                  << (Operand << (LocalRef ^ size_name)))
                    << (AssignVarStmt << (Operand << (Boolean ^ "true"))
                                      << (LocalRef ^ isobject_name));
              return OpBlock << (Operand << (LocalRef ^ isobject_name))
                             << block;
            },

          In(Literal) *
              ((T(ExprAssignFromArray)
                << (T(AssignVar)[AssignVar] * T(Var)[Array] * T(Int)[Int])) *
               T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              if (!_(WithSeq)->empty())
              {
                logging::Warn()
                  << "Anomalous WithSeq on ExprAssignFromArray literal: "
                  << _(WithSeq);
              }

              Location index_name = _.fresh({"index"});
              Node block = NodeDef::create(Block);
              Node array = to_operand(block, _(Array));
              block << (MakeNumberIntStmt << (Int64 ^ _(Int))
                                          << (LocalRef ^ index_name))
                    << (DotStmt << array << (Operand << (LocalRef ^ index_name))
                                << (LocalRef ^ _(AssignVar)));
              return AssignBlock << (LocalRef ^ _(AssignVar)) << block;
            },

          In(Literal) *
              ((T(ExprAssignFromObject)
                << (T(AssignVar)[AssignVar] * T(Var)[Object] * T(Expr)[Key])) *
               T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              if (!_(WithSeq)->empty())
              {
                logging::Warn()
                  << "Anomalous WithSeq on ExprAssignFromObject literal: "
                  << _(WithSeq);
              }

              Location index_name = _.fresh({"index"});
              Node block = NodeDef::create(Block);
              Node object = to_operand(block, _(Object));
              Node key = to_operand(block, _(Key));
              block << (DotStmt << object << key << (LocalRef ^ _(AssignVar)));
              return AssignBlock << (LocalRef ^ _(AssignVar)) << block;
            },

          In(Literal) *
              ((T(ExprScan)
                << ((T(Expr) << T(OpBlock, Term)[OpBlock]) * T(Local)[Key] *
                    T(Local)[Val] * T(Query)[Query])) *
               T(WithSeq)[WithSeq]) >>
            [rule_locals, resultexprs](Match& _) {
              Node opblock = _(OpBlock);
              if (opblock == Term)
              {
                opblock = term_to_opblock(opblock);
              }

              Node rule = _(Key)->parent(Rule);
              if (rule_locals->find(rule) == rule_locals->end())
              {
                rule_locals->insert({rule, {}});
              }

              rule_locals->at(rule).push_back(_(Key));
              rule_locals->at(rule).push_back(_(Val));

              Node ruleheadquery = _(Query)->parent(RuleHeadQuery);

              Node block = NodeDef::create(Block);
              for (Node literal : *_(Query))
              {
                add_literal_to_block(block, literal);
                if (ruleheadquery)
                {
                  Node expr = literal->front();
                  if (expr == OpBlock)
                  {
                    resultexprs->push_back(
                      ResultExpr << (IRString ^ literal->location())
                                 << (expr / Operand)->clone());
                  }
                  else if (expr == AssignBlock)
                  {
                    resultexprs->push_back(
                      ResultExpr << (IRString ^ literal->location())
                                 << (Operand << (Boolean ^ "true")));
                  }
                }
              }

              return expand_withseq(
                ScanStmt << opblock << (LocalRef ^ _(Key)->front())
                         << (LocalRef ^ _(Val)->front()) << block,
                _(WithSeq));
            },

          In(Literal) *
              ((T(ExprEvery)
                << (T(LocalSeq) * T(Var)[Var] * T(Placeholder, Var)[Key] *
                    T(Var)[Val] * T(Query)[Query])) *
               T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              Location scanindex_name = _.fresh({"scanindex"});
              Location scanvalue_name = _.fresh({"scanvalue"});
              Location itemok_name = _.fresh({"itemok"});
              Location onebad_name = _.fresh({"onebad"});
              Location allok_name = _.fresh({"allok"});

              Node everyblock = NodeDef::create(Block);
              for (Node literal : *_(Query))
              {
                if (literal->size() != 1)
                {
                  everyblock << err(literal, "Invalid literal");
                  continue;
                }

                add_literal_to_block(everyblock, literal);
              }

              get_inner(everyblock)
                << (AssignVarStmt << (Operand << (Boolean ^ "true"))
                                  << (LocalRef ^ itemok_name));

              Node scanblock = NodeDef::create(Block);
              if (_(Key) == Var)
              {
                scanblock
                  << (AssignVarStmt << (Operand << (LocalRef ^ scanindex_name))
                                    << (LocalRef ^ _(Key)));
              }

              scanblock << (AssignVarStmt
                            << (Operand << (LocalRef ^ scanvalue_name))
                            << (LocalRef ^ _(Val)))
                        << (ResetLocalStmt << (LocalRef ^ itemok_name))
                        << (BlockStmt << (BlockSeq << everyblock))
                        << (IsUndefinedStmt << (LocalRef ^ itemok_name))
                        << (AssignVarStmt << (Operand << (Boolean ^ "true"))
                                          << (LocalRef ^ onebad_name));

              Node scanstmt =
                (ScanStmt << (OpBlock << (Operand << (LocalRef ^ _(Var)))
                                      << Block)
                          << (LocalRef ^ scanindex_name)
                          << (LocalRef ^ scanvalue_name) << scanblock);

              Node block = Block << (ResetLocalStmt << (LocalRef ^ onebad_name))
                                 << (IsDefinedStmt << (LocalRef ^ _(Var)));

              if (_(WithSeq)->empty())
              {
                block << scanstmt;
              }
              else
              {
                Node opblock = expand_withseq(scanstmt, _(WithSeq));
                if (opblock == Error)
                {
                  return opblock;
                }

                block << *(opblock / Block);
              }

              block << (IsUndefinedStmt << (LocalRef ^ onebad_name))
                    << (AssignVarStmt << (Operand << (Boolean ^ "true"))
                                      << (LocalRef ^ allok_name));

              return OpBlock << (Operand << (LocalRef ^ allok_name)) << block;
            },

          In(Expr) *
              (T(ExprCall)[ExprCall] << (T(Ref) << (T(RefHead) << (T(Var))))) >>
            [](Match& _) { return call_to_opblock(_(ExprCall)); },

          In(Expr, RefHead) * T(ExprCall)[ExprCall] >>
            [](Match& _) { return err(_(ExprCall), "Invalid call"); },

          In(Expr) *
              (T(UnaryExpr) << (T(Expr) << (T(OpBlock, Term)[OpBlock]))) >>
            [](Match& _) {
              Node opblock = _(OpBlock);
              if (opblock == Term)
              {
                opblock = term_to_opblock(opblock);
              }

              if (opblock == Error)
              {
                logging::Error() << "Invalid opblock: " << opblock;
                return opblock;
              }

              Node term_block = opblock / Block;

              Location unary_name = _.fresh({"unary"});
              Location zero_name = _.fresh({"zero"});
              return OpBlock
                << (Operand << (LocalRef ^ unary_name))
                << (Block << *term_block
                          << (MakeNumberIntStmt << (Int64 ^ "0")
                                                << (LocalRef ^ zero_name))
                          << (BuiltInCallStmt
                              << (IRString ^ "minus")
                              << (OperandSeq
                                  << (Operand << (LocalRef ^ zero_name))
                                  << (opblock / Operand))
                              << (LocalRef ^ unary_name)));
            },

          In(Literal) *
              ((T(NotExpr) << (T(Expr) << T(OpBlock, Term)[OpBlock])) *
               T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              Node opblock = _(OpBlock);
              if (opblock == Term)
              {
                opblock = term_to_opblock(opblock);
              }

              Node term_block = opblock / Block;
              term_block
                << (NotEqualStmt << (opblock / Operand)
                                 << (Operand << (Boolean ^ "false")));
              return expand_withseq(NotStmt << term_block, _(WithSeq));
            },

          In(Literal) *
              ((T(Expr) << T(OpBlock, Term)[OpBlock]) * T(WithSeq)[WithSeq]) >>
            [](Match& _) {
              Node opblock = _(OpBlock);
              if (opblock == Term)
              {
                opblock = term_to_opblock(opblock);
              }

              return expand_withseq(opblock, _(WithSeq));
            },

          In(RuleHeadQuery) * T(Query)[Query] >>
            [resultexprs](Match& _) {
              return Seq << _(Query) << (ResultExprSeq << *resultexprs);
            },

          In(With) * (T(Term) << T(Ref)[Ref]) >>
            [](Match& _) { return _(Ref); },

          In(With) * (T(Term) << T(Var, "input")[Var]) >>
            [](Match& _) { return LocalRef ^ _(Var); },

          In(With) * (T(Term) << T(Var)[Var]) >>
            [builtins](Match& _) {
              Nodes results = _(Var)->lookup();
              if (results.empty() || builtins->is_builtin(_(Var)->location()))
              {
                return Ref << (RefHead << _(Var)) << RefArgSeq;
              }

              if (results.front() == Local)
              {
                return LocalRef ^ _(Var);
              }

              Node ref = to_absolute_path(_(Var));
              if (ref == Ref)
              {
                return ref;
              }

              return err(_(Var), "Invalid with reference");
            },

          In(With) *
              (T(Expr)
               << (T(Term)[Term] << (T(
                     Array,
                     Object,
                     Set,
                     Scalar,
                     ArrayCompr,
                     ObjectCompr,
                     SetCompr)))) >>
            [](Match& _) { return term_to_opblock(_(Term)); },

          In(With) * (T(Expr) << (T(Term)[Term] << T(Ref)[Ref])) >>
            [builtins](Match& _) -> Node {
            Node ref = to_absolute_path(_(Ref));
            if (ref == nullptr)
            {
              return term_to_opblock(_(Term));
            }

            return ref;
          },

          In(With) * (T(Expr) << (T(Term)[Term] << T(Var, "input")[Var])) >>
            [builtins](Match& _) -> Node { return LocalRef ^ _(Var); },

          In(With) * (T(Expr) << (T(Term)[Term] << T(Var)[Var])) >>
            [builtins](Match& _) -> Node {
            Nodes results = _(Var)->lookup();
            if (results.empty() || builtins->is_builtin(_(Var)->location()))
            {
              return Ref << (RefHead << _(Var)) << RefArgSeq;
            }

            if (results.front() == Local)
            {
              return LocalRef ^ _(Var);
            }

            return to_absolute_path(_(Var));
          },

          In(ObjectItem, Array, Set, RefArgBrack) *
              (T(Expr)
               << (T(Term)[Term] << T(Scalar, Object, Array, Set, Var, Ref))) >>
            [](Match& _) { return term_to_opblock(_(Term)); },

          In(Rule) * T(LocalSeq)[LocalSeq] >>
            [rule_locals](Match& _) {
              Node localseq = _(LocalSeq);
              Node rule = localseq->parent();
              if (rule_locals->find(rule) == rule_locals->end())
              {
                return localseq;
              }

              localseq << rule_locals->at(rule);
              return localseq;
            },

          In(EntryPoint) * T(Ref)[Ref] >>
            [](Match& _) { return ref_to_opblock(_(Ref)); },

          // errors

          In(With) * T(Term)[Term] >>
            [](Match& _) { return err(_(Term), "Invalid with term"); },

          In(RefArgBrack) * T(Expr)[Expr] >>
            [](Match& _) {
              return err(_(Expr), "Invalid reference bracket expression");
            },
        }};

    pass.pre([rule_locals, resultexprs](Node) {
      rule_locals->clear();
      resultexprs->clear();
      return 0;
    });

    return pass;
  }

  inline const auto FuncName = TokenDef("rego-funcname");
  inline const auto Body = TokenDef("rego-bodycomp");
  inline const auto BodyData = TokenDef("rego-bodydata");
  inline const auto BodyObj = TokenDef("rego-bodyobj");
  inline const auto BodyObjStatic = TokenDef("rego-bodyobjstatic");
  inline const auto BodyObjDynamic = TokenDef("rego-bodyobjdynamic");
  inline const auto ObjValBlockSeq = TokenDef("rego-objvalblockseq");
  inline const auto BodySet = TokenDef("rego-bodyset");
  inline const auto BodySetDynamic = TokenDef("rego-bodysetdynamic");
  inline const auto CompBlockSeq = TokenDef("rego-compblockseq");
  inline const auto ObjValSeq = TokenDef("rego-objvalseq");
  inline const auto ObjPathBlockSeq = TokenDef("rego-objpathblockseq");
  inline const auto SetBlockSeq = TokenDef("rego-setblockseq");
  inline const auto SetPathBlockSeq = TokenDef("rego-setpathblockseq");
  inline const auto BodySeq = TokenDef("rego-bodyseq");
  inline const auto HasArgs = TokenDef("rego-hasargs");

  inline const auto wf_ir_lift_functions_stmts = wf_ir_expr_to_opblock_stmts;

  // clang-format off
  inline const auto wf_ir_lift_functions =
    wf_ir_expr_to_opblock
    | builtins::wf_decl
    | (Static <<= PathSeq * BuiltInFunctionSeq)
    | (BuiltInFunctionSeq <<= BuiltInFunction++)
    | (BuiltInFunction <<= IRString * builtins::Decl)
    | (VirtualDocument <<= Ident * RuleSeq * DocumentSeq)[Ident]
    | (Function <<= (Name >>= IRString) * IRPath * ParameterSeq * LocalRef * BodySeq)[Name]
    | (IRPath <<= IRString++[1])
    | (With <<= (Key >>= Var | Ref | LocalRef) * (Val >>= Ref | LocalRef | OpBlock))
    | (RefHead <<= Var)
    | (BodySeq <<= (Default | BodyData | Body | BodyObj | BodySet)++[1])
    | (BodyData <<= AssignBlock)
    | (Body <<= CompBlockSeq++)
    | (BodyObj <<= BodyObjStatic | BodyObjDynamic | BodySetDynamic)
    | (BodyObjStatic <<= Ident * (Key >>= OpBlock) * ObjValSeq)
    | (BodyObjDynamic <<= ObjPathBlockSeq)
    | (ObjPathBlockSeq <<= (Path >>= OpBlockSeq) * (Val >>= OpBlock) * BlockSeq)
    | (BodySet <<= SetBlockSeq)
    | (BodySetDynamic <<= SetPathBlockSeq)
    | (SetPathBlockSeq <<= (Path >>= OpBlockSeq) * (Val >>= OpBlock) * BlockSeq)
    | (CompBlockSeq <<= (Val >>= OpBlock) * BlockSeq)
    | (ObjValSeq <<= ObjValBlockSeq++)
    | (ObjValBlockSeq <<= (Val >>= OpBlock) * BlockSeq)
    | (SetBlockSeq <<= (Val >>= OpBlock) * BlockSeq)
    | (Block <<= wf_ir_lift_functions_stmts++)
    | (Default <<= OpBlock)
    | (StringSeq <<= IRString++)
    | (RuleSeq <<= Rule++)
    | (Policy <<= Static * EntryPointSeq * IRQuery * FunctionSeq)
    | (FunctionSeq <<= Function++)
    | (Rule <<= Ident * (FuncName >>= IRString) * (HasArgs >>= True | False))[Ident]
    | (ParameterSeq <<= LocalRef++)
    | (ScanStmt <<= LocalRef * (Key >>= LocalRef) * (Val >>= LocalRef) * Block)
    | (WithStmt <<= LocalRef * (Path >>= StringSeq) * (Val >>= Operand) * Block)
    ;
  // clang-format on

  Node to_ref(Node localref)
  {
    if (
      localref->location().view() == "data" ||
      localref->location().view() == "input")
    {
      return Ref << (RefHead << (Var ^ localref)) << RefArgSeq;
    }

    Nodes nodes = localref->lookup();
    if (nodes.empty())
    {
      return nullptr;
    }

    if (nodes.front() == Local)
    {
      Node local = nodes.front();
      std::string local_name(local->front()->location().view());
      return LocalRef ^ local_name;
    }

    std::vector<std::string> path;
    Node current = nodes.front();
    while (current)
    {
      path.emplace_back((current / Ident)->location().view());
      current = current->parent(VirtualDocument);
    }

    Node head = RefHead << (Var ^ path.back());
    path.pop_back();
    Node args = NodeDef::create(RefArgSeq);
    for (auto it = path.rbegin(); it != path.rend(); ++it)
    {
      args << (RefArgDot << (Var ^ *it));
    }

    return Ref << head << args;
  }

  bool is_builtin(BuiltIns builtins, Node ref)
  {
    Location name;
    if (ref == Var)
    {
      name = ref->location();
    }
    else
    {
      Node head = ref / RefHead;
      Node argseq = ref / RefArgSeq;

      name = head->front()->location();
      if (!argseq->empty())
      {
        Location lastarg = argseq->back()->location();
        name.len = lastarg.pos + lastarg.len - name.pos;
      }
    }

    return builtins->is_builtin(name);
  }

  std::string rule_ident_to_string(Node ident, const std::string& prefix = "g0")
  {
    Node doc = ident->parent(VirtualDocument);
    std::vector<std::string_view> doc_idents;
    doc_idents.push_back(ident->location().view());
    while (doc)
    {
      doc_idents.push_back((doc / Ident)->location().view());
      doc = doc->parent(VirtualDocument);
    }

    std::ostringstream path_buf;
    path_buf << prefix;

    while (!doc_idents.empty())
    {
      std::string_view part = doc_idents.back();
      doc_idents.pop_back();
      if (part.find('.') != std::string::npos)
      {
        path_buf << "[" << part << "]";
        continue;
      }

      path_buf << '.' << part;
    }

    return path_buf.str();
  }

  Node rule_ident_to_path(Node ident, const std::string& start)
  {
    Node doc = ident->parent(VirtualDocument);
    std::vector<Location> doc_idents;
    while (doc)
    {
      doc_idents.push_back((doc / Ident)->location());
      doc = doc->parent(VirtualDocument);
    }

    doc_idents.pop_back(); // remove the "data" document

    Node path = IRPath << (IRString ^ start);
    while (!doc_idents.empty())
    {
      path << (IRString ^ doc_idents.back());
      doc_idents.pop_back();
    }

    path << (IRString ^ ident);
    return path;
  }

  Node rule_ident_to_body(Node ident)
  {
    Node doc = ident->parent(VirtualDocument);
    Nodes results = doc->lookdown(ident->location());

    Node virtualdoc;
    auto it =
      std::find_if(results.begin(), results.end(), [](const Node& node) {
        return node == VirtualDocument;
      });

    if (it != results.end())
    {
      virtualdoc = *it;
    }

    Node basedoc = ident->parent(RegoBundle) / BaseDocument;
    std::vector<Location> path{ident->location()};
    while (doc != nullptr)
    {
      path.push_back((doc / Ident)->location());
      doc = doc->parent(VirtualDocument);
    }

    path.pop_back(); // data
    while (!path.empty())
    {
      Nodes base_results = basedoc->lookdown(path.back());
      if (base_results.empty())
      {
        basedoc = nullptr;
        break;
      }

      basedoc = results.front();
    }

    if (virtualdoc == nullptr && basedoc == nullptr)
    {
      return nullptr;
    }

    Location doc_name = ident->fresh({"doc"});
    Node doc_block = Block << (MakeObjectStmt << (LocalRef ^ doc_name));
    if (virtualdoc != nullptr)
    {
      doc_block << document_stmt(virtualdoc, doc_name, nullptr, 0);
    }

    if (basedoc != nullptr)
    {
      Location bdoc_name = ident->fresh({"bdoc"});
      doc_block
        << (BlockStmt
            << (BlockSeq << base_block(basedoc, bdoc_name)
                         << (Block
                             << (IsDefinedStmt << (LocalRef ^ bdoc_name))
                             << (ObjectMergeStmt << (LocalRef ^ bdoc_name)
                                                 << (LocalRef ^ doc_name)
                                                 << (LocalRef ^ doc_name)))));
    }

    return BodyData << (AssignBlock << (LocalRef ^ doc_name) << doc_block);
  }

  Node arith_builtin_ref(Node op)
  {
    if (op == Add)
    {
      return Ref << (RefHead << (Var ^ "add")) << RefArgSeq;
    }
    else if (op == Subtract)
    {
      return Ref << (RefHead << (Var ^ "sub")) << RefArgSeq;
    }
    else if (op == Multiply)
    {
      return Ref << (RefHead << (Var ^ "mul")) << RefArgSeq;
    }
    else if (op == Divide)
    {
      return Ref << (RefHead << (Var ^ "div")) << RefArgSeq;
    }
    else if (op == Modulo)
    {
      return Ref << (RefHead << (Var ^ "rem")) << RefArgSeq;
    }
    else
    {
      return err(op, "Unknown arithmetic operator");
    }
  }

  /** This pass lifts all rules as functions that can be called from plans.
   */
  PassDef lift_functions(const BuiltIns& builtins)
  {
    auto functions = std::make_shared<std::map<std::string, Node>>();
    auto builtinfunctions = std::make_shared<std::set<std::string>>();
    PassDef pass = {
      "lift_functions",
      wf_ir_lift_functions,
      dir::bottomup | dir::once,
      {
        In(Block) * (T(BuiltInCallStmt) << T(IRString)[IRString]) >>
          [builtinfunctions](Match& _) -> Node {
          std::string func_name(_(IRString)->location().view());
          builtinfunctions->insert(func_name);
          return NoChange;
        },

        In(Block) *
            (T(CallStmt)
             << ((T(Ref)[Ref]
                  << ((T(RefHead) << T(Var)[RefHead]) *
                      T(RefArgSeq)[RefArgSeq])) *
                 T(OperandSeq)[Args] * T(LocalRef)[Target])) >>
          [builtins, builtinfunctions](Match& _) -> Node {
          Node ref = to_ref(_(RefHead));
          if (ref)
          {
            ref / RefArgSeq << *_[RefArgSeq];
            return CallStmt << ref << _(Args) << _(Target);
          }

          auto maybe_string = ref_to_string(_(Ref));
          if (
            maybe_string.has_value() &&
            builtins->is_builtin(maybe_string.value()))
          {
            const std::string& refstring = maybe_string.value();
            // no need to pass input and data to builtins
            Node args = NodeDef::create(OperandSeq);
            if (_(Args)->size() > 2)
            {
              args->insert(args->end(), _(Args)->begin() + 2, _(Args)->end());
            }

            builtinfunctions->insert(refstring);

            return BuiltInCallStmt << (IRString ^ refstring) << args
                                   << _(Target);
          }

          return NoChange;
        },

        In(Query) *
            (T(Literal)
             << (T(ScanStmt)
                 << (T(OpBlock)[OpBlock] * T(LocalRef)[Key] * T(LocalRef)[Val] *
                     T(Block)[Block]))) >>
          [](Match& _) {
            Node scansource_ref = (_(OpBlock) / Operand)->front();
            if (scansource_ref != LocalRef)
            {
              return err(scansource_ref, "Invalid scan source");
            }

            return Seq << (Literal << _(OpBlock))
                       << (Literal
                           << (ScanStmt << scansource_ref->clone() << _(Key)
                                        << _(Val) << _(Block)));
          },

        In(Block) *
            (T(ScanStmt)
             << (T(OpBlock)[OpBlock] * T(LocalRef)[Key] * T(LocalRef)[Val] *
                 T(Block)[Block])) >>
          [](Match& _) {
            Node scansource_ref = (_(OpBlock) / Operand)->front();
            if (scansource_ref != LocalRef)
            {
              return err(scansource_ref, "Invalid scan source");
            }

            Node scansource_block = _(OpBlock) / Block;
            return Seq << *scansource_block
                       << (ScanStmt << scansource_ref->clone() << _(Key)
                                    << _(Val) << _(Block));
          },

        In(WithStmt) *
            (T(ScanStmt)
             << (T(OpBlock)[OpBlock] * T(LocalRef)[Key] * T(LocalRef)[Val] *
                 T(Block)[Block])) >>
          [](Match& _) {
            Node scansource_ref = (_(OpBlock) / Operand)->front();
            if (scansource_ref != LocalRef)
            {
              return err(scansource_ref, "Invalid scan source");
            }

            Node scansource_block = _(OpBlock) / Block;
            return Block << *scansource_block
                         << (ScanStmt << scansource_ref->clone() << _(Key)
                                      << _(Val) << _(Block));
          },

        In(WithStmt) * (T(OpBlock) << (T(Operand) * T(Block)[Block])) >>
          [](Match& _) { return _(Block); },

        In(WithStmt) * (T(AssignBlock) << (T(LocalRef) * T(Block)[Block])) >>
          [](Match& _) { return _(Block); },

        In(WithStmt) * (T(WithStmt, NotStmt)[Stmt]) >>
          [](Match& _) { return Block << _(Stmt); },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq) * T(True) *
                (T(RuleHead) << (T(RuleHeadComp) << T(OpBlock)[OpBlock])) *
                (T(RuleBodySeq) << T(Query)[Query])) >>
          [functions](Match& _) -> Node {
          Node param_input = LocalRef ^ "input";
          Node param_data = LocalRef ^ "data";
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");

          Node block = _(OpBlock) / Block;
          for (Node literal : *_(Query))
          {
            add_literal_to_block(block, literal);
          }

          Node def = Default << _(OpBlock);

          if (functions->find(func_name) == functions->end())
          {
            Location return_name = _.fresh({"compreturn"});
            functions->emplace(
              func_name,
              (Function ^ _(Rule))
                << (IRString ^ func_name) << path
                << (ParameterSeq << param_input << param_data)
                << (LocalRef ^ return_name) << (BodySeq << def));
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (False ^ "false");
          }

          Node function = functions->at(func_name);
          function / BodySeq << def;
          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq) * T(False) *
                (T(RuleHead) << (T(RuleHeadComp) << T(OpBlock)[OpBlock])) *
                T(RuleBodySeq)[RuleBodySeq]) >>
          [functions](Match& _) -> Node {
          Node param_input = LocalRef ^ "input";
          Node param_data = LocalRef ^ "data";
          Location return_name = _.fresh({"compreturn"});
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");

          Node vdoc = _(Ident)->parent(VirtualDocument);
          bool generate_rule = false;
          Node function;
          if (functions->find(func_name) == functions->end())
          {
            function = (Function ^ _(Rule))
              << (IRString ^ func_name) << path
              << (ParameterSeq << param_input << param_data)
              << (LocalRef ^ return_name) << BodySeq;
            functions->emplace(func_name, function);
            generate_rule = true;
          }
          else
          {
            function = functions->at(func_name);
          }

          Node body = NodeDef::create(Body);
          Node current = CompBlockSeq << _(OpBlock) << BlockSeq;
          for (Node query : *_(RuleBodySeq))
          {
            if (query == Else)
            {
              body << current;
              current = CompBlockSeq << (query / OpBlock) << BlockSeq;
              query = query / Query;
            }

            Node body_block = NodeDef::create(Block);

            for (Node literal : *query)
            {
              add_literal_to_block(body_block, literal);
            }

            current / BlockSeq << body_block;
          }

          body << current;
          function / BodySeq << body;

          if (generate_rule)
          {
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (False ^ "false");
          }

          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq) * T(True) *
                (T(RuleHead)
                 << (T(RuleHeadFunc)
                     << (T(RuleArgs)[RuleArgs] * T(OpBlock)[OpBlock]))) *
                ((T(RuleBodySeq) << T(Query)[Query]))) >>
          [functions](Match& _) -> Node {
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");

          Node block = _(OpBlock) / Block;
          for (Node literal : *_(Query))
          {
            add_literal_to_block(block, literal);
          }

          Node def = Default << _(OpBlock);

          if (functions->find(func_name) == functions->end())
          {
            Node paramseq = ParameterSeq << (LocalRef ^ "input")
                                         << (LocalRef ^ "data");
            for (Node var : *_(RuleArgs))
            {
              paramseq << (LocalRef ^ var);
            }

            Location return_name = _.fresh({"funcreturn"});
            functions->emplace(
              func_name,
              (Function ^ _(Rule))
                << (IRString ^ func_name) << path << paramseq
                << (LocalRef ^ return_name) << (BodySeq << def));
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (True ^ "true");
          }

          Node function = functions->at(func_name);
          function / BodySeq << def;
          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq)[LocalSeq] * T(False) *
                (T(RuleHead)
                 << (T(RuleHeadFunc)
                     << (T(RuleArgs)[RuleArgs] * T(OpBlock)[OpBlock]))) *
                T(RuleBodySeq)[RuleBodySeq]) >>
          [functions](Match& _) -> Node {
          Location return_name = _.fresh({"funcreturn"});
          Node value_ref = _(LocalRef);
          Node value_block = _(Block);
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");
          bool generate_rule = false;
          Node function;
          if (functions->find(func_name) == functions->end())
          {
            Node paramseq = ParameterSeq << (LocalRef ^ "input")
                                         << (LocalRef ^ "data");
            for (Node var : *_(RuleArgs))
            {
              paramseq << (LocalRef ^ var);
            }

            function = (Function ^ _(Rule))
              << (IRString ^ func_name) << path << paramseq
              << (LocalRef ^ return_name) << BodySeq;

            functions->emplace(func_name, function);
            generate_rule = true;
          }
          else
          {
            function = functions->at(func_name);
          }

          Node paramseq = function / ParameterSeq;

          Node body = NodeDef::create(Body);
          Node current = CompBlockSeq << _(OpBlock) << BlockSeq;
          for (Node query : *_(RuleBodySeq))
          {
            if (query == Else)
            {
              body << current;
              current = CompBlockSeq << (query / OpBlock) << BlockSeq;
              query = query / Query;
            }

            Node body_block = NodeDef::create(Block);

            for (Node literal : *query)
            {
              add_literal_to_block(body_block, literal);
            }

            current / BlockSeq << body_block;
          }

          body << current;
          function / BodySeq << body;

          if (generate_rule)
          {
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (True ^ "true");
          }

          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident) * T(LocalSeq) * T(True) *
                (T(RuleHead) << T(RuleHeadComp, RuleHeadFunc))) >>
          [](Match& _) { return err(_(Rule), "Invalid default rule"); },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq) * T(False) *
                (T(RuleHead) << (T(RuleHeadSet) << T(OpBlock)[OpBlock])) *
                T(RuleBodySeq)[RuleBodySeq]) >>
          [functions](Match& _) -> Node {
          Node param_input = LocalRef ^ "input";
          Node param_data = LocalRef ^ "data";
          Location return_name = _.fresh({"setreturn"});
          Node value_ref = _(LocalRef);
          Node value_block = _(Block);
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");

          bool generate_rule = false;
          Node function;
          if (functions->find(func_name) == functions->end())
          {
            function = (Function ^ _(Rule))
              << (IRString ^ func_name) << path
              << (ParameterSeq << param_input << param_data)
              << (LocalRef ^ return_name) << BodySeq;
            functions->emplace(func_name, function);
            generate_rule = true;
          }
          else
          {
            function = functions->at(func_name);
          }

          Node body = NodeDef::create(BodySet);
          Node current = SetBlockSeq << _(OpBlock) << BlockSeq;
          for (Node query : *_(RuleBodySeq))
          {
            if (query == Else)
            {
              return err(query, "else statement in set");
            }

            Node body_block = NodeDef::create(Block);

            for (Node literal : *query)
            {
              add_literal_to_block(body_block, literal);
            }

            current / BlockSeq << body_block;
          }

          body << current;
          function / BodySeq << body;

          if (generate_rule)
          {
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (False ^ "false");
          }

          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq) * T(False) *
                (T(RuleHead)
                 << (T(RuleHeadSetDynamic)
                     << (T(OpBlockSeq)[OpBlockSeq] * T(OpBlock)[OpBlock]))) *
                T(RuleBodySeq)[RuleBodySeq]) >>
          [functions](Match& _) -> Node {
          Node param_input = LocalRef ^ "input";
          Node param_data = LocalRef ^ "data";
          Location return_name = _.fresh({"setreturn"});
          Node value_ref = _(LocalRef);
          Node value_block = _(Block);
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");

          bool generate_rule = false;
          Node function;
          if (functions->find(func_name) == functions->end())
          {
            function = (Function ^ _(Rule))
              << (IRString ^ func_name) << path
              << (ParameterSeq << param_input << param_data)
              << (LocalRef ^ return_name)
              << (BodySeq << rule_ident_to_body(_(Ident)));
            functions->emplace(func_name, function);
            generate_rule = true;
          }
          else
          {
            function = functions->at(func_name);
          }

          Node body = NodeDef::create(BodySetDynamic);
          Node current = SetPathBlockSeq << _(OpBlockSeq) << _(OpBlock)
                                         << BlockSeq;
          for (Node query : *_(RuleBodySeq))
          {
            if (query == Else)
            {
              return err(query, "else statement in set");
            }

            Node body_block = NodeDef::create(Block);

            for (Node literal : *query)
            {
              add_literal_to_block(body_block, literal);
            }

            current / BlockSeq << body_block;
          }

          body << current;
          function / BodySeq << (BodyObj << body);

          if (generate_rule)
          {
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (False ^ "false");
          }

          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq) * T(False) *
                (T(RuleHead)
                 << (T(RuleHeadObj)
                     << (T(Ident)[String] * T(OpBlock)[Key] *
                         T(OpBlock)[Val]))) *
                T(RuleBodySeq)[RuleBodySeq]) >>
          [functions](Match& _) -> Node {
          Node param_input = LocalRef ^ "input";
          Node param_data = LocalRef ^ "data";
          Location return_name = _.fresh({"objreturn"});
          Node value_ref = _(LocalRef);
          Node value_block = _(Block);
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");

          bool generate_rule = false;
          Node function;
          if (functions->find(func_name) == functions->end())
          {
            function = (Function ^ _(Rule))
              << (IRString ^ func_name) << path
              << (ParameterSeq << param_input << param_data)
              << (LocalRef ^ return_name)
              << (BodySeq << rule_ident_to_body(_(Ident)));
            functions->emplace(func_name, function);
            generate_rule = true;
          }
          else
          {
            function = functions->at(func_name);
          }

          Node objvalseq = NodeDef::create(ObjValSeq);

          Node current = ObjValBlockSeq << _(Val) << BlockSeq;
          for (Node query : *_(RuleBodySeq))
          {
            if (query == Else)
            {
              objvalseq << current;
              current = ObjValBlockSeq << (query / OpBlock) << BlockSeq;
              query = query / Query;
            }

            Node body_block = NodeDef::create(Block);

            for (Node literal : *query)
            {
              add_literal_to_block(body_block, literal);
            }

            current / BlockSeq << body_block;
          }
          objvalseq << current;

          function / BodySeq
            << (BodyObj
                << (BodyObjStatic << _(String)->clone() << _(Key)
                                  << (objvalseq)));

          if (generate_rule)
          {
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (False ^ "false");
          }

          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq) * T(False) *
                (T(RuleHead)
                 << (T(RuleHeadObjDynamic)
                     << (T(OpBlockSeq)[OpBlockSeq] * T(OpBlock)[OpBlock]))) *
                T(RuleBodySeq)[RuleBodySeq]) >>
          [functions](Match& _) -> Node {
          Node param_input = LocalRef ^ "input";
          Node param_data = LocalRef ^ "data";
          Location return_name = _.fresh({"objreturn"});
          Node value_ref = _(LocalRef);
          Node value_block = _(Block);
          std::string func_name = rule_ident_to_string(_(Ident), "g0");
          Node path = rule_ident_to_path(_(Ident), "g0");

          bool generate_rule = false;
          Node function;
          if (functions->find(func_name) == functions->end())
          {
            function = (Function ^ _(Rule))
              << (IRString ^ func_name) << path
              << (ParameterSeq << param_input << param_data)
              << (LocalRef ^ return_name)
              << (BodySeq << rule_ident_to_body(_(Ident)));
            functions->emplace(func_name, function);
            generate_rule = true;
          }
          else
          {
            function = functions->at(func_name);
          }

          Node body = NodeDef::create(BodyObjDynamic);
          Node current = ObjPathBlockSeq << _(OpBlockSeq) << _(OpBlock)
                                         << BlockSeq;
          for (Node query : *_(RuleBodySeq))
          {
            if (query == Else)
            {
              return err(query, "else statement in set");
            }

            Node body_block = NodeDef::create(Block);

            for (Node literal : *query)
            {
              add_literal_to_block(body_block, literal);
            }

            current / BlockSeq << body_block;
          }

          body << current;
          function / BodySeq << (BodyObj << body);

          if (generate_rule)
          {
            return Rule << _(Ident) << (IRString ^ func_name)
                        << (False ^ "false");
          }

          return nullptr;
        },

        T(Rule)[Rule]
            << (T(Ident)[Ident] * T(LocalSeq)[LocalSeq] * T(False) *
                (T(RuleHead)
                 << (T(RuleHeadQuery)
                     << (T(Query)[Query] * T(ResultExprSeq)[ResultExprSeq]))) *
                T(RuleBodySeq)[RuleBodySeq]) >>
          [functions](Match& _) {
            if (!_(RuleBodySeq)->empty())
            {
              return err(_(RuleBodySeq), "non-empty query body");
            }

            Node param_input = LocalRef ^ "input";
            Node param_data = LocalRef ^ "data";
            Location return_name = _.fresh({"return"});
            Location result_name = _.fresh({"result"});
            Location bindings_name = _.fresh({"bindings"});
            Location exprs_name = _.fresh({"expressions"});
            Location body_name = _.fresh({"body"});
            std::string func_name = rule_ident_to_string(_(Ident), "g0");
            Node path = rule_ident_to_path(_(Ident), "g0");

            size_t num_exprs = _(Query)->size();

            std::vector<std::pair<Location, Node>> exprs;
            for (Node resultexpr : *_(ResultExprSeq))
            {
              Location text = (resultexpr / IRString)->location();
              Node ref = resultexpr / Operand;
              if (text.len > 0)
              {
                text.len += 1;
                exprs.push_back(std::make_pair(text, ref));
              }
            }

            Node body_block = Block
              << (MakeArrayStmt << (Int32 ^ "1") << (LocalRef ^ result_name));
            for (Node literal : *_(Query))
            {
              add_literal_to_block(body_block, literal);
              if (literal->location().len == 0)
              {
                continue;
              }

              Node expr = literal / Expr;
              Location text = literal->location();
              text.len += 1;
              if (expr == OpBlock)
              {
                exprs.push_back(std::make_pair(text, expr / Operand));
              }
              else if (expr == AssignBlock)
              {
                exprs.push_back(
                  std::make_pair(text, Operand << (Boolean ^ "true")));
              }
            }

            std::sort(
              exprs.begin(), exprs.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.first.pos < rhs.first.pos;
              });

            Node inner = get_inner(body_block);
            inner
              << (MakeArrayStmt << (Int32 ^ std::to_string(exprs.size()))
                                << (LocalRef ^ exprs_name));
            for (auto [text, expr] : exprs)
            {
              Location expr_name = _.fresh({"expr"});
              Location loc_name = _.fresh({"loc"});
              Location row_name = _.fresh({"row"});
              Location col_name = _.fresh({"col"});
              size_t row = text.linecol().first + 1;
              size_t col = text.linecol().second + 1;
              inner << (MakeObjectStmt << (LocalRef ^ expr_name))
                    << (ObjectInsertStmt << (Operand << (IRString ^ "value"))
                                         << expr << (LocalRef ^ expr_name))
                    << (ObjectInsertStmt
                        << (Operand << (IRString ^ "text"))
                        << (Operand << (IRString ^ json::escape(text.view())))
                        << (LocalRef ^ expr_name))
                    << (MakeObjectStmt << (LocalRef ^ loc_name))
                    << (MakeNumberIntStmt << (Int64 ^ std::to_string(row))
                                          << (LocalRef ^ row_name))
                    << (MakeNumberIntStmt << (Int64 ^ std::to_string(col))
                                          << (LocalRef ^ col_name))
                    << (ObjectInsertStmt << (Operand << (IRString ^ "row"))
                                         << (Operand << (LocalRef ^ row_name))
                                         << (LocalRef ^ loc_name))
                    << (ObjectInsertStmt << (Operand << (IRString ^ "col"))
                                         << (Operand << (LocalRef ^ col_name))
                                         << (LocalRef ^ loc_name))
                    << (ObjectInsertStmt << (Operand << (IRString ^ "location"))
                                         << (Operand << (LocalRef ^ loc_name))
                                         << (LocalRef ^ expr_name))
                    << (ArrayAppendStmt << (LocalRef ^ exprs_name)
                                        << (Operand << (LocalRef ^ expr_name)));
            }

            inner << (MakeObjectStmt << (LocalRef ^ bindings_name));

            for (Node local : *_(LocalSeq))
            {
              Node binding = local->front();
              if (is_temporary(binding))
              {
                continue;
              }

              inner << (IsDefinedStmt << (LocalRef ^ binding))
                    << (ObjectInsertStmt << (Operand << (IRString ^ binding))
                                         << (Operand << (LocalRef ^ binding))
                                         << (LocalRef ^ bindings_name));
            }

            inner << (MakeObjectStmt << (LocalRef ^ body_name))
                  << (ObjectInsertStmt
                      << (Operand << (IRString ^ "bindings"))
                      << (Operand << (LocalRef ^ bindings_name))
                      << (LocalRef ^ body_name))
                  << (ObjectInsertStmt
                      << (Operand << (IRString ^ "expressions"))
                      << (Operand << (LocalRef ^ exprs_name))
                      << (LocalRef ^ body_name))
                  << (ArrayAppendStmt << (LocalRef ^ result_name)
                                      << (Operand << (LocalRef ^ body_name)));

            Node body = Body
              << (CompBlockSeq
                  << (OpBlock << (Operand << (LocalRef ^ result_name)) << Block)
                  << (BlockSeq << body_block));
            functions->emplace(
              func_name,
              (Function ^ _(Rule))
                << (IRString ^ func_name) << path
                << (ParameterSeq << param_input << param_data)
                << (LocalRef ^ return_name) << (BodySeq << body));

            return Rule << _(Ident) << (IRString ^ func_name)
                        << (False ^ "false");
          },

        T(Rule)[Rule]
            << (T(Ident) * T(LocalSeq) * T(True) *
                (T(RuleHead)
                 << T(RuleHeadObj,
                      RuleHeadObjDynamic,
                      RuleHeadSetDynamic,
                      RuleHeadSet,
                      RuleHeadQuery))) >>
          [](Match& _) { return err(_(Rule), "Invalid default rule"); },
      }};

    pass.pre([functions, builtinfunctions](Node) {
      functions->clear();
      builtinfunctions->clear();
      return 0;
    });

    pass.post([functions, builtinfunctions, builtins](Node top) {
      Node policy = top / RegoBundle / Policy;
      Node functionseq = NodeDef::create(FunctionSeq);
      for (auto [name, func] : *functions)
      {
        functionseq << func;
      }

      policy << functionseq;

      Node builtinfunctionseq = NodeDef::create(BuiltInFunctionSeq);
      for (const auto& name : *builtinfunctions)
      {
        if (builtins->is_builtin(name))
        {
          builtinfunctionseq
            << (BuiltInFunction << (IRString ^ name) << builtins->decl(name));
        }
        else
        {
          logging::Warn() << "'" << name
                          << "' is not the name of a registered built-in";
        }
      }

      policy / Static << builtinfunctionseq;
      return 0;
    });

    return pass;
  }

  inline const auto wf_ir_with_rules_stmts =
    wf_ir_lift_functions_stmts - WithCallStmt;

  // clang-format off
  inline const auto wf_ir_with_rules =
    wf_ir_lift_functions
    | (Block <<= wf_ir_with_rules_stmts++)
    | (CallStmt <<= (Func >>= Ref | IRString) * OperandSeq * LocalRef)
    ;
  // clang-format on

  struct WithInfo
  {
    std::string alias;
    Node withseq;
    std::map<std::string, Node> replacements;
    std::map<Location, Location> arguments;

    WithInfo update(Node scope, const std::string& alias, Node withseq) const
    {
      WithInfo info = WithInfo::create(scope, alias, withseq);
      info.withseq << *(withseq->clone());
      for (auto [key, val] : replacements)
      {
        if (info.replacements.find(key) == info.replacements.end())
        {
          info.replacements[key] = val;
        }
      }

      for (auto [_, val] : arguments)
      {
        info.arguments[val] = val;
      }

      return info;
    }

    static WithInfo create(Node scope, const std::string& alias, Node withseq)
    {
      WithInfo info = {alias, withseq};
      for (Node with : *withseq)
      {
        Node ref = with / Key;
        auto maybe_string = ref_to_string(ref);
        assert(maybe_string.has_value());
        std::string ref_key = maybe_string.value();
        if (with / Val == OpBlock)
        {
          Node head = (ref / RefHead)->front();
          assert(head == Var);
          if (head->location().view() != "data")
          {
            info.replacements[ref_key] = (with / Val)->clone();
            continue;
          }

          Node block = NodeDef::create(Block);
          Node argseq = ref / RefArgSeq;
          Location with_name("data");
          for (Node arg : *argseq)
          {
            assert(arg == RefArgDot);
            Node key = arg->front();
            Location child_name = scope->fresh({"with"});
            block
              << (DotStmt << (Operand << (LocalRef ^ with_name))
                          << (Operand << (IRString ^ key))
                          << (LocalRef ^ child_name));
            with_name = child_name;
          }

          info.replacements[ref_key] = OpBlock
            << (Operand << (LocalRef ^ with_name)) << block;
          continue;
        }

        if (with / Val == LocalRef)
        {
          Node val = with / Val;
          if (val->location().view().starts_with("witharg$"))
          {
            info.replacements[ref_key] = OpBlock << (Operand << val->clone())
                                                 << Block;
            continue;
          }

          Location witharg_name = scope->fresh({"witharg"});
          info.arguments[(with / Val)->location()] = witharg_name;
          info.replacements[ref_key] = OpBlock
            << (Operand << (LocalRef ^ witharg_name)) << Block;
          with / Val = (LocalRef ^ witharg_name);
          continue;
        }

        Node replace_ref = with / Val;
        if ((replace_ref / RefHead)->front()->location().view() == "data")
        {
          Node rule = lookup_rule(replace_ref);
          info.replacements[ref_key] = (rule / FuncName)->clone();
          continue;
        }

        maybe_string = ref_to_string(replace_ref);
        assert(maybe_string.has_value());
        std::string builtin = maybe_string.value();
        info.replacements[ref_key] = (IRString ^ builtin);
      }

      return info;
    }

    std::optional<Node> replacement(const std::string& rule) const
    {
      auto it = replacements.find(rule);
      if (it == replacements.end())
      {
        return std::nullopt;
      }

      return it->second->clone();
    }
  };

  bool contains_callstmt(Node function)
  {
    Nodes frontier({function / BodySeq});
    while (!frontier.empty())
    {
      Node current = frontier.back();
      frontier.pop_back();

      if (current->in({BuiltInCallStmt, CallStmt}))
      {
        return true;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }

    return false;
  }

  PassDef with_rules()
  {
    auto rule_templates = std::make_shared<std::map<Location, Node>>();
    auto with_info = std::make_shared<NodeMap<WithInfo>>();
    auto alias_count = std::make_shared<size_t>();
    PassDef pass = {
      "with_rules",
      wf_ir_with_rules,
      dir::topdown,
      {
        In(CallStmt) * (T(Ref)[Ref] * In(EntryPoint)++) >>
          [](Match& _) -> Node {
          Node rule = lookup_rule(_(Ref));
          if (rule == nullptr)
          {
            return IRString ^ withref_to_string(_(Ref));
          }

          return (rule / FuncName)->clone();
        },

        In(Block) *
            (T(CallStmt)[CallStmt]
             << (T(Ref)[Ref] * T(OperandSeq)[OperandSeq] *
                 T(LocalRef)[LocalRef])) *
            In(Function)++ >>
          [with_info](Match& _) -> Node {
          Node function = _(CallStmt)->parent(Function);

          Node rule = lookup_rule(_(Ref));
          if (with_info->empty())
          {
            Node funcname;
            if (rule == nullptr)
            {
              auto maybe_string = ref_to_string(_(Ref));
              if (!maybe_string.has_value())
              {
                return err(_(Ref), "Invalid rule reference");
              }

              funcname = IRString ^ maybe_string.value();
            }
            else
            {
              funcname = (rule / FuncName)->clone();
            }

            return CallStmt << funcname << _(OperandSeq) << _(LocalRef);
          }

          auto info_it = with_info->find(function);
          if (info_it == with_info->end())
          {
            // not in a with rule

            Node funcname;
            if (rule == nullptr)
            {
              auto maybe_string = ref_to_string(_(Ref));
              if (!maybe_string.has_value())
              {
                return err(_(Ref), "Invalid rule reference");
              }

              funcname = IRString ^ maybe_string.value();
            }
            else
            {
              funcname = (rule / FuncName)->clone();
            }

            return CallStmt << funcname << _(OperandSeq) << _(LocalRef);
          }

          auto& info = info_it->second;

          auto maybe_refstring = ref_to_string(_(Ref));
          if (!maybe_refstring.has_value())
          {
            return err(_(CallStmt), "Invalid function ref");
          }

          std::string refstring = maybe_refstring.value();

          auto maybe_replacement = info.replacement(refstring);
          if (!maybe_replacement.has_value())
          {
            // this call now inherits the withseq from the caller
            return WithCallStmt << _(Ref) << _(OperandSeq) << _(LocalRef)
                                << info.withseq->clone();
          }

          Node replacement = maybe_replacement.value()->clone();
          if (replacement == OpBlock)
          {
            return Seq << *(replacement / Block)
                       << (AssignVarStmt << replacement / Operand
                                         << _(LocalRef));
          }

          // func rule rewrite: we call the rewritten function
          assert(replacement == IRString);
          Node operandseq = _(OperandSeq)->clone();
          if (replacement->location().view()[0] != 'g')
          {
            // replacement is a builtin
            operandseq->erase(operandseq->begin(), operandseq->begin() + 2);
          }

          return CallStmt << replacement << operandseq << _(LocalRef)->clone();
        },

        In(Block) *
            (T(BuiltInCallStmt)[BuiltInCallStmt]
             << (T(IRString)[Func] * T(OperandSeq)[OperandSeq] *
                 T(LocalRef)[LocalRef])) *
            In(Function)++ >>
          [with_info, alias_count](Match& _) -> Node {
          Node function = _(BuiltInCallStmt)->parent(Function);
          if (with_info->empty())
          {
            return CallStmt << _(Func) << _(OperandSeq) << _(LocalRef);
          }

          auto info_it = with_info->find(function);
          if (info_it == with_info->end())
          {
            // not in a with rule
            return NoChange;
          }

          auto& info = info_it->second;
          std::string builtin(_(Func)->location().view());

          auto maybe_replacement = info.replacement(builtin);
          if (!maybe_replacement.has_value())
          {
            // no replacement
            return NoChange;
          }

          Node replacement = maybe_replacement.value()->clone();
          if (replacement == OpBlock)
          {
            return Seq << *(replacement / Block)
                       << (AssignVarStmt << replacement / Operand
                                         << _(LocalRef));
          }

          // func rule rewrite: we call the rewritten function
          assert(replacement == IRString);
          Node operandseq = _(OperandSeq)->clone();
          if (replacement->location().view()[0] == 'g')
          {
            // rule function
            operandseq = OperandSeq << (Operand << (LocalRef ^ "input"))
                                    << (Operand << (LocalRef ^ "data"))
                                    << *operandseq;
          }

          return CallStmt << replacement << operandseq << _(LocalRef)->clone();
        },

        In(Block) *
            (T(WithCallStmt)
             << (T(Ref)[Ref] * T(OperandSeq)[OperandSeq] *
                 T(LocalRef)[LocalRef] * T(WithSeq)[WithSeq])) *
            In(Function)++ >>
          [with_info, rule_templates, alias_count](Match& _) -> Node {
          Node rule = lookup_rule(_(Ref));
          if (rule == nullptr)
          {
            return err(_(Ref), "Invalid rule reference");
          }

          Node function = _(Ref)->parent(Function);
          WithInfo info;
          if (with_info->find(function) != with_info->end())
          {
            info = with_info->at(function);
          }

          Location funcname = (rule / FuncName)->location();
          if (rule_templates->find(funcname) == rule_templates->end())
          {
            return err(_(Ref), "Function does not exist");
          }

          Node aliased_function = rule_templates->at(funcname);

          auto maybe_string = ref_to_string(_(Ref));
          assert(maybe_string.has_value());
          std::string ref_string = maybe_string.value();
          if (!contains_callstmt(aliased_function))
          {
            return CallStmt << (aliased_function / Name)->clone()
                            << _(OperandSeq) << _(LocalRef);
          }

          aliased_function = aliased_function->clone();
          std::string name((aliased_function / Name)->location().view());
          *alias_count = (*alias_count) + 1;
          std::string alias_prefix = "g" + std::to_string(*alias_count);
          name = alias_prefix + name.substr(2);
          info = info.update(_(Ref)->scope(), name, _(WithSeq));
          aliased_function / Name = IRString ^ name;
          (aliased_function / IRPath)->replace_at(0, IRString ^ alias_prefix);
          Node operandseq = _(OperandSeq)->clone();
          for (auto& [local_name, arg_name] : info.arguments)
          {
            aliased_function / ParameterSeq << (LocalRef ^ arg_name);
            operandseq << (Operand << (LocalRef ^ local_name));
          }
          with_info->insert({aliased_function, info});

          return Seq << (CallStmt << (IRString ^ name) << operandseq
                                  << _(LocalRef))
                     << (Lift << FunctionSeq << aliased_function);
        },

        In(Block) * T(WithCallStmt)[WithCallStmt] >>
          [](Match& _) { return err(_(WithCallStmt), "Invalid with call"); },
      },
    };

    pass.pre([alias_count, rule_templates](Node top) {
      *alias_count = 0;
      rule_templates->clear();

      Node functionseq = top / RegoBundle / Policy / FunctionSeq;
      for (Node func : *functionseq)
      {
        rule_templates->insert({(func / Name)->location(), func->clone()});
      }

      return 0;
    });

    return pass;
  }

  const auto inline wf_ir_add_plans_stmts =
    (wf_ir_with_rules_stmts | ResultSetAddStmt) - BuiltInCallStmt;

  // clang-format off
  inline const auto wf_ir_add_plans =
    wf_ir_with_rules
    | (Policy <<= Static * PlanSeq * IRQuery * FunctionSeq)
    | (Function <<= (Name >>= IRString) * IRPath * ParameterSeq * (Return >>= LocalRef) * BlockSeq)[Name]
    | (PlanSeq <<= Plan++)
    | (Plan <<= IRString * BlockSeq)
    | (Block <<= wf_ir_add_plans_stmts++)
    | (ResultSetAddStmt <<= LocalRef)
    | (CallStmt <<= (Func >>= IRString) * OperandSeq * LocalRef)
    ;
  // clang-format on

  void check_operand_is_defined(Node block, Node operand)
  {
    Node arg = operand->front();
    if (arg != LocalRef)
    {
      return;
    }

    const std::string_view& name = arg->location().view();
    if (name.starts_with("number$") || name.starts_with("null$"))
    {
      return;
    }

    block << (IsDefinedStmt << arg->clone());
  }

  Node bodyset_block(Node body, Node return_ref)
  {
    assert(body == BodySet);
    assert(body->size() == 1);
    Node bodyinstance = body->at(0);
    Node blockseq = bodyinstance / BlockSeq;

    if (blockseq->empty())
    {
      return err(blockseq, "Empty body");
    }

    Node ruleset_seq = NodeDef::create(BlockSeq);

    for (size_t j = 0; j < blockseq->size(); ++j)
    {
      Node value_opblock = (bodyinstance / Val)->clone();
      Node block = Block << *(value_opblock / Block) << *blockseq->at(j);
      Node value_op = value_opblock / Operand;
      Node inner = get_inner(block);
      check_operand_is_defined(inner, value_op);
      inner << (SetAddStmt << value_op << return_ref->clone());
      ruleset_seq << block;
    }

    if (ruleset_seq->empty())
    {
      return err(body, "Invalid rule set body");
    }

    return Block << (BlockStmt << ruleset_seq);
  }

  Node bodysetdynamic_block(Node body, Node return_ref)
  {
    assert(body == BodySetDynamic);
    assert(body->size() == 1);

    Node bodyinstance = body->at(0);
    Node blockseq = bodyinstance / BlockSeq;

    if (blockseq->empty())
    {
      return err(blockseq, "Empty body");
    }

    Node bodyinstance_blockseq = NodeDef::create(BlockSeq);
    for (size_t i = 0; i < blockseq->size(); ++i)
    {
      Node path_opblockseq = (bodyinstance / Path);
      Node value_opblock = (bodyinstance / Val)->clone();
      Node block = Block << *(value_opblock / Block);
      for (Node pathpart_opblock : *path_opblockseq)
      {
        block << *(pathpart_opblock->clone() / Block);
      }

      block << *blockseq->at(i);

      Node value_op = value_opblock / Operand;

      Node inner = get_inner(block);
      check_operand_is_defined(inner, value_opblock);

      // we first need to create the (potentially nested) object structure
      // which holds the dynamic set
      std::vector<Location> name_stack({return_ref->location()});
      for (size_t j = 0; j < path_opblockseq->size() - 1; ++j)
      {
        Node pathpart_opblock = path_opblockseq->at(j)->clone();
        Node pathpart_op = pathpart_opblock / Operand;
        check_operand_is_defined(inner, pathpart_opblock);

        Location child_name = body->fresh({"child"});
        inner
          << (BlockStmt
              << (BlockSeq
                  << (Block << (ResetLocalStmt << (LocalRef ^ child_name)))
                  << (Block
                      << (DotStmt << (Operand << (LocalRef ^ name_stack.back()))
                                  << pathpart_op << (LocalRef ^ child_name)))
                  << (Block << (IsUndefinedStmt << (LocalRef ^ child_name))
                            << (MakeObjectStmt << (LocalRef ^ child_name)))));
        name_stack.push_back(child_name);
      }

      // the final element in the path is the key for the set
      Location set_name = body->fresh({"set"});
      Node key_opblock = path_opblockseq->back()->clone();
      Node key_op = key_opblock / Operand;
      check_operand_is_defined(inner, key_opblock);
      inner << (BlockStmt
                << (BlockSeq
                    << (Block << (ResetLocalStmt << (LocalRef ^ set_name)))
                    << (Block
                        << (DotStmt
                            << (Operand << (LocalRef ^ name_stack.back()))
                            << key_op << (LocalRef ^ set_name)))
                    << (Block << (IsUndefinedStmt << (LocalRef ^ set_name))
                              << (MakeSetStmt << (LocalRef ^ set_name)))))
            << (SetAddStmt << value_op << (LocalRef ^ set_name))
            << (ObjectInsertStmt << key_op->clone()
                                 << (Operand << (LocalRef ^ set_name))
                                 << (LocalRef ^ name_stack.back()));

      // now we need to walk back up the structure updating as we go
      for (int j = static_cast<int>(path_opblockseq->size()) - 2; j >= 0; --j)
      {
        Location child_name = name_stack.back();
        name_stack.pop_back();
        Node pathpart_op = path_opblockseq->at(j) / Operand;
        inner
          << (ObjectInsertStmt << pathpart_op->clone()
                               << (Operand << (LocalRef ^ child_name))
                               << (LocalRef ^ name_stack.back()));
      }

      bodyinstance_blockseq << block;
    }

    if (bodyinstance_blockseq->empty())
    {
      return err(body, "Invalid rule body");
    }

    return Block << (BlockStmt << bodyinstance_blockseq);
  }

  Node bodyobjdynamic_block(Node body, Node return_ref)
  {
    assert(body == BodyObjDynamic);
    assert(body->size() == 1);

    Node bodyinstance = body->at(0);
    Node blockseq = bodyinstance / BlockSeq;

    if (blockseq->empty())
    {
      return err(blockseq, "Empty body");
    }

    Node bodyinstance_blockseq = NodeDef::create(BlockSeq);
    for (size_t i = 0; i < blockseq->size(); ++i)
    {
      Node path_opblockseq = (bodyinstance / Path);
      Node value_opblock = (bodyinstance / Val)->clone();
      Node block = Block << *(value_opblock / Block);
      for (Node pathpart_opblock : *path_opblockseq)
      {
        block << *(pathpart_opblock->clone() / Block);
      }

      block << *blockseq->at(i);

      Node value_op = value_opblock / Operand;

      Node inner = get_inner(block);
      check_operand_is_defined(inner, value_opblock);

      // we first need to create the (potentially nested) object structure
      // which holds the dynamic set
      std::vector<Location> name_stack({return_ref->location()});
      for (size_t j = 0; j < path_opblockseq->size() - 1; ++j)
      {
        Node pathpart_opblock = path_opblockseq->at(j)->clone();
        Node pathpart_op = pathpart_opblock / Operand;
        check_operand_is_defined(inner, pathpart_opblock);

        Location child_name = body->fresh({"child"});
        inner
          << (BlockStmt
              << (BlockSeq
                  << (Block << (ResetLocalStmt << (LocalRef ^ child_name)))
                  << (Block
                      << (DotStmt << (Operand << (LocalRef ^ name_stack.back()))
                                  << pathpart_op << (LocalRef ^ child_name)))
                  << (Block << (IsUndefinedStmt << (LocalRef ^ child_name))
                            << (MakeObjectStmt << (LocalRef ^ child_name)))));
        name_stack.push_back(child_name);
      }

      // the final element is the key
      Node key_opblock = path_opblockseq->back()->clone();
      Node key_op = key_opblock / Operand;
      check_operand_is_defined(inner, key_opblock);
      inner
        << (ObjectInsertOnceStmt << key_op->clone() << value_op
                                 << (LocalRef ^ name_stack.back()));

      // now we need to walk back up the structure updating as we go
      for (int j = static_cast<int>(path_opblockseq->size()) - 2; j >= 0; --j)
      {
        Location child_name = name_stack.back();
        name_stack.pop_back();
        Node pathpart_op = path_opblockseq->at(j) / Operand;
        inner
          << (ObjectInsertStmt << pathpart_op->clone()
                               << (Operand << (LocalRef ^ child_name))
                               << (LocalRef ^ name_stack.back()));
      }

      bodyinstance_blockseq << block;
    }

    if (bodyinstance_blockseq->empty())
    {
      return err(body, "Invalid rule body");
    }

    return Block << (BlockStmt << bodyinstance_blockseq);
  }

  Node _body_block_add(
    Node dst_block,
    Node src_block,
    Node value_opblock,
    Location value_name,
    bool check_undefined)
  {
    if (check_undefined)
    {
      dst_block << (IsUndefinedStmt << (LocalRef ^ value_name));
    }

    dst_block << *(value_opblock / Block) << *src_block;
    Node value_op = value_opblock / Operand;
    Node inner = get_inner(dst_block);
    check_operand_is_defined(inner, value_op);
    inner << (AssignVarOnceStmt << value_op << (LocalRef ^ value_name));
    return dst_block;
  }

  Node body_block(Node body, Node return_ref)
  {
    assert(body == Body);

    Node ret_blockseq = NodeDef::create(BlockSeq);

    Location bodyval_name = return_ref->location();
    if (body->size() > 1)
    {
      bodyval_name = body->fresh({"bodyval"});
      ret_blockseq << (Block << (ResetLocalStmt << (LocalRef ^ bodyval_name)));
    }

    for (size_t i = 0; i < body->size(); ++i)
    {
      Node bodyinstance = body->at(i);
      Node blockseq = bodyinstance / BlockSeq;

      if (blockseq->empty())
      {
        ret_blockseq << err(blockseq, "Empty body");
        continue;
      }

      Node bodyinstance_block = NodeDef::create(Block);
      if (i > 0)
      {
        bodyinstance_block << (IsUndefinedStmt << (LocalRef ^ bodyval_name));
      }

      if (blockseq->size() == 1)
      {
        _body_block_add(
          bodyinstance_block,
          blockseq->front(),
          (bodyinstance / Val)->clone(),
          bodyval_name,
          false);
      }
      else
      {
        Node bodyinstance_blockseq = NodeDef::create(BlockSeq);
        for (size_t j = 0; j < blockseq->size(); ++j)
        {
          bodyinstance_blockseq << _body_block_add(
            NodeDef::create(Block),
            blockseq->at(j),
            (bodyinstance / Val)->clone(),
            bodyval_name,
            j > 0);
        }

        if (bodyinstance_blockseq->empty())
        {
          bodyinstance_block << err(bodyinstance, "Invalid rule body");
        }
        else
        {
          bodyinstance_block << (BlockStmt << bodyinstance_blockseq);
        }
      }

      ret_blockseq << bodyinstance_block;
    }

    if (body->size() > 1)
    {
      ret_blockseq
        << (Block << (IsDefinedStmt << (LocalRef ^ bodyval_name))
                  << (AssignVarOnceStmt
                      << (Operand << (LocalRef ^ bodyval_name))
                      << return_ref->clone()));
    }

    if (ret_blockseq->empty())
    {
      return err(body, "Invalid rule body");
    }

    return Block << (BlockStmt << ret_blockseq);
  }

  Node bodyobjstatic_block(
    Node body, std::string bodyobjval_name, Node return_ref)
  {
    // one key, but the value assigned may require the evaluation of
    // one or more else statements. This means more than one body, and
    // each of those bodies may have more than one block. Thus, we create
    // a local to store the value, assign to it (as for a comp or func rule)
    // and then add it to the object.
    assert(body == BodyObjStatic);
    Node key_opblock = body / Key;
    Node ret_blockseq = BlockSeq << key_opblock / Block;

    Node objvalseq = body / ObjValSeq;
    for (size_t i = 0; i < objvalseq->size(); ++i)
    {
      Node bodyinstance = objvalseq->at(i);
      Node blockseq = bodyinstance / BlockSeq;

      if (blockseq->empty())
      {
        ret_blockseq << err(blockseq, "Empty body");
        continue;
      }

      Node bodyinstance_block = NodeDef::create(Block);
      if (i > 0)
      {
        bodyinstance_block << (IsUndefinedStmt << (LocalRef ^ bodyobjval_name));
      }

      if (blockseq->size() == 1)
      {
        _body_block_add(
          bodyinstance_block,
          blockseq->front(),
          (bodyinstance / Val)->clone(),
          bodyobjval_name,
          false);
      }
      else
      {
        Node bodyinstance_blockseq = NodeDef::create(BlockSeq);
        for (size_t j = 0; j < blockseq->size(); ++j)
        {
          bodyinstance_blockseq << _body_block_add(
            NodeDef::create(Block),
            blockseq->at(j),
            (bodyinstance / Val)->clone(),
            bodyobjval_name,
            j > 0);
        }

        if (bodyinstance_blockseq)
        {
          bodyinstance_block << err(bodyinstance, "Invalid rule body");
        }
        else
        {
          bodyinstance_block << (BlockStmt << bodyinstance_blockseq);
        }
      }
      ret_blockseq << bodyinstance_block;
    }

    ret_blockseq
      << (Block << (IsDefinedStmt << (LocalRef ^ bodyobjval_name))
                << (ObjectInsertOnceStmt
                    << (key_opblock / Operand)
                    << (Operand << (LocalRef ^ bodyobjval_name))
                    << return_ref->clone()));

    return Block << (BlockStmt << ret_blockseq);
  }

  PassDef add_plans(const BuiltIns& builtins)
  {
    PassDef pass = {
      "add_plans",
      wf_ir_add_plans,
      dir::bottomup | dir::once,
      {
        In(CallStmt) * T(Ref)[Ref] >>
          [](Match& _) {
            Node rule = lookup_rule(_(Ref));
            if (rule == nullptr)
            {
              auto maybe_string = ref_to_string(_(Ref));
              if (maybe_string.has_value())
              {
                return IRString ^ maybe_string.value();
              }

              return err(_(Ref), "Invalid function reference");
            }

            return (rule / FuncName)->clone();
          },

        T(BuiltInCallStmt)
            << (T(IRString)[IRString] * T(OperandSeq)[OperandSeq] *
                T(LocalRef)[LocalRef]) >>
          [builtins](Match& _) {
            if (builtins->is_builtin(_(IRString)->location()))
            {
              return CallStmt << _(IRString) << _(OperandSeq) << _(LocalRef);
            }

            return BuiltInCallStmt << err(_(IRString), "BuiltIn does not exist")
                                   << _(OperandSeq) << _(LocalRef);
          },

        In(Function) * (T(LocalRef)[LocalRef] * T(BodySeq)[BodySeq]) >>
          [](Match& _) {
            Location loc = _(LocalRef)->parent()->location();
            Node return_ref = _(LocalRef);
            Node def;

            Node start_block = Block
              << ((ResetLocalStmt ^ loc) << return_ref->clone());

            Node base_ref;
            Node bodyseq = _(BodySeq);
            std::string bodyobj_prefix;
            std::set<std::string> bodyobjval_names;
            if (bodyseq->contains(BodyObj))
            {
              bodyobj_prefix = std::string(_.fresh({"bodyobj"}).view());
              if (bodyseq->front() == BodyData)
              {
                Node bodydata = bodyseq->front();
                Node doc_assign = bodydata->front();
                start_block << *(doc_assign / Block)
                            << ((AssignVarStmt ^ loc)
                                << (Operand << doc_assign / LocalRef)
                                << return_ref->clone());
                bodyseq->replace_at(0);
              }
              else
              {
                start_block << ((MakeObjectStmt ^ loc) << return_ref->clone());
              }

              for (size_t i = 0; i < bodyseq->size(); ++i)
              {
                if (bodyseq->at(i) != BodyObj)
                {
                  return err(
                    bodyseq->at(i), "Invalid rule body for object rule");
                }
              }
            }
            else if (bodyseq->contains(BodySet))
            {
              if (bodyseq->front() == BodyData)
              {
                Node bodydata = bodyseq->front();
                Node doc_assign = bodydata->front();
                start_block << *(doc_assign / Block)
                            << ((AssignVarStmt ^ loc)
                                << (Operand << doc_assign / LocalRef)
                                << return_ref->clone());
                bodyseq->replace_at(0);
              }
              else
              {
                start_block << ((MakeSetStmt ^ loc) << return_ref->clone());
              }

              for (size_t i = 0; i < bodyseq->size(); ++i)
              {
                if (bodyseq->at(i) != BodySet)
                {
                  return err(bodyseq->at(i), "Invalid rule body for set rule");
                }
              }
            }

            Node func_blockseq = BlockSeq << start_block;
            for (Node body : *bodyseq)
            {
              if (body == Default)
              {
                if (def == nullptr)
                {
                  def = body;
                  continue;
                }
                else
                {
                  return err(body, "Multiple default rules");
                }
              }

              if (body == Body)
              {
                func_blockseq << body_block(body, return_ref);
                continue;
              }

              if (body == BodySet)
              {
                func_blockseq << bodyset_block(body, return_ref);
                continue;
              }

              if (body == BodyObj)
              {
                body = body->front();
                if (body == BodyObjStatic)
                {
                  std::string ident((body / Ident)->location().view());
                  std::string bodyobjval_name =
                    bodyobj_prefix + "#" + strip_quotes(ident);
                  if (
                    bodyobjval_names.find(bodyobjval_name) ==
                    bodyobjval_names.end())
                  {
                    func_blockseq
                      << (Block
                          << ((ResetLocalStmt ^ loc)
                              << (LocalRef ^ bodyobjval_name)));
                    bodyobjval_names.insert(bodyobjval_name);
                  }

                  func_blockseq
                    << bodyobjstatic_block(body, bodyobjval_name, return_ref);
                  continue;
                }

                if (body == BodyObjDynamic)
                {
                  func_blockseq << bodyobjdynamic_block(body, return_ref);
                  continue;
                }

                if (body == BodySetDynamic)
                {
                  func_blockseq << bodysetdynamic_block(body, return_ref);
                  continue;
                }

                return err(body, "Unsupported bodyobj type");
              }

              return err(body, "Unsupported body type");
            }

            if (def)
            {
              Node opblock = def->front();
              func_blockseq
                << (Block << ((IsUndefinedStmt ^ loc) << return_ref->clone())
                          << *(opblock / Block)
                          << ((AssignVarOnceStmt ^ loc)
                              << (opblock / Operand) << return_ref->clone()));
            }

            func_blockseq
              << (Block << ((IsDefinedStmt ^ loc) << return_ref->clone())
                        << ((ReturnLocalStmt ^ loc) << return_ref->clone()));
            return Seq << _(LocalRef) << func_blockseq;
          },

        In(EntryPointSeq) *
            (T(EntryPoint) << (T(Ident)[Ident] * T(OpBlock)[OpBlock])) >>
          [](Match& _) -> Node {
          Location obj_name = _.fresh({"obj"});
          Node opblock = _(OpBlock);
          Node blockseq = BlockSeq << opblock / Block;
          Node value_ref = (opblock / Operand)->front();
          if (value_ref != LocalRef)
          {
            return err(value_ref, "Invalid entrypoint opblock");
          }

          if (_(Ident)->location().view().starts_with("querymodule$"))
          {
            Location index_name = _.fresh({"scanindex"});
            Location value_name = _.fresh({"scanvalue"});
            blockseq
              << (Block << (IsDefinedStmt << value_ref->clone())
                        << (ScanStmt
                            << value_ref->clone() << (LocalRef ^ index_name)
                            << (LocalRef ^ value_name)
                            << (Block
                                << (MakeObjectStmt << (LocalRef ^ obj_name))
                                << (ObjectInsertStmt
                                    << (Operand << (IRString ^ "result"))
                                    << (Operand << (LocalRef ^ value_name))
                                    << (LocalRef ^ obj_name))
                                << (ResultSetAddStmt
                                    << (LocalRef ^ obj_name)))));
          }
          else
          {
            blockseq
              << (Block << (MakeObjectStmt << (LocalRef ^ obj_name))
                        << (IsDefinedStmt << value_ref->clone())
                        << (ObjectInsertStmt
                            << (Operand << (IRString ^ "result"))
                            << (Operand << value_ref->clone())
                            << (LocalRef ^ obj_name))
                        << (ResultSetAddStmt << (LocalRef ^ obj_name)));
          }

          return Plan << (IRString ^ _(Ident)) << blockseq;
        },

        In(Policy) *
            (T(Static)[Static] * T(EntryPointSeq)[EntryPointSeq] *
             T(IRQuery)[IRQuery] * T(FunctionSeq)[FunctionSeq]) >>
          [](Match& _) {
            return Seq << _(Static) << (PlanSeq << *_[EntryPointSeq])
                       << _(IRQuery) << _(FunctionSeq);
          },
      }};

    return pass;
  }

  PassDef index_strings_locals()
  {
    auto func_lookup =
      std::make_shared<NodeMap<std::map<Location, Location>>>();
    auto locals = std::make_shared<std::map<Location, size_t>>();
    auto strings = std::make_shared<std::map<Location, size_t>>();
    // replace all local refs with their indices in the frame
    PassDef pass = {
      "index_strings_locals",
      wf_bundle,
      dir::bottomup | dir::once,
      {
        In(Operand) * T(IRString)[IRString] >>
          [strings](Match& _) {
            Location loc = _(IRString)->location();
            size_t index;
            auto it = strings->find(loc);
            if (it == strings->end())
            {
              index = strings->size();
              (*strings)[loc] = index;
            }
            else
            {
              index = it->second;
            }

            return StringIndex ^ std::to_string(index);
          },

        In(StringSeq) * T(IRString)[IRString] * In(WithStmt)++ >>
          [strings](Match& _) {
            Location loc = _(IRString)->location();
            size_t index;
            auto it = strings->find(loc);
            if (it == strings->end())
            {
              index = strings->size();
              (*strings)[loc] = index;
            }
            else
            {
              index = it->second;
            }

            return Int32 ^ std::to_string(index);
          },

        In(WithStmt) * T(StringSeq)[StringSeq] >>
          [](Match& _) { return Int32Seq << *_[StringSeq]; },

        In(MakeNumberRefStmt) * T(IRString)[IRString] >>
          [strings](Match& _) {
            Location loc = _(IRString)->location();
            size_t index;
            if (loc.len > 0)
            {
              auto it = strings->find(loc);
              if (it == strings->end())
              {
                index = strings->size();
                (*strings)[loc] = index;
              }
              else
              {
                index = it->second;
              }
            }

            return Int32 ^ std::to_string(index);
          },

        In(Policy) * T(Static)[Static] >>
          [strings](Match& _) {
            return Static << to_stringseq(StringSeq, *strings) << *_[Static];
          },

        T(LocalRef, "input") >> [](Match& _) { return LocalIndex ^ "0"; },

        T(LocalRef, "data") >> [](Match& _) { return LocalIndex ^ "1"; },

        T(LocalRef)[LocalRef] >>
          [locals, func_lookup](Match& _) {
            Location name = _(LocalRef)->location();
            if (name.view().find('$') == std::string::npos)
            {
              Node function = _(LocalRef)->parent(Function);
              if (func_lookup->find(function) == func_lookup->end())
              {
                func_lookup->insert({function, {}});
              }

              auto& lookup = func_lookup->at(function);
              if (lookup.find(name) == lookup.end())
              {
                Location unique_name = _.fresh(name);
                lookup[name] = unique_name;
              }

              name = lookup[name];
            }

            auto it = locals->find(name);
            if (it == locals->end())
            {
              size_t index = locals->size();
              (*locals)[name] = index;
              return LocalIndex ^ std::to_string(index);
            }
            else
            {
              return LocalIndex ^ std::to_string(it->second);
            }
          },

        T(DataTerm)[Term] >> [](Match& _) { return Term << _(Term)->front(); },

        T(BaseObjectItem)[BaseObjectItem] >>
          [](Match& _) {
            Node key = Term
              << (Scalar << (JSONString ^ _(BaseObjectItem) / Ident));
            return ObjectItem << key << _(BaseObjectItem)->back();
          },

        T(DataArray)[Array] >> [](Match& _) { return Array << *_[Array]; },

        T(DataSet)[Set] >> [](Match& _) { return Set << *_[Set]; },

        T(BaseObject)[Object] >> [](Match& _) { return Object << *_[Object]; },

        T(PlanSeq)[PlanSeq] << End >>
          [](Match& _) { return err(_(PlanSeq), "No plans in policy bundle"); },

        In(RegoBundle) * (T(BaseDocument) << (T(Ident) * T(Object)[Data])) *
            T(VirtualDocument) * T(VirtualDocument, Undefined) *
            T(Policy)[Policy] >>
          [](Match& _) { return Seq << (Data << _(Data)) << _(Policy); },
      }};

    pass.pre([strings, locals, func_lookup](Node) {
      strings->clear();
      locals->clear();
      func_lookup->clear();
      locals->insert({{"input"}, 0});
      locals->insert({{"data"}, 0});
      return 0;
    });

    return pass;
  }
}

namespace rego
{
  Rewriter rego_to_bundle(BuiltIns builtins)
  {
    return {
      "rego_to_bundle",
      {refheads(),
       rules(),
       locals(),
       implicit_scans(),
       merge(),
       unify(builtins),
       expr_to_opblock(builtins),
       lift_functions(builtins),
       with_rules(),
       add_plans(builtins),
       index_strings_locals()},
      wf_bundle_input};
  }
}