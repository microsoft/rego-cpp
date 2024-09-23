#include "unify.hh"

namespace
{
  using namespace rego;

  using Locs = std::set<Location>;
  using LocMap = std::map<Location, Location>;

  bool is_captured(Node unifybody, Node var)
  {
    Nodes defs = var->lookup();
    if (defs.size() != 1)
    {
      return false;
    }

    Node local = defs[0];

    if (local->type() != Local && local->type() != ArgVar)
    {
      return false;
    }

    return var->common_parent(local) != unifybody;
  }

  void add_captures(Node body, Node node, Locs& locs)
  {
    if (node->type() == RefArgDot || node->type() == NestedBody)
    {
      return;
    }

    if (node->type() == Var)
    {
      if (is_captured(body, node))
      {
        locs.insert(node->location());
      }
    }
    else
    {
      for (Node child : *node)
      {
        add_captures(body, child, locs);
      }
    }
  }

  void find_invars(Node unifybody, Locs& invars)
  {
    Locs locals;
    // input vars will be rvalues (i.e. in the Val node)
    for (auto unifyexpr : *unifybody)
    {
      if (unifyexpr->type() == Local)
      {
        locals.insert((unifyexpr / Var)->location());
      }
      else if (unifyexpr->type() == UnifyExpr)
      {
        add_captures(unifybody, unifyexpr / Val, invars);
      }
      else if (unifyexpr->type() == UnifyExprNot)
      {
        find_invars(unifyexpr / UnifyBody, invars);
      }
      else if (unifyexpr->type() == UnifyExprWith)
      {
        find_invars(unifyexpr / UnifyBody, invars);
      }
    }

    // remove locals (they may be incorrectly marked as captured by nested
    // bodies)
    for (auto& loc : locals)
    {
      invars.erase(loc);
    }
  }

  void find_outvars(Node unifybody, Locs& outvars)
  {
    Locs locals;
    // output vars will consist of lvalues
    for (auto unifyexpr : *unifybody)
    {
      if (unifyexpr->type() == Local)
      {
        locals.insert((unifyexpr / Var)->location());
      }
      else if (unifyexpr->type() == UnifyExpr)
      {
        Node var = unifyexpr / Var;
        if (is_captured(unifybody, unifyexpr / Var))
        {
          outvars.insert(var->location());
        }
      }
      else if (unifyexpr->type() == UnifyExprNot)
      {
        find_outvars(unifyexpr / UnifyBody, outvars);
      }
      else if (unifyexpr->type() == UnifyExprWith)
      {
        find_outvars(unifyexpr / UnifyBody, outvars);
      }
    }

    // remove locals (they may be incorrectly marked as captured by nested
    // bodies)
    for (auto& loc : locals)
    {
      outvars.erase(loc);
    }
  }

  void replace(Node node, const LocMap& lookup)
  {
    if (node->type() == RefArgDot)
    {
      return;
    }

    if (node->type() == Var && contains(lookup, node->location()))
    {
      node->parent()->replace(node, Var ^ lookup.at(node->location()));
    }
    else
    {
      for (Node child : *node)
      {
        replace(child, lookup);
      }
    }
  }

  Node get_version(NodeDef* node)
  {
    if (node == Top)
    {
      return nullptr;
    }

    if (node->in({Module, RuleComp, RuleFunc, RuleObj, RuleSet}))
    {
      auto it = node->find_first(Version, node->begin());
      if (it != node->end())
      {
        return (*it)->clone();
      }

      return err(node->intrusive_ptr_from_this(), "Missing version");
    }

    return get_version(node->parent_unsafe());
  }
}

namespace rego
{
  // This pass lifts enumerations, comprehensions, and every expressions into
  // rules and replaces them with ExprCalls.
  PassDef lift_to_rule()
  {
    return {
      "lift_to_rule",
      wf_pass_lift_to_rule,
      dir::bottomup,
      {
        In(UnifyBody) *
            ((T(UnifyExprEnum, UnifyExprWalk)[Op] * In(DataModule)++)
             << (T(Var)[Var] * T(Var)[Item] * T(Var)[ItemSeq] *
                 T(UnifyBody)[UnifyBody])) >>
          [](Match& _) {
            ACTION();
            Node version = get_version(_(UnifyBody)->parent_unsafe());
            Node rulebody = _(UnifyBody);
            // in vars
            Locs invars;
            find_invars(rulebody, invars);
            // out vars
            Locs outvars;
            find_outvars(rulebody, outvars);

            LocMap out_map;
            for (auto& loc : outvars)
            {
              // each out variable gets a new name. This makes
              // in/out variables easier to manage.
              Location out_loc = _.fresh({"out"});
              out_map[loc] = out_loc;
            }

            // replace all references to the return values
            // with their new locations.
            replace(rulebody, out_map);

            // create the out variables
            for (auto& [var, out_var] : out_map)
            {
              if (contains(invars, var))
              {
                // we don't want return values to be passed in as arguments.
                // i.e. we implicitly disable an in/out pattern, as the
                // values which are are returned from this function will
                // be merged with the variables in the outer unification via
                // a different mechanism.
                invars.erase(var);
              }
              rulebody->push_front(Local << (Var ^ out_var) << Undefined);
            }

            // create the arguments for the rule and call
            Node ruleargs = NodeDef::create(RuleArgs);
            Node exprseq = NodeDef::create(ExprSeq);
            for (auto& var : invars)
            {
              ruleargs << (ArgVar << (Var ^ var) << Undefined);
              exprseq << (Expr << (RefTerm << (Var ^ var)));
            }

            Node rulename = Var ^ _.fresh({"enum"});
            Node rulevalue;
            if (out_map.empty())
            {
              // no outputs. We just return true.
              rulevalue = DataTerm << (Scalar << True);
            }
            else
            {
              // embed the outputs in an object.
              rulevalue = NodeDef::create(Object);
              for (auto& [var, out_var] : out_map)
              {
                rulevalue
                  << (ObjectItem
                      << (Expr << (Term << (Scalar << (JSONString ^ var))))
                      << (Expr << (RefTerm << (Var ^ out_var))));
              }
              Location value = _.fresh({"value"});
              rulevalue = UnifyBody
                << (Local << (Var ^ value) << Undefined)
                << (UnifyExpr << (Var ^ value)
                              << (Expr << (Term << rulevalue)));
            }

            Token func = _(Op)->type() == UnifyExprEnum ? Enumerate : Walk;
            Node result = Seq
              << (Lift << DataModule
                       << (RuleFunc << rulename << ruleargs << rulebody
                                    << rulevalue << version << (Int ^ "0")))
              << (UnifyExpr
                  << _(Item)
                  << (Expr << (func << (Expr << (RefTerm << _(ItemSeq))))))
              << (UnifyExpr
                  << _(Var)
                  << (Expr << (ExprCall << rulename->clone() << exprseq)));
            for (auto var : outvars)
            {
              // we need to unify the results with the variables in the source
              // problem
              result
                << (UnifyExpr
                    << (Var ^ var)
                    << (Expr
                        << (RefTerm
                            << (SimpleRef << (Var ^ _(Var))
                                          << (RefArgDot << (Var ^ var))))));
            }
            return result;
          },

        In(UnifyBody) *
            ((T(UnifyExprCompr) * In(DataModule)++)
             << (T(Var)[Var] *
                 (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] *
                 (T(NestedBody) << (T(Key)[Key] * T(UnifyBody)[UnifyBody])))) >>
          [](Match& _) {
            ACTION();
            Node version = get_version(_(Var)->parent_unsafe());
            Node rulebody = _(UnifyBody);
            Locs invars;
            find_invars(_(UnifyBody), invars);

            Node default_value;
            if (_(Compr)->type() == ArrayCompr)
            {
              default_value = DataTerm << DataArray;
            }
            else if (_(Compr)->type() == SetCompr)
            {
              default_value = DataTerm << DataSet;
            }
            else
            {
              default_value = DataTerm << DataObject;
            }

            Node rulename = Var ^ _(Key);
            Location value = _.fresh({"value"});
            if (invars.empty())
            {
              // no invars. This can be expressed as a RuleComp.
              Node rulevalue = UnifyBody
                << (Local << (Var ^ value) << Undefined)
                << (UnifyExpr
                    << (Var ^ value)
                    << (Expr << (_(Compr)->type() << _(Compr) / Var)));
              return Seq << (Lift
                             << DataModule
                             << (RuleComp << rulename << rulebody << rulevalue
                                          << version->clone() << (Int ^ "0")))
                         << (Lift
                             << DataModule
                             << (RuleComp << rulename->clone() << Empty
                                          << default_value << version->clone()
                                          << (Int ^ "1")))
                         << (UnifyExpr
                             << _(Var)
                             << (Expr << (RefTerm << rulename->clone())));
            }
            else
            {
              // similar to Enum above, but with a single known output
              Node ruleargs = NodeDef::create(RuleArgs);
              Node exprseq = NodeDef::create(ExprSeq);
              for (auto& var : invars)
              {
                ruleargs << (ArgVar << (Var ^ var) << Undefined);
                exprseq << (Expr << (RefTerm << (Var ^ var)));
              }

              Node rulevalue = UnifyBody
                << (Local << (Var ^ value) << Undefined)
                << (UnifyExpr
                    << (Var ^ value)
                    << (Expr << (_(Compr)->type() << _(Compr) / Var)));

              Location partial = _.fresh({"partial"});
              return Seq
                << (Lift << DataModule
                         << (RuleFunc << rulename << ruleargs << rulebody
                                      << rulevalue << version->clone()
                                      << (Int ^ "0")))
                << (Lift << DataModule
                         << (RuleFunc << rulename->clone() << ruleargs->clone()
                                      << Empty << default_value
                                      << version->clone() << (Int ^ "1")))
                << (Local << (Var ^ partial) << Undefined)
                << (UnifyExpr
                    << (Var ^ partial)
                    << (Expr << (ExprCall << rulename->clone() << exprseq)))
                << (UnifyExpr << _(Var)
                              << (Expr << (Merge << (Var ^ partial))));
            }
          },

        In(Expr) *
            ((T(ExprEvery) * In(DataModule)++) << T(UnifyBody)[UnifyBody]) >>
          [](Match& _) {
            ACTION();
            Node version = get_version(_(UnifyBody)->parent_unsafe());
            Node rulebody = _(UnifyBody);
            Locs invars;
            find_invars(_(UnifyBody), invars);

            Location rulename = _.fresh({"every"});
            Node rulevalue = DataTerm << (Scalar << (True ^ "true"));
            if (invars.empty())
            {
              return Seq << (Lift << DataModule
                                  << (RuleComp << (Var ^ rulename) << rulebody
                                               << rulevalue << version
                                               << (Int ^ "0")))
                         << (RefTerm << (Var ^ rulename));
            }

            Node ruleargs = NodeDef::create(RuleArgs);
            Node exprseq = NodeDef::create(ExprSeq);
            for (auto& var : invars)
            {
              ruleargs << (ArgVar << (Var ^ var) << Undefined);
              exprseq << (Expr << (RefTerm << (Var ^ var)));
            }

            return Seq << (Lift << DataModule
                                << (RuleFunc << (Var ^ rulename) << ruleargs
                                             << rulebody << rulevalue << version
                                             << (Int ^ "0")))
                       << (ExprCall << (Var ^ rulename) << exprseq);
          },

        // errors
      }};
  }
}