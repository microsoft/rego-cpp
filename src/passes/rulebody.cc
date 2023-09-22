#include "internal.hh"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline Numbery = T(ArithInfix) / T(UnaryExpr) / T(NumTerm);
  const auto inline CallToken =
    T(ArrayCompr) / T(ObjectCompr) / T(SetCompr) / T(ExprCall);

  void find_locs_from(
    const Node& node, const std::set<Location>& vars, Node varseq)
  {
    if (node->type() == Var && contains(vars, node->location()))
    {
      varseq->push_back(node->clone());
    }
    for (auto& item : *node)
    {
      find_locs_from(item, vars, varseq);
    }
  }

  // This function analyzes the object structures and variables on both
  // sides and produces the necessary LiteralInit and BoolInfix nodes
  // that represent the unification.
  void add_object_comparisons(
    const Location& obj,
    const Node& lhs,
    const std::set<Location>& lhs_vars,
    const Node& rhs,
    const std::set<Location>& rhs_vars,
    Nodes& literalinits,
    Nodes& boolinfixes)
  {
    // this enables us to find values with constant keys
    std::map<std::string, Node> rhsmap;
    for (auto item : *rhs)
    {
      auto key = (item / Key)->front();
      auto val = (item / Val)->front()->clone();

      if (is_constant(key))
      {
        rhsmap[to_json(key)] = val;
      }
    }

    for (auto item : *lhs)
    {
      Node key = (item / Key);
      if (key->front()->type() == Term)
      {
        key = key->front()->front();
      }
      auto lhsval = (item / Val)->front()->clone();
      if (is_constant(key))
      {
        // constant keys have values that can be determined at compile time
        std::string key_str = to_json(key);

        // gather variables to initialization from the lvalue
        Node lhs_init_vars = NodeDef::create(VarSeq);
        find_locs_from(lhsval, lhs_vars, lhs_init_vars);
        Node rhsval;
        Node rhs_init_vars = NodeDef::create(VarSeq);
        if (contains(rhsmap, key_str))
        {
          // if the rvalue has a constant key, we can use that value
          // and perform variable initialization analysis.
          rhsval = rhsmap[key_str];
          find_locs_from(rhsval, rhs_vars, rhs_init_vars);
        }
        else
        {
          // we cannot know the rvalue at compile time, which means
          // no initialization can take place.
          rhsval = RefTerm
            << (SimpleRef << (Var ^ obj) << (RefArgBrack << key->clone()));
        }

        if (lhs_init_vars->size() > 0)
        {
          literalinits.push_back(
            LiteralInit << lhs_init_vars << rhs_init_vars
                        << (AssignInfix << (AssignArg << lhsval)
                                        << (AssignArg << rhsval)));
        }
        else
        {
          // if nothing is being initialized, we just need to perform
          // an equality check.
          boolinfixes.push_back(
            BoolInfix << (BoolArg << lhsval) << Equals << (BoolArg << rhsval));
        }
      }
      else
      {
        // if the key is not constant, no initialization can take place
        // so we simple perofrm an equality check.
        boolinfixes.push_back(
          BoolInfix << (BoolArg << lhsval) << Equals
                    << (BoolArg
                        << (RefTerm
                            << (SimpleRef << (Var ^ obj)
                                          << (RefArgBrack << key)))));
      }
    }
  }
}

namespace rego
{
  using namespace wf::ops;

  // Convert all Literal nodes into UnifyExpr nodes of the form <var> = <expr> |
  // <not-expr>.
  PassDef rulebody()
  {
    PassDef rulebody = {
      In(UnifyBody) *
          (T(LiteralWith) << (T(UnifyBody)[UnifyBody] * T(WithSeq)[WithSeq])) >>
        [](Match& _) {
          ACTION();
          return UnifyExprWith << _(UnifyBody) << _(WithSeq);
        },

      In(UnifyBody) *
          (T(LiteralEnum)
           << (T(Var)[Lhs] * T(Var)[Rhs] * T(UnifyBody)[UnifyBody])) >>
        [](Match& _) {
          ACTION();
          LOG("enum");
          Location value = _.fresh({"value"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ value) << Undefined))
                     << (UnifyExprEnum << (Var ^ value) << _(Lhs) << _(Rhs)
                                       << _(UnifyBody));
        },

      // LiteralInit nodes are handled separately. They designate what the
      // vars are being initialized. There are three basic cases: Arrays,
      // Objects, and everything else. Arrays and Objects need to be handled
      // specially because they can have nested variables. For example: [x, 1]
      // = [2, 1] or
      // ["a": x, "b": 1] = ["a": 2, "b": 1]
      // These will get broken down into AssignInfix and BoolInfix nodes.
      // Everything else simply gets unified.

      // <array> := indexable
      In(UnifyBody) *
          (T(LiteralInit)
           << (T(VarSeq)[LhsVars] * (T(VarSeq)[RhsVars] << End) *
               (T(AssignInfix)
                << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
                    T(AssignArg)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          LOG("<array> := indexable");
          Node seq = NodeDef::create(Seq);
          Node rhs = _(Rhs)->front();
          if (rhs->type() != RefTerm || rhs->front()->type() != Var)
          {
            // Check if rhs is a var. If not, create a var and set it.
            Location rhs_loc = _.fresh({"rhs"});
            seq << (Local << (Var ^ rhs_loc) << Undefined)
                << (UnifyExpr << (Var ^ rhs_loc) << (Expr << rhs));
            rhs = Var ^ rhs_loc;
          }
          else
          {
            rhs = rhs->front();
          }
          std::set<Location> vars;
          for (auto& item : *_(LhsVars))
          {
            vars.insert(item->location());
          }

          // Enumerate the items of lhs.
          for (std::size_t i = 0; i < _(Lhs)->size(); ++i)
          {
            Node item = _(Lhs)->at(i)->front();
            Node index = Scalar << (Int ^ std::to_string(i));
            Node rhsval = RefTerm
              << (SimpleRef << rhs->clone() << (RefArgBrack << index));
            Node init_vars = NodeDef::create(VarSeq);
            find_locs_from(item, vars, init_vars);
            if (init_vars->size() > 0)
            {
              // If an item contains mne or more vars from the list, create a
              // LiteralInit << AssignInfix for that item.
              seq
                << (LiteralInit << init_vars << VarSeq
                                << (AssignInfix << (AssignArg << item)
                                                << (AssignArg << rhsval)));
            }
            else
            {
              // Otherwise create a BoolInfix.
              Location unify = _.fresh({"unify"});
              seq << (Local << (Var ^ unify) << Undefined)
                  << (UnifyExpr << (Var ^ unify)
                                << (Expr
                                    << (BoolInfix << (BoolArg << item) << Equals
                                                  << (BoolArg << rhsval))));
            }
          }

          if (seq->size() == 0)
          {
            return err(_(Lhs), "Invalid initialization: nothing to initialize");
          }

          return seq;
        },

      // <object> := indexable
      In(UnifyBody) *
          (T(LiteralInit)
           << (T(VarSeq)[LhsVars] * (T(VarSeq)[RhsVars] << End) *
               (T(AssignInfix)
                << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
                    T(AssignArg)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          LOG("<object> := indexable");
          Node seq = NodeDef::create(Seq);
          Node rhs = _(Rhs)->front();
          if (rhs->type() != RefTerm || rhs->front()->type() != Var)
          {
            // Check if rhs is a var. If not, create a var and set it.
            Location rhs_loc = _.fresh({"rhs"});
            seq << (Local << (Var ^ rhs_loc) << Undefined)
                << (UnifyExpr << (Var ^ rhs_loc) << (Expr << rhs));
            rhs = Var ^ rhs_loc;
          }
          else
          {
            rhs = rhs->front();
          }
          std::set<Location> vars;
          for (auto& var : *_(LhsVars))
          {
            vars.insert(var->location());
          }

          // Enumerate the items of lhs.
          for (std::size_t i = 0; i < _(Lhs)->size(); ++i)
          {
            Node item = _(Lhs)->at(i);
            Node key = (item / Key);
            if (key->front()->type() == Term)
            {
              key = key->front()->front();
            }

            Node val = (item / Val)->front();
            Node rhsval = RefTerm
              << (SimpleRef << rhs->clone() << (RefArgBrack << key->clone()));
            Node init_vars = NodeDef::create(VarSeq);
            find_locs_from(item, vars, init_vars);

            if (init_vars->size() > 0)
            {
              // If an item contains one or more vars from the list, create a
              // LiteralInit << AssignInfix for that item.
              seq
                << (LiteralInit << init_vars << VarSeq
                                << (AssignInfix << (AssignArg << val)
                                                << (AssignArg << rhsval)));
            }
            else
            {
              // Otherwise create a Literal << BoolInfix.
              Location unify = _.fresh({"unify"});
              seq << (Local << (Var ^ unify) << Undefined)
                  << (UnifyExpr << (Var ^ unify)
                                << (Expr
                                    << (BoolInfix << (BoolArg << val) << Equals
                                                  << (BoolArg << rhsval))));
            }
          }

          if (seq->size() == 0)
          {
            return err(_(Lhs), "Invalid initialization: nothing to initialize");
          }

          return seq;
        },

      // <array> :=: <array>
      In(UnifyBody) *
          (T(LiteralInit)
           << (T(VarSeq)[LhsVars] * T(VarSeq)[RhsVars] *
               (T(AssignInfix)
                << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
                    (T(AssignArg) << (T(Term) << T(Array)[Rhs])))))) >>
        [](Match& _) {
          ACTION();
          LOG("<array> :=: <array>");

          if (_(Lhs)->size() != _(Rhs)->size())
          {
            return err(_(Rhs), "Array size mismatch");
          }

          std::set<Location> lhs_vars;
          for (auto& var : *_(LhsVars))
          {
            lhs_vars.insert(var->location());
          }

          std::set<Location> rhs_vars;
          for (auto& var : *_(RhsVars))
          {
            rhs_vars.insert(var->location());
          }

          Node seq = NodeDef::create(Seq);
          for (std::size_t i = 0; i < _(Lhs)->size(); ++i)
          {
            Node lhs_term = _(Lhs)->at(i)->front();
            Node rhs_term = _(Rhs)->at(i)->front();

            Node lhs_init_vars = NodeDef::create(VarSeq);
            find_locs_from(lhs_term, lhs_vars, lhs_init_vars);

            Node rhs_init_vars = NodeDef::create(VarSeq);
            find_locs_from(rhs_term, rhs_vars, rhs_init_vars);

            if (lhs_init_vars->size() > 0 || rhs_init_vars->size() > 0)
            {
              seq
                << (LiteralInit << lhs_init_vars << rhs_init_vars
                                << (AssignInfix << (AssignArg << lhs_term)
                                                << (AssignArg << rhs_term)));
            }
            else
            {
              Location unify = _.fresh({"unify"});
              seq << (Local << (Var ^ unify) << Undefined)
                  << (UnifyExpr
                      << (Var ^ unify)
                      << (Expr
                          << (BoolInfix << (BoolArg << lhs_term) << Equals
                                        << (BoolArg << rhs_term))));
            }
          }

          if (seq->size() == 0)
          {
            return err(_(Lhs), "Invalid initialization: nothing to initialize");
          }

          return seq;
        },

      // <object> :=: <object>
      In(UnifyBody) *
          (T(LiteralInit)
           << (T(VarSeq)[LhsVars] * T(VarSeq)[RhsVars] *
               (T(AssignInfix)
                << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
                    (T(AssignArg) << (T(Term) << T(Object)[Rhs])))))) >>
        [](Match& _) {
          ACTION();
          LOG("<object> :=: <object>");
          // first, we set up all equalities going lhs <- rhs
          // so for example, if we have {"a": x, "b": 2, c: 3, "d": 4} = {"a":
          // 1, "b": y, "c": 3, "d": 4} then this will generate: x =
          // rhsobj["a"] 2 == rhsobj["b"] 3 == rhsobj[c] 4 == rhsobj["d"] we
          // then go lhs -> rhs to get the rest: 1 == lhsobj["a"] y =
          // lhsobj["b"] 3 == lhsobj["c"] 4 == lhsobj["d"] ordering matters
          // here! unifications need to go first, then boolean checks x =
          // rhsobj["a"] y = rhsobj["b"] then the boolean checks 1 ==
          // lhsobj["a"] 2 == rhsobj["b"] 3 == rhsobj[c] 4 == rhsobj["d"] 3 ==
          // lhsobj["c"] 4 == lhsobj["d"]

          Node lhs = _(Lhs);
          Node rhs = _(Rhs);
          if (lhs->size() != rhs->size())
          {
            return err(rhs, "Object size mismatch");
          }

          Node seq = NodeDef::create(Seq);
          Location lhsobj = _.fresh({"lhsobj"});
          Location rhsobj = _.fresh({"rhsobj"});

          Nodes literalinits;
          Nodes booleaninfixes;

          std::set<Location> lhs_vars;
          for (auto& var : *_(LhsVars))
          {
            lhs_vars.insert(var->location());
          }

          std::set<Location> rhs_vars;
          for (auto& var : *_(RhsVars))
          {
            rhs_vars.insert(var->location());
          }

          add_object_comparisons(
            lhsobj, lhs, lhs_vars, rhs, rhs_vars, literalinits, booleaninfixes);
          add_object_comparisons(
            rhsobj, rhs, rhs_vars, lhs, lhs_vars, literalinits, booleaninfixes);

          for (auto item : literalinits)
          {
            seq << item;
          }

          seq << (Local << (Var ^ lhsobj) << Undefined)
              << (Local << (Var ^ rhsobj) << Undefined)
              << (UnifyExpr << (Var ^ lhsobj) << (Expr << (Term << _(Lhs))))
              << (UnifyExpr << (Var ^ rhsobj) << (Expr << (Term << _(Rhs))));

          for (auto item : booleaninfixes)
          {
            Location unify = _.fresh({"unify"});
            seq << (Local << (Var ^ unify) << Undefined)
                << (UnifyExpr << (Var ^ unify) << (Expr << item));
          }

          if (seq->size() == 0)
          {
            return err(_(Lhs), "Invalid initialization: nothing to initialize");
          }

          return seq;
        },

      In(UnifyBody) *
          (T(LiteralInit)
           << ((T(VarSeq)[LhsVars] << End) * (T(VarSeq)[RhsVars] << Any) *
               (T(AssignInfix) << (T(AssignArg)[Lhs] * T(AssignArg)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          return LiteralInit << _(RhsVars) << _(LhsVars)
                             << (AssignInfix << _(Rhs) << _(Lhs));
        },

      In(UnifyBody) *
          (T(LiteralInit)
           << ((T(VarSeq)[LhsVars] << Any) * (T(VarSeq)[RhsVars] << End) *
               (T(AssignInfix)
                << ((T(AssignArg) << (T(RefTerm) << T(Var)[Lhs])) *
                    T(AssignArg)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          // eventually, most init statements will reduce down
          // to this.
          return UnifyExpr << _(Lhs) << (Expr << _(Rhs)->front());
        },

      In(UnifyBody) *
          (T(LiteralInit)
           << (T(VarSeq)[LhsVars] * T(VarSeq)[RhsVars] *
               (T(AssignInfix)
                << ((T(AssignArg) << (T(RefTerm) << T(Var)[Lhs])) *
                    T(AssignArg)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          // this occurs when there is a loop in the init. This is allowed,
          // so we unify, but we may want to warn the user.
          return UnifyExpr << _(Lhs) << (Expr << _(Rhs)->front());
        },

      In(UnifyBody) *
          (T(LiteralInit)
           << (T(VarSeq)[LhsVars] * T(VarSeq)[RhsVars] *
               (T(AssignInfix)
                << (T(AssignArg)[Lhs] * (T(RefTerm) << T(Var)[Rhs]))))) >>
        [](Match& _) {
          ACTION();
          // this occurs when there is a loop in the init. This is allowed,
          // so we unify, but we may want to warn the user.
          return UnifyExpr << _(Rhs) << (Expr << _(Lhs)->front());
        },

      In(UnifyBody) *
          (T(Literal) << ((T(Expr) << T(AssignInfix)[AssignInfix]))) >>
        [](Match& _) {
          ACTION();
          return _(AssignInfix);
        },

      In(With) * T(Expr)[Expr] * In(UnifyBody)++ >>
        [](Match& _) {
          ACTION();
          LOG("with");
          Location temp = _.fresh({"with"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ temp) << Undefined))
                     << (Lift << UnifyBody
                              << (UnifyExpr << (Var ^ temp) << _(Expr)))
                     << (Var ^ temp);
        },

      // <expr>
      In(UnifyBody) * (T(Literal) << T(Expr)[Expr]) >>
        [](Match& _) {
          ACTION();
          std::string prefix = in_query(_(Expr)) ? "value" : "unify";
          Location temp = _.fresh({prefix});
          return Seq << (Local << (Var ^ temp) << Undefined)
                     << (UnifyExpr << (Var ^ temp) << _(Expr));
        },

      // <any> = <any>
      In(UnifyBody) *
          (T(AssignInfix) << (T(AssignArg)[Lhs] * T(AssignArg)[Rhs])) >>
        [](Match& _) {
          ACTION();
          LOG("<any> = <any>");
          Node seq = NodeDef::create(Seq);
          Location unify = _.fresh({"unify"});
          return Seq << (Local << (Var ^ unify) << Undefined)
                     << (UnifyExpr
                         << (Var ^ unify)
                         << (Expr
                             << (BoolInfix << (BoolArg << _(Lhs)->front())
                                           << Equals
                                           << (BoolArg << _(Rhs)->front()))));
        },

      // not any = any
      In(UnifyBody) *
          (T(LiteralNot)
           << (T(Expr)
               << (T(AssignInfix)
                   << (T(AssignArg)[Lhs] * T(AssignArg)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          LOG("not any = any");
          Location unify = _.fresh({"unify"});
          return Seq << (Local << (Var ^ unify) << Undefined)
                     << (UnifyExpr
                         << (Var ^ unify)
                         << (Expr
                             << (BoolInfix << (BoolArg << _(Lhs)->front())
                                           << NotEquals
                                           << (BoolArg << _(Rhs)->front()))));
        },

      // <notexpr>
      In(UnifyBody) * (T(LiteralNot) << T(UnifyBody)[UnifyBody]) >>
        [](Match& _) {
          ACTION();
          return UnifyExprNot << _(UnifyBody);
        },

      // <array> = <array>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Array)[Lhs])) *
               (T(AssignArg) << (T(Term) << T(Array)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          LOG("<array> = <array>");
          Node lhs = _(Lhs);
          Node rhs = _(Rhs);
          if (lhs->size() != rhs->size())
          {
            return err(_(Lhs), "Array size mismatch");
          }

          Location unify = _.fresh({"unify"});
          return Seq << (Local << (Var ^ unify) << Undefined)
                     << (UnifyExpr
                         << (Var ^ unify)
                         << (Expr
                             << (BoolInfix << (BoolArg << (Term << lhs))
                                           << Equals
                                           << (BoolArg << (Term << rhs)))));
        },

      // <object> = <object>
      In(UnifyBody) *
          (T(AssignInfix)
           << ((T(AssignArg) << (T(Term) << T(Object)[Lhs])) *
               (T(AssignArg) << (T(Term) << T(Object)[Rhs])))) >>
        [](Match& _) {
          ACTION();
          LOG("<object> = <object>");

          Node lhs = _(Lhs);
          Node rhs = _(Rhs);
          if (lhs->size() != rhs->size())
          {
            return err(rhs, "Object size mismatch");
          }

          Location unify = _.fresh({"unify"});
          return Seq << (Local << (Var ^ unify) << Undefined)
                     << (UnifyExpr
                         << (Var ^ unify)
                         << (Expr
                             << (BoolInfix << (BoolArg << (Term << lhs))
                                           << Equals
                                           << (BoolArg << (Term << rhs)))));
        },

      In(UnifyBody) * (T(LiteralEnum) << T(UnifyExpr)[UnifyExpr]) >>
        [](Match& _) {
          ACTION();
          return _(UnifyExpr);
        },

      // <compr>
      In(UnifyBody) *
          (T(UnifyExpr)
           << (T(Var)[Var] *
               (T(Expr)
                << (T(Term)
                    << (T(ArrayCompr) / T(SetCompr) /
                        T(ObjectCompr))[Compr])))) >>
        [](Match& _) {
          ACTION();
          LOG("<compr>");
          return UnifyExprCompr << _(Var)
                                << (_(Compr)->type() << (_(Compr) / Var))
                                << (_(Compr) / NestedBody);
        },

      // <compr>
      In(UnifyBody) *
          (T(UnifyExpr)
           << (T(Var)[Var] *
               (T(Expr)
                << (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr]))) >>
        [](Match& _) {
          ACTION();
          LOG("<compr>");
          return UnifyExprCompr << _(Var)
                                << (_(Compr)->type() << (_(Compr) / Var))
                                << (_(Compr) / NestedBody);
        },

      // <compr> (other)
      T(Term) << (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] *
            In(UnifyBody)++ >>
        [](Match& _) {
          ACTION();
          LOG("<compr> (other)");
          Location term = _.fresh({"term"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ term) << Undefined))
                     << (Lift
                         << UnifyBody
                         << (UnifyExpr << (Var ^ term) << (Expr << _(Compr))))
                     << (RefTerm << (Var ^ term));
        },

      // <binarg>.<setcompr>
      In(BinArg) * T(SetCompr)[SetCompr] * In(UnifyBody)++ >>
        [](Match& _) {
          ACTION();
          LOG("<binarg>.<setcompr>");
          Location set = _.fresh({"setcompr"});
          return Seq << (Lift << UnifyBody
                              << (Local << (Var ^ set) << Undefined))
                     << (Lift
                         << UnifyBody
                         << (UnifyExpr << (Var ^ set) << (Expr << _(SetCompr))))
                     << (RefTerm << (Var ^ set));
        },

      In(RefArgBrack) * T(NumTerm)[NumTerm] >>
        [](Match& _) {
          ACTION();
          return Scalar << _(NumTerm)->front();
        },

      // errors

      In(BoolArg) * T(BoolInfix)[BoolInfix] >>
        [](Match& _) {
          ACTION();
          return err(_(BoolInfix), "Invalid boolean expression");
        },

      In(Term, BinArg) * T(SetCompr)[SetCompr] >>
        [](Match& _) {
          ACTION();
          return err(_(SetCompr), "Invalid set comprehension");
        },

      In(Term) * T(ArrayCompr)[ArrayCompr] >>
        [](Match& _) {
          ACTION();
          return err(_(ArrayCompr), "Invalid array comprehension");
        },

      In(Term) * T(ObjectCompr)[ObjectCompr] >>
        [](Match& _) {
          ACTION();
          return err(_(ObjectCompr), "Invalid object comprehension");
        },

      In(ObjectItem) * (T(Expr)[Expr] << T(AssignInfix)) >>
        [](Match& _) {
          ACTION();
          return err(_(Expr), "Invalid object item");
        },

      In(UnifyBody) * T(AssignInfix)[AssignInfix] >>
        [](Match& _) {
          ACTION();
          return err(_(AssignInfix), "Invalid assignment");
        },

      In(Expr) * T(AssignInfix)[AssignInfix] >>
        [](Match& _) {
          ACTION();
          return err(_(AssignInfix), "Invalid assignment");
        },

      In(BoolArg) * (T(Membership) / T(AssignInfix))[Arg] >>
        [](Match& _) {
          ACTION();
          return err(_(Arg), "Invalid boolean argument");
        },

      In(RefArgBrack) * (T(Membership) / T(AssignInfix))[Arg] >>
        [](Match& _) {
          ACTION();
          return err(_(Arg), "Invalid index");
        },
    };

    rulebody.post(UnifyBody, [](Node n) {
      for (auto child : *n)
      {
        if (child->type() == LiteralInit)
        {
          n->replace(child, err(child, "Invalid initialization"));
        }
      }
      return 0;
    });

    return rulebody;
  }
}