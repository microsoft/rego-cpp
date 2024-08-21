#include "internal.hh"
#include "rego.hh"

namespace
{
  using namespace rego;

  bool in_parent(NodeRange range, Token token)
  {
    return range.front()->parent()->parent() == token;
  }

  bool newline_between(Node lhs, Node rhs)
  {
    Location start = lhs->location();
    Location end = rhs->location();
    Location between(
      start.source, start.pos + start.len, end.pos - start.pos - start.len);
    return between.view().find('\n') != std::string::npos;
  }

  const std::set<Token> op_tokens = {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Equals,
    NotEquals,
    LessThan,
    GreaterThan,
    LessThanOrEquals,
    GreaterThanOrEquals,
    Or,
    And,
    Assign,
    Unify,
    IsIn,
    Colon,
    As};

  Nodes comma_separation(Node group)
  {
    Nodes items;
    Node current = NodeDef::create(Group);
    for (Node node : *group)
    {
      if (node == Comma)
      {
        items.push_back(current);
        current = NodeDef::create(Group);
      }
      else
      {
        current->push_back(node);
      }
    }

    if (!current->empty())
    {
      items.push_back(current);
    }

    return items;
  }

  Node set_or_query(Node brace, bool has_commas)
  {
    auto parent = brace->parent();
    Node group = brace->front();
    auto pos = parent->find(brace);
    Node before = pos != parent->begin() ? pos[-1] : nullptr;
    Node after = pos + 1 != parent->end() ? pos[1] : nullptr;
    if (before != nullptr && contains(op_tokens, before->type()))
    {
      if (has_commas)
      {
        return Set << comma_separation(group);
      }
      else
      {
        return Set << group;
      }
    }
    else if (after != nullptr && contains(op_tokens, after->type()))
    {
      if (has_commas)
      {
        return Set << comma_separation(group);
      }
      else
      {
        return Set << group;
      }
    }
    else if (before == nullptr && after == nullptr)
    {
      if (has_commas)
      {
        return Set << comma_separation(group);
      }
      else
      {
        return Set << group;
      }
    }
    else
    {
      return Query << group;
    }
  }

  const auto RefGroup = TokenDef("rego-refgroup");
  const auto Keyword = TokenDef("rego-keyword", flag::lookdown);

  const auto wf_prep_tokens = (wf_parse_tokens | Scalar | Placeholder) -
    (Int | Float | JSONString | RawString | True | False | Null);

  // clang-format off
  const auto wf_prep =
    (Top <<= (Module | Query))
    | (Square <<= Group++)
    | (Brace <<= Group++)
    | (Paren <<= Group++)
    | (Module <<= (Group | Package | Import)++)
    | (Query <<= Group)
    | (Package <<= RefGroup)
    | (Import <<= RefGroup * Var)
    | (Scalar <<= Int | Float | String | True | False | Null)
    | (String <<= JSONString | RawString)
    | (RefGroup <<= (Var | Dot | Square)++)
    | (Group <<= wf_prep_tokens++)
    ;
  // clang-format on

  PassDef prep()
  {
    return {
      "prep",
      wf_prep,
      dir::topdown | dir::once,
      {
        In(Top) * (T(File) << (T(Group) << (T(Module)[Module]))) >>
          [](Match& _) { return _(Module); },

        In(Top) * (T(File) << (T(Group) << (T(Query)[Query]))) >>
          [](Match& _) { return _(Query); },

        In(Module) * T(Group)
            << (T(Package) * T(Var, Dot, Square)++[Group] * End) >>
          [](Match& _) { return Package << (RefGroup << _[Group]); },

        In(Module) * T(Group)
            << (T(Import) * T(Var, Dot, Square)++[Group] * End) >>
          [](Match& _) {
            Node alias = _[Group].back();
            if (alias == Square)
            {
              Node group = alias->front();
              if (group->empty())
              {
                return err(alias, "Invalid import", "well-formed error");
              }
              alias = group->front();
              if (alias == JSONString)
              {
                alias = Var ^ strip_quotes(alias->location().view());
              }
            }

            if (alias != Var)
            {
              return err(alias, "Invalid import", "well-formed error");
            }

            return Import << (RefGroup << _[Group]) << alias->clone();
          },

        T(Group)
            << (T(Import) * T(Var, Dot, Square)++[Group] * T(As) * T(Var)[Var] *
                End) >>
          [](Match& _) { return Import << (RefGroup << _[Group]) << _(Var); },

        T(Var, "_")[Var] >> [](Match& _) { return Placeholder ^ _(Var); },

        In(Group) * T(Int, Float, True, False, Null)[Scalar] >>
          [](Match& _) { return Scalar << _(Scalar); },

        In(Group) * T(JSONString, RawString)[String] >>
          [](Match& _) { return Scalar << (String << _(String)); },

        // errors
        In(Top) * T(File)[File] >>
          [](Match& _) {
            return err(_(File), "Invalid file", WellFormedError);
          },

        T(Package)[Package] << End >>
          [](Match& _) {
            return err(_(Package), "Invalid package declaration");
          },

        T(Import)[Import] << End >>
          [](Match& _) { return err(_(Import), "Invalid import declaration"); },
      }};
  }

  const auto wf_keywords_tokens = wf_prep_tokens | If | IsIn | Contains | Every;

  // clang-format off
  const auto wf_keywords =
    wf_prep
    | (Module <<= (Group | Package | Import | Keyword | Version)++)
    | (Keyword <<= Var)[Var] 
    | (Group <<= wf_keywords_tokens++)
    ;
  // clang-format on

  PassDef keywords()
  {
    std::shared_ptr<std::map<std::string, Token>> keywords =
      std::make_shared<std::map<std::string, Token>>();
    PassDef pass = {
      "keywords",
      wf_keywords,
      dir::topdown | dir::once,
      {
        In(Group) *
            T(Var, "(?:if|in|contains|every)")[Var]([keywords](auto& n) {
              std::string keyword(n.front()->location().view());
              return keywords->count(keyword) != 0;
            }) >>
          [keywords](Match& _) {
            std::string keyword(_(Var)->location().view());
            return keywords->at(keyword) ^ _(Var);
          },

        In(Module) *
            (T(Import)[Import]
             << (T(RefGroup)
                 << (T(Var, "rego") * T(Dot) * T(Var, "v1")[Version]))) >>
          [keywords](Match& _) {
            auto nodes = NodeDef::create(Seq);
            if (keywords->empty())
            {
              keywords->insert({"if", If});
              nodes << (Keyword << (Var ^ "if"));
              keywords->insert({"in", IsIn});
              nodes << (Keyword << (Var ^ "in"));
              keywords->insert({"contains", Contains});
              nodes << (Keyword << (Var ^ "contains"));
              keywords->insert({"every", Every});
              nodes << (Keyword << (Var ^ "every"));
              keywords->insert({"version", Version});
              nodes << (Version ^ _(Version));
            }
            else
            {
              return err(
                _(Import),
                "the `rego.v1` import implies `future.keywords`, these are "
                "therefore mutually exclusive",
                RegoParseError);
            }

            return nodes;
          },

        In(Module) *
            (T(Import)[Import]
             << (T(RefGroup)
                 << (T(Var, "future") * T(Dot) * T(Var, "keywords") *
                     ~(T(Dot) * T(Var, "(?:if|in|contains|every)")[Var])))) >>
          [keywords](Match& _) {
            if (contains(keywords, "version"))
            {
              return err(
                _(Import),
                "the `rego.v1` import implies `future.keywords`, these are "
                "therefore mutually exclusive",
                RegoParseError);
            }

            auto nodes = NodeDef::create(Seq);
            if (_(Var) == nullptr)
            {
              keywords->insert({"if", If});
              nodes << (Keyword << (Var ^ "if"));
              keywords->insert({"in", IsIn});
              nodes << (Keyword << (Var ^ "in"));
              keywords->insert({"contains", Contains});
              nodes << (Keyword << (Var ^ "contains"));
              keywords->insert({"every", Every});
              nodes << (Keyword << (Var ^ "every"));
            }
            else
            {
              std::string keyword(_(Var)->location().view());
              if (keyword == "if")
              {
                keywords->insert({"if", If});
              }
              else if (keyword == "in")
              {
                keywords->insert({"in", IsIn});
              }
              else if (keyword == "contains")
              {
                keywords->insert({"contains", Contains});
              }
              else if (keyword == "every")
              {
                keywords->insert({"in", IsIn});
                keywords->insert({"every", Every});
              }
              nodes << (Keyword << _(Var));
            }

            return nodes;
          },
      }};

    pass.post([keywords](Node) {
      keywords->clear();
      return 0;
    });

    return pass;
  }

  // clang-format off
  const auto wf_some_every =
    wf_keywords
    | (Every <<= Var++)
    | (Some <<= Var++)
    ;
  // clang-format on

  Node to_var(Node node)
  {
    if (node == Var)
    {
      return node;
    }

    return Var ^ node->fresh(node->location());
  }

  PassDef some_every()
  {
    return {
      "some_every",
      wf_some_every,
      dir::bottomup | dir::once,
      {
        In(Group) *
            (T(Some) * T(Var)[Head] * (T(Comma) * T(Var))++[Tail] *
             --T(IsIn, Dot, Comma)) >>
          [](Match& _) -> Node {
          Node some = Some << _(Head);
          for (auto it = _[Tail].begin(); it != _[Tail].end(); it += 2)
          {
            // skip the comma
            some << it[1];
          }

          return some;
        },

        In(Group) *
            (T(Every) * T(Var, Placeholder)[Head] *
             (T(Comma) * T(Var, Placeholder))++[Tail]) >>
          [](Match& _) {
            Node every = Every << to_var(_(Head));
            for (auto it = _[Tail].begin(); it != _[Tail].end(); it += 2)
            {
              // skip the comma
              every << to_var(it[1]);
            }

            return every;
          },
      }};
  }

  const auto wf_ref_args_tokens =
    (wf_keywords_tokens | RefArgDot | RefArgBrack);

  // clang-format off
  const auto wf_ref_args =
    wf_some_every
    | (RefArgDot <<= Var)
    | (RefArgBrack <<= Group)
    | (RefGroup <<= (Var | RefArgDot | RefArgBrack)++)
    | (Group <<= wf_ref_args_tokens++)
    ;
  // clang-format on

  PassDef ref_args()
  {
    return {
      "ref_args",
      wf_ref_args,
      dir::bottomup,
      {
        In(Group, RefGroup) * (T(Dot) * T(Var)[Var])[RefArgDot] >>
          [](Match& _) { return RefArgDot << _(Var); },

        In(Group) * (T(Dot) * T(If, IsIn, Contains, Every)[Var]) >>
          [](Match& _) { return RefArgDot << (Var ^ _(Var)); },

        In(Group) * (T(Contains)[Contains] * T(Paren)[Paren]) >>
          [](Match& _) { return Seq << (Var ^ _(Contains)) << _(Paren); },

        In(Group, RefGroup) *
            (T(Var, RefArgDot, RefArgBrack, Square, Brace, Paren)[Val] *
             (T(Square) << T(Group)[Group]))(
              [](auto n) { return !newline_between(n.front(), n.back()); }) >>
          [](Match& _) { return Seq << _(Val) << (RefArgBrack << _(Group)); },

        // errors

        In(RefGroup) * T(Dot, Square)[Val] >>
          [](Match& _) {
            return err(_(Val), "Invalid reference argument", WellFormedError);
          },
      }};
  }

  const auto wf_refs_tokens = (wf_ref_args_tokens | Expr | Ref);

  // clang-format off
  const auto wf_refs =
    wf_ref_args
    | (Expr <<= ExprCall)
    | (ExprCall <<= Ref * ExprSeq)
    | (ExprSeq <<= Group++)
    | (Ref <<= RefHead * RefArgSeq)
    | (RefHead <<= Var | ExprCall | Square | Brace)
    | (RefArgSeq <<= (RefArgDot | RefArgBrack)++)
    | (RefGroup <<= Var | Ref)
    | (Group <<= wf_refs_tokens++)
    ;
  // clang-format on

  PassDef refs()
  {
    PassDef pass = {
      "refs",
      wf_refs,
      dir::bottomup,
      {
        In(Group) * (T(Ref, Var)[Ref] * (T(Paren) << T(Group)[Group])) >>
          [](Match& _) {
            Node ref = _(Ref);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            return ExprCall << ref << (ExprSeq << comma_separation(_(Group)));
          },

        In(Group) * (T(Ref, Var)[Ref] * (T(Paren) << End)) >>
          [](Match& _) {
            Node ref = _(Ref);
            if (ref == Var)
            {
              ref = Ref << (RefHead << ref) << RefArgSeq;
            }

            return ExprCall << ref << ExprSeq;
          },

        In(Group, RefGroup) *
            (T(Var, ExprCall, Square, Brace)[RefHead] *
             T(RefArgDot, RefArgBrack)[Head] *
             T(RefArgDot, RefArgBrack)++[Tail]) >>
          [](Match& _) {
            return Ref << (RefHead << _(RefHead))
                       << (RefArgSeq << _(Head) << _[Tail]);
          },

        In(Group, RefGroup) *
            T(Var)[Var]([](auto& n) { return !in_parent(n, RefArgBrack); }) >>
          [](Match& _) { return Ref << (RefHead << _(Var)) << RefArgSeq; },

        In(Group) * T(ExprCall)[ExprCall] >>
          [](Match& _) { return Expr << _(ExprCall); },

        // errors

        In(RefGroup) * T(Var, RefArgDot, RefArgBrack)[Ref] >>
          [](Match& _) {
            return err(_(Ref), "Invalid reference", WellFormedError);
          },

        T(RefGroup)[RefGroup] << End >>
          [](Match& _) {
            return err(_(RefGroup), "Missing reference", WellFormedError);
          },

        In(RefGroup) * (T(Ref) * T(Ref)[Ref]) >>
          [](Match& _) {
            return err(_(Ref), "Invalid reference", WellFormedError);
          },
      }};

    return pass;
  }

  const auto wf_groups_tokens = (wf_refs_tokens | Array | Object | Set |
                                 ArrayCompr | SetCompr | ObjectCompr) -
    (Square | Brace | Paren | EmptySet);

  // clang-format off
  const auto wf_groups =
    wf_refs
    | (Expr <<= ExprParens | ExprCall)
    | (ExprParens <<= Group)
    | (RefHead <<= Var | ExprCall | Array | Object | Set | ArrayCompr | ObjectCompr | SetCompr)
    | (Array <<= Group++)
    | (Object <<= Group++)
    | (Set <<= Group++)
    | (Query <<= Group)
    | (ArrayCompr <<= Group * Query)
    | (SetCompr <<= Group * Query)
    | (ObjectCompr <<= Group * Query)
    | (Package <<= Ref)
    | (Import <<= Ref * Var)
    | (RefArgBrack <<= Group | Placeholder)
    | (Group <<= wf_groups_tokens++)
    ;
  // clang-format on

  PassDef groups()
  {
    auto comma_groups = std::make_shared<std::set<NodeDef*>>();
    auto colon_groups = std::make_shared<std::set<NodeDef*>>();
    auto or_groups = std::make_shared<std::set<NodeDef*>>();

    PassDef pass = {
      "groups",
      wf_groups,
      dir::bottomup | dir::once,
      {
        In(Package) * (T(RefGroup) << (T(Ref)[Ref] * End)) >>
          [](Match& _) { return _(Ref); },

        In(Import) * (T(RefGroup) << (T(Ref)[Ref] * End)) >>
          [](Match& _) { return _(Ref); },

        T(Brace)[Brace] << T(Group)[Group] >>
          [comma_groups, colon_groups, or_groups](Match& _) {
            NodeDef* group = _(Group).get();
            if (contains(comma_groups, group))
            {
              comma_groups->erase(group);
              if (colon_groups->count(group) != 0)
              {
                colon_groups->erase(group);
                if (contains(or_groups, group))
                {
                  or_groups->erase(group);
                  auto pos = _(Group)->find_first(Or, _(Group)->begin());
                  Nodes item(_(Group)->begin(), pos);
                  Nodes query(pos + 1, _(Group)->end());
                  return ObjectCompr << (Group << item)
                                     << (Query << (Group << query));
                }
                else
                {
                  return Object << comma_separation(_(Group));
                }
              }
              else
              {
                return set_or_query(_(Brace), true);
              }
            }
            else if (contains(colon_groups, group))
            {
              colon_groups->erase(group);
              if (contains(or_groups, group))
              {
                or_groups->erase(group);
                auto pos = _(Group)->find_first(Or, _(Group)->begin());
                Nodes item(_(Group)->begin(), pos);
                Nodes query(pos + 1, _(Group)->end());
                return ObjectCompr << (Group << item)
                                   << (Query << (Group << query));
              }
              else
              {
                return Object << _(Group);
              }
            }
            else if (contains(or_groups, group))
            {
              or_groups->erase(group);
              auto pos = _(Group)->find_first(Or, _(Group)->begin());
              Nodes item(_(Group)->begin(), pos);
              Nodes query(pos + 1, _(Group)->end());
              return SetCompr << (Group << item) << (Query << (Group << query));
            }
            else
            {
              return set_or_query(_(Brace), false);
            }
          },

        T(Square) << T(Group)[Group] >>
          [comma_groups, or_groups](Match& _) {
            NodeDef* group = _(Group).get();
            if (contains(or_groups, group))
            {
              or_groups->erase(group);
              if (contains(comma_groups, group))
              {
                comma_groups->erase(group);
                return Array << comma_separation(_(Group));
              }
              else
              {
                auto pos = _(Group)->find_first(Or, _(Group)->begin());
                Nodes item(_(Group)->begin(), pos);
                Nodes query(pos + 1, _(Group)->end());
                return ArrayCompr << (Group << item)
                                  << (Query << (Group << query));
              }
            }
            else if (contains(comma_groups, group))
            {
              return Array << comma_separation(_(Group));
            }
            else
            {
              return Array << _(Group);
            }
          },

        T(Paren) << T(Group)[Group] >>
          [](Match& _) { return Expr << (ExprParens << _(Group)); },

        T(EmptySet)[EmptySet] >> [](Match& _) { return Set ^ _(EmptySet); },

        In(RefArgBrack) * (T(Group) << T(Placeholder)[Placeholder]) >>
          [](Match& _) { return _(Placeholder); },

        T(Square)[Square] << End >> [](Match& _) { return Array ^ _(Square); },

        T(Brace)[Brace] << End >> [](Match& _) { return Object ^ _(Brace); },

        // errors

        In(Group) * T(Brace, Square, Paren)[Val] >>
          [](Match& _) {
            return err(_(Val), "Invalid collection", WellFormedError);
          },

        T(RefGroup)[RefGroup] >>
          [](Match& _) {
            return err(_(RefGroup), "Invalid reference", WellFormedError);
          },
      }};

    pass.pre(Comma, [comma_groups](Node node) {
      comma_groups->insert(node->parent_unsafe());
      return 0;
    });

    pass.pre(Colon, [colon_groups](Node node) {
      colon_groups->insert(node->parent_unsafe());
      return 0;
    });

    pass.pre(Or, [or_groups](Node node) {
      auto group = node->parent_unsafe();
      auto pos = group->find(node);
      for (auto it = group->begin(); it != pos; ++it)
      {
        Node n = *it;
        if (n == Comma || (n != Colon && contains(op_tokens, n->type())))
        {
          return 0;
        }
      }

      or_groups->insert(group);
      return 0;
    });

    std::shared_ptr<std::map<Location, Location>> import_vars =
      std::make_shared<std::map<Location, Location>>();
    pass.post(Import, [import_vars](Node node) {
      Location ref = (node / Ref)->location();
      Location loc = (node / Var)->location();
      if (contains(*import_vars, loc))
      {
        std::ostringstream err_buf;
        err_buf << "import must not shadow import "
                << import_vars->at(loc).view();
        node->parent()->replace(
          node, err(node, err_buf.str(), RegoCompileError));
      }
      else
      {
        import_vars->insert({loc, ref});
      }
      return 0;
    });

    pass.post([comma_groups, colon_groups, or_groups, import_vars](Node) {
      comma_groups->clear();
      colon_groups->clear();
      or_groups->clear();
      import_vars->clear();
      return 0;
    });

    return pass;
  }

  const auto wf_terms_tokens = wf_groups_tokens -
    (Ref | Var | Scalar | Array | Object | Set | ArrayCompr | ObjectCompr |
     SetCompr | Placeholder);

  // clang-format off
  const auto wf_terms =
    wf_groups
    | (Expr <<= Term | ExprParens | ExprCall)
    | (Term <<= Ref | Var | Scalar | Array | Object | Set | ArrayCompr | ObjectCompr | SetCompr)
    | (Some <<= Group++)
    | (Group <<= wf_terms_tokens++)
    ;
  // clang-format on

  const auto TermToken =
    T(Ref, Var, Scalar, Array, Object, Set, ArrayCompr, ObjectCompr, SetCompr);

  PassDef terms()
  {
    return {
      "terms",
      wf_terms,
      dir::bottomup | dir::once,
      {
        In(Group) * TermToken[Term] >>
          [](Match& _) { return Expr << (Term << _(Term)); },

        In(Group) * T(Placeholder)[Placeholder] >>
          [](Match& _) {
            return Expr
              << (Term << (Var ^ _.fresh(_(Placeholder)->location())));
          },

        In(Some) * T(Var)[Var] >>
          [](Match& _) {
            // because ExprSeq <<= Group++ at this point
            return (Group << (Expr << (Term << _(Var))));
          },
      }};
  }

  const auto InfixOperatorToken =
    T(Multiply,
      Divide,
      Modulo,
      Add,
      Subtract,
      Equals,
      NotEquals,
      LessThan,
      GreaterThan,
      LessThanOrEquals,
      GreaterThanOrEquals,
      Assign,
      Unify);

  // clang-format off
  const auto wf_unary =
    wf_terms
    | (Expr <<= Term | UnaryExpr | ExprParens | ExprCall)
    | (UnaryExpr <<= Expr)
    ;
  // clang-format on

  PassDef unary()
  {
    return {
      "unary",
      wf_unary,
      dir::bottomup,
      {
        In(Group) * (Start * T(Subtract) * T(Expr)[Expr]) >>
          [](Match& _) { return Expr << (UnaryExpr << _(Expr)); },

        In(Group) * (InfixOperatorToken[Op] * T(Subtract) * T(Expr)[Expr]) >>
          [](Match& _) {
            return Seq << _(Op) << (Expr << (UnaryExpr << _(Expr)));
          },
      }};
  }

  // clang-format off
  const auto wf_arithbin_first =
    wf_unary
    | (Expr <<= Term | ExprInfix | ExprParens | ExprCall | UnaryExpr)
    | (ExprInfix <<= Expr * InfixOperator * Expr)
    | (InfixOperator <<= ArithOperator | BinOperator)
    | (ArithOperator <<= Multiply | Divide | Modulo)
    | (BinOperator <<= And)
    ;
  // clang-format on

  PassDef arithbin_first()
  {
    return {
      "arithbin_first",
      wf_arithbin_first,
      dir::bottomup,
      {
        In(Group) *
            (T(Expr)[Lhs] * T(Multiply, Divide, Modulo)[Op] * T(Expr)[Rhs]) >>
          [](Match& _) {
            return Expr
              << (ExprInfix << _(Lhs)
                            << (InfixOperator << (ArithOperator << _(Op)))
                            << _(Rhs));
          },

        In(Group) * (T(Expr)[Lhs] * T(And)[Op] * T(Expr)[Rhs]) >>
          [](Match& _) {
            return Expr
              << (ExprInfix << _(Lhs)
                            << (InfixOperator << (BinOperator << _(Op)))
                            << _(Rhs));
          },
      }};
  }

  // clang-format off
  const auto wf_arithbin_second =
    wf_arithbin_first
    | (ArithOperator <<= Multiply | Divide | Modulo | Add | Subtract)
    | (BinOperator <<= And | Or)
    ;
  // clang-format on

  PassDef arithbin_second()
  {
    return {
      "arithbin_second",
      wf_arithbin_second,
      dir::bottomup,
      {
        In(Group) * (T(Expr)[Lhs] * T(Add, Subtract)[Op] * T(Expr)[Rhs]) >>
          [](Match& _) {
            return Expr
              << (ExprInfix << _(Lhs)
                            << (InfixOperator << (ArithOperator << _(Op)))
                            << _(Rhs));
          },

        In(Group) *
            (T(Expr)[Lhs] *
             (T(Expr) << (T(Term) << (T(Scalar) << T(Int, "-.*")[Rhs])))) >>
          [](Match& _) {
            Location subtract_loc = _(Rhs)->location();
            Location value_loc = subtract_loc;
            value_loc.pos += 1;
            value_loc.len -= 1;
            subtract_loc.len = 1;
            return Expr
              << (ExprInfix
                  << _(Lhs)
                  << (InfixOperator
                      << (ArithOperator << (Subtract ^ subtract_loc)))
                  << (Expr << (Term << (Scalar << (Int ^ value_loc)))));
          },

        In(Group) *
            (T(Expr)[Lhs] *
             (T(Expr) << (T(Term) << (T(Scalar) << T(Float, "-.*")[Rhs])))) >>
          [](Match& _) {
            Location subtract_loc = _(Rhs)->location();
            Location value_loc = subtract_loc;
            value_loc.pos += 1;
            value_loc.len -= 1;
            subtract_loc.len = 1;
            return Expr
              << (ExprInfix
                  << _(Lhs)
                  << (InfixOperator
                      << (ArithOperator << (Subtract ^ subtract_loc)))
                  << (Expr << (Term << (Scalar << (Float ^ value_loc)))));
          },

        In(Group) * (T(Expr)[Lhs] * T(Or)[Op] * T(Expr)[Rhs]) >>
          [](Match& _) {
            return Expr
              << (ExprInfix << _(Lhs)
                            << (InfixOperator << (BinOperator << _(Op)))
                            << _(Rhs));
          },
      }};
  }

  // clang-format off
  const auto wf_comparison =
    wf_arithbin_second
    | (InfixOperator <<= ArithOperator | BinOperator | BoolOperator)
    | (BoolOperator <<= Equals | NotEquals | LessThan | GreaterThan | LessThanOrEquals | GreaterThanOrEquals)
    ;
  // clang-format on

  PassDef comparison()
  {
    return {
      "comparison",
      wf_comparison,
      dir::bottomup,
      {
        In(Group) *
            (T(Expr)[Lhs] *
             T(Equals,
               NotEquals,
               LessThan,
               GreaterThan,
               LessThanOrEquals,
               GreaterThanOrEquals)[Op] *
             T(Expr)[Rhs]) >>
          [](Match& _) {
            return Expr
              << (ExprInfix << _(Lhs)
                            << (InfixOperator << (BoolOperator << _(Op)))
                            << _(Rhs));
          },
      }};
  }

  const auto wf_membership_tokens = (wf_terms_tokens | SomeDecl);

  // clang-format off
  const auto wf_membership =
    wf_comparison
    | (Term <<= Var | Ref | Scalar | Array | Object | Set | ArrayCompr | ObjectCompr | SetCompr | Membership)
    | (Membership <<= ExprSeq * Expr)
    | (SomeDecl <<= ExprSeq * (IsIn >>= Expr | Undefined))
    | (Group <<= wf_membership_tokens++)
    ;
  // clang-format on

  PassDef membership()
  {
    return {
      "membership",
      wf_membership,
      dir::bottomup,
      {
        In(Group) *
            ((T(Some) << End) * T(Expr)[Idx] * T(Comma) * T(Expr)[Val] *
             T(IsIn) * T(Expr)[Expr]) >>
          [](Match& _) {
            return SomeDecl
              << (ExprSeq << (Group << _(Idx)) << (Group << _(Val))) << _(Expr);
          },

        In(Group) *
            ((T(Some) << End) * T(Expr)[Val] * T(IsIn) * T(Expr)[Expr]) >>
          [](Match& _) {
            return SomeDecl << (ExprSeq << (Group << _(Val))) << _(Expr);
          },

        In(Group) * (T(Some) << (T(Group)[Head] * T(Group)++[Tail] * End)) >>
          [](Match& _) {
            return SomeDecl << (ExprSeq << _(Head) << _[Tail]) << Undefined;
          },

        In(Group) *
            (T(Expr)[Head] * (T(Comma) * T(Expr))++[Tail] * T(IsIn) *
             T(Expr)[Expr]) >>
          [](Match& _) {
            Node exprseq = ExprSeq << (Group << _(Head));
            for (auto it = _[Tail].begin(); it != _[Tail].end(); it += 2)
            {
              // skip the comma
              exprseq << (Group << it[1]);
            }

            return Expr << (Term << (Membership << exprseq << _(Expr)));
          },
      }};
  }

  const auto wf_assign_tokens = wf_membership_tokens | ObjectItem;

  // clang-format off
  const auto wf_assign =
    wf_membership
    | (InfixOperator <<= ArithOperator | BinOperator | BoolOperator | AssignOperator)
    | (AssignOperator <<= Assign | Unify)
    | (ObjectItem <<= Expr * Expr)
    | (Group <<= wf_assign_tokens++)
    ;
  // clang-format on

  PassDef assign()
  {
    return {
      "assign",
      wf_assign,
      dir::bottomup,
      {
        In(Group) * (T(Expr)[Lhs] * T(Assign, Unify)[Op] * T(Expr)[Rhs]) >>
          [](Match& _) {
            return Expr
              << (ExprInfix << _(Lhs)
                            << (InfixOperator << (AssignOperator << _(Op)))
                            << _(Rhs));
            ;
          },

        In(Group) * (T(Expr)[Key] * T(Colon) * T(Expr)[Val]) >>
          [](Match& _) { return ObjectItem << _(Key) << _(Val); },
      }};
  }

  const auto wf_else_not_tokens = wf_assign_tokens | NotExpr;

  // clang-format off
  const auto wf_else_not =
    wf_assign
    | (Else <<= Expr * Query)
    | (NotExpr <<= Expr)
    | (Group <<= wf_else_not_tokens++)
    ;
  // clang-format on

  PassDef else_not()
  {
    std::shared_ptr<bool> strict = std::make_shared<bool>(false);
    PassDef pass = {
      "else_not",
      wf_else_not,
      dir::bottomup | dir::once,
      {
        In(Group) *
            (T(Else) * T(Assign, Unify) * T(Expr)[Expr] * ~T(If) *
             T(Query)[Query]) >>
          [](Match& _) { return Else << _(Expr) << _(Query); },

        In(Group) *
            (T(Else) * T(Assign, Unify) * T(Expr)[Expr] * T(If) * ~T(Not)[Not] *
             T(Expr, SomeDecl)[Query]) >>
          [](Match& _) {
            Node query = _(Query);
            if (_(Not) != nullptr)
            {
              query = NotExpr << query;
            }

            return Else << _(Expr) << (Query << (Group << query));
          },

        In(Group) * (T(Else) * T(Query)[Query]) >>
          [](Match& _) {
            return Else << (Expr << (Term << (Scalar << True))) << _(Query);
          },

        In(Group) * (T(Else) * T(Assign, Unify) * T(Expr)[Expr]) >>
          [](Match& _) { return Else << _(Expr) << (Query << Group); },

        In(Group) * (T(If)[If] * ~T(Not)[Not] * T(Expr, SomeDecl)[Query]) >>
          [](Match& _) {
            Node query = _(Query);
            if (_(Not) != nullptr)
            {
              query = NotExpr << query;
            }
            return Seq << _(If) << (Query << (Group << query));
          },

        In(Group) * (T(Not) * T(Expr)[Expr]) >>
          [](Match& _) { return NotExpr << _(Expr); },

        In(ExprInfix) *
            ((T(Expr)
              << (T(Term)
                  << (T(Ref)
                      << ((T(RefHead) << T(Var, "input|data")[Var]) *
                          (T(RefArgSeq) << End))))) *
             (T(InfixOperator) << T(AssignOperator)))(
              [strict](auto&) { return *strict; }) >>
          [](Match& _) {
            std::ostringstream err_buf;
            err_buf << "variables must not shadow " << _(Var)->location().view()
                    << " (use a different variable name)";
            return err(_(Var), err_buf.str(), RegoCompileError);
          },

        // errors

        T(Else)[Else] << End >>
          [](Match& _) {
            return err(_(Else), "Invalid else statement", WellFormedError);
          },
      }};

    pass.pre(Version, [strict](Node) {
      *strict = true;
      return 0;
    });

    pass.post([strict](Node) {
      *strict = false;
      return 0;
    });

    return pass;
  }

  const auto wf_collections_tokens = wf_else_not_tokens -
    (ObjectItem | Comma | Add | Subtract | Multiply | Divide | Modulo | And |
     Or | Equals | NotEquals | LessThan | GreaterThan | LessThanOrEquals |
     GreaterThanOrEquals | Assign | Unify);

  // clang-format off
  const auto wf_collections =
    wf_else_not
    | (Object <<= ObjectItem++)
    | (Array <<= Expr++)
    | (Set <<= Expr++)
    | (ArrayCompr <<= Expr * Query)
    | (SetCompr <<= Expr * Query)
    | (ObjectCompr <<= Expr * Expr * Query)
    | (ExprParens <<= Expr)
    | (RefArgBrack <<= Expr | Placeholder)
    | (Membership <<= ExprSeq * Expr)
    | (ExprSeq <<= Expr++)
    | (Group <<= wf_collections_tokens++)
    ;

  PassDef collections()
  {
    return {
      "collections",
      wf_collections,
      dir::topdown | dir::once,
      {
        In(Object) * (T(Group) << (T(ObjectItem)[ObjectItem] * End)) >>
          [](Match& _) { return _(ObjectItem); },

        In(Array, Set, ExprSeq) * (T(Group) << (T(Expr)[Expr] * End)) >>
          [](Match& _) { return _(Expr); },

        In(Array, Set, ObjectItem) *
            (T(Group)
             << (T(Query)
                 << (T(Group)
                     << (T(Expr)[Head] * (T(Comma) * T(Expr))++[Tail] *
                         End)))) >>
          [](Match& _) {
            Node set = Set << _(Head);
            for (auto it = _[Tail].begin(); it != _[Tail].end(); it += 2)
            {
              // skip the comma
              set << it[1];
            }
            return Expr << (Term << set);
          },

        In(RefArgBrack) * (T(Group) << (T(Expr)[Expr]) * End) >>
          [](Match& _) { return _(Expr); },

        In(ExprParens) * (T(Group) << (T(Expr)[Expr]) * End) >>
          [](Match& _) { return _(Expr); },

        In(ArrayCompr, SetCompr) * (T(Group) << (T(Expr)[Expr]) * End) >>
          [](Match& _) { return _(Expr); },

        In(ObjectCompr) *
            (T(Group)
             << ((T(ObjectItem) << (T(Expr)[Key] * T(Expr)[Val])) * End)) >>
          [](Match& _) { return Seq << _(Key) << _(Val); },

        T(Term)
            << (T(Expr)
                << (T(ExprParens) << (T(Expr) << (T(Term)[Term] * End)))) >>
          [](Match& _) { return _(Term); },
        
        // errors

        In(Group) * T(ObjectItem)[ObjectItem] >>
          [](Match& _){
            return err(_(ObjectItem), "Invalid object item", WellFormedError);
          },
        
        In(Group) * T(Comma)[Comma] >>
          [](Match& _){
            return err(_(Comma), "Invalid comma", WellFormedError);
          },

        In(Group) * T(Add, Subtract, Multiply, Divide, Modulo)[Op] >>
          [](Match& _){
            return err(_(Op), "Invalid arithmetic operator", WellFormedError);
          },

        In(Group) * T(And, Or)[Op] >>
          [](Match& _){
            return err(_(Op), "Invalid set operator", WellFormedError);
          },

        In(Group) * T(Equals, NotEquals, LessThan, GreaterThan, LessThanOrEquals, GreaterThanOrEquals)[Op] >>
          [](Match& _){
            return err(_(Op), "Invalid boolean operator", WellFormedError);
          },

        In(Group) * T(Assign, Unify)[Op] >>
          [](Match& _){
            return err(_(Op), "Invalid assignment operator", WellFormedError);
          },
        
        In(Array, Object, Set) * T(Group)[Group] >>
          [](Match& _){
            return err(_(Group), "Invalid collection", WellFormedError);
          },

        In(ExprParens) * T(Group)[Group] >>
          [](Match& _){
            return err(_(Group), "Invalid expression", WellFormedError);
          },

        In(ExprSeq) * T(Group)[Group] >>
          [](Match& _){
            return err(_(Group), "Invalid sequence", WellFormedError);
          },

        In(ArrayCompr, ObjectCompr, SetCompr) * T(Group)[Group] >>
          [](Match& _){
            return err(_(Group), "Invalid comprehension", WellFormedError);
          },

        In(RefArgBrack) * T(Group)[Group] >>
          [](Match& _){
            return err(_(Group), "Invalid reference argument", WellFormedError);
          },
      }};
  }

  // clang-format off
  const auto wf_lines =
    wf_collections
    | (Expr <<= Term | ExprInfix | ExprEvery | ExprParens | ExprCall | UnaryExpr)
    | (ExprEvery <<= VarSeq * (IsIn >>= Term | ExprCall | ExprInfix) * Query)
    | (VarSeq <<= Var++[1])
    | (With <<= Term * Expr)
    ;
  // clang-format on

  PassDef lines()
  {
    return {
      "lines",
      wf_lines,
      dir::bottomup,
      {
        In(Group) *
            (T(SomeDecl, Expr, Query, Else, With, NotExpr)[Lhs] *
             T(SomeDecl, NotExpr, Default, Expr)[Rhs])(
              [](auto& n) { return newline_between(n.front(), n.back()); }) >>
          [](Match& _) { return Seq << _(Lhs) << NewLine << _(Rhs); },

        In(Group) *
            ((T(With) << End) * (T(Expr) << T(Term)[Term]) * T(As) *
             T(Expr)[Expr]) >>
          [](Match& _) { return With << _(Term) << _(Expr); },

        In(Group) *
            (T(Expr, NotExpr, Query, With, SomeDecl, Else)[Expr] * End) >>
          [](Match& _) { return Seq << _(Expr) << NewLine; },

        In(Group) *
            (T(Every)[Every] * T(IsIn) *
             (T(Expr) << T(Term, ExprCall, ExprInfix)[IsIn]) *
             T(Query)[Query]) >>
          [](Match& _) {
            return Expr
              << (ExprEvery << (VarSeq << *_[Every]) << _(IsIn) << _(Query));
          },

        // errors
        T(With)[With] << End >>
          [](Match& _) {
            return err(_(With), "Invalid with statement", WellFormedError);
          },
      }};
  }

  const auto wf_rules_tokens = (wf_collections_tokens | Rule) - (If | Contains);

  // clang-format off
  const auto wf_rules =
    wf_lines
    | (Rule <<= (Default >>= True | False) * RuleHead * RuleBodySeq)
    | (RuleHead <<= RuleRef * (RuleHeadType >>= RuleHeadComp | RuleHeadFunc | RuleHeadObj | RuleHeadSet))
    | (RuleBodySeq <<= (Query | Else)++)
    | (RuleHeadComp <<= Expr)
    | (RuleHeadFunc <<= RuleArgs * Expr)
    | (RuleHeadObj <<= Expr * Expr)
    | (RuleHeadSet <<= Expr)
    | (RuleRef <<= Ref | Var)
    | (RuleArgs <<= Expr++)
    | (Group <<= wf_rules_tokens++)
    ;
  // clang-format on

  PassDef rules()
  {
    std::shared_ptr<bool> strict = std::make_shared<bool>(false);
    PassDef pass = {
      "rules",
      wf_rules,
      dir::bottomup | dir::once,
      {
        In(Group) *
            ((T(Expr)[Rule]([](auto& n) { return in_parent(n, Module); })
              << (T(ExprInfix)
                  << ((T(Expr)
                       << (T(Term)
                           << (T(Ref)
                               << ((T(RefHead) << T(Var)[RuleRef]) *
                                   (T(RefArgSeq)[RefArgSeq]([](auto& n) {
                                     Node refargseq = n.front();
                                     return !refargseq->empty() &&
                                       refargseq->back()->type() == RefArgBrack;
                                   })))))) *
                      (T(InfixOperator) << T(AssignOperator)) *
                      T(Expr)[Val]))) *
             ~T(If)[If] * T(Query, Else)++[RuleBodySeq] * T(NewLine)) >>
          [strict](Match& _) {
            if (*strict && _(If) == nullptr && !_[RuleBodySeq].empty())
            {
              return err(
                _(Rule),
                "`if` keyword is required before rule body",
                RegoParseError);
            }

            Node refargseq = _(RefArgSeq);
            Node key = refargseq->pop_back()->front();
            Node ruleref = _(RuleRef);
            if (!refargseq->empty())
            {
              ruleref = Ref << (RefHead << ruleref) << refargseq;
            }
            return Rule << False
                        << (RuleHead << (RuleRef << ruleref)
                                     << (RuleHeadObj << key << _(Val)))
                        << (RuleBodySeq << _[RuleBodySeq]);
          },

        In(Group) *
            ((T(Expr)[Rule]([](auto& n) { return in_parent(n, Module); })
              << (T(Term)
                  << (T(Ref)
                      << ((T(RefHead) << T(Var)[RuleRef]) *
                          (T(RefArgSeq)[RefArgSeq]([](auto& n) {
                            Node refargseq = n.front();
                            return !refargseq->empty() &&
                              refargseq->back()->type() == RefArgBrack;
                          })))))) *
             ~T(If)[If] * T(Query, Else)++[RuleBodySeq] * T(NewLine)) >>
          [strict](Match& _) {
            if (*strict && _(If) == nullptr && !_[RuleBodySeq].empty())
            {
              return err(
                _(Rule),
                "`if` keyword is required before rule body",
                RegoParseError);
            }

            Node refargseq = _(RefArgSeq);
            Node value = refargseq->pop_back()->front();
            Node ruleref = _(RuleRef);
            auto defs = _(RuleRef)->scope()->lookdown({"contains"});

            if (!refargseq->empty())
            {
              ruleref = Ref << (RefHead << ruleref) << refargseq;
            }

            Node rulehead;
            if (defs.empty() && !refargs_contain_varref(refargseq))
            {
              rulehead = RuleHead << (RuleRef << ruleref)
                                  << (RuleHeadSet << value);
            }
            else
            {
              rulehead = RuleHead
                << (RuleRef << ruleref)
                << (RuleHeadObj << value
                                << (Expr << (Term << (Scalar << True))));
            }

            return Rule << False << rulehead << (RuleBodySeq << _[RuleBodySeq]);
          },

        In(Group) *
            (~T(Default)[Default] *
             (T(Expr)[Rule]([](auto& n) { return in_parent(n, Module); })
              << (T(ExprInfix)
                  << ((T(Expr) << (T(Term) << T(Ref)[RuleRef])) *
                      (T(InfixOperator) << T(AssignOperator)) *
                      T(Expr)[Expr]))) *
             ~T(If)[If] * T(Query, Else)++[RuleBodySeq] * T(NewLine)) >>
          [strict](Match& _) {
            if (*strict && _(If) == nullptr && !_[RuleBodySeq].empty())
            {
              return err(
                _(Rule),
                "`if` keyword is required before rule body",
                RegoParseError);
            }

            return Rule << (_(Default) != nullptr ? True : False)
                        << (RuleHead << (RuleRef << _(RuleRef))
                                     << (RuleHeadComp << _(Expr)))
                        << (RuleBodySeq << _[RuleBodySeq]);
          },

        In(Group) *
            (~T(Default)[Default] *
             (T(Expr)[Rule]([](auto& n) { return in_parent(n, Module); })
              << (T(Term) << T(Ref)[RuleRef])) *
             ~T(If)[If] * T(Query, Else)++[RuleBodySeq] * T(NewLine)) >>
          [strict](Match& _) {
            if (*strict && _(If) == nullptr && !_[RuleBodySeq].empty())
            {
              return err(
                _(Rule),
                "`if` keyword is required before rule body",
                RegoParseError);
            }

            return Rule << (_(Default) != nullptr ? True : False)
                        << (RuleHead
                            << (RuleRef << _(RuleRef))
                            << (RuleHeadComp
                                << (Expr
                                    << (Term << (Scalar << (True ^ "true"))))))
                        << (RuleBodySeq << _[RuleBodySeq]);
          },

        In(Group) *
            (~T(Default)[Default] *
             (T(Expr)[Rule]([](auto& n) { return in_parent(n, Module); })
              << (T(ExprInfix)
                  << ((T(Expr)
                       << (T(ExprCall)
                           << (T(Ref)[RuleRef] * T(ExprSeq)[RuleArgs]))) *
                      (T(InfixOperator) << T(AssignOperator)) *
                      T(Expr)[Expr]))) *
             ~T(If)[If] * T(Query, Else)++[RuleBodySeq] * T(NewLine)) >>
          [strict](Match& _) {
            if (*strict && _(If) == nullptr && !_[RuleBodySeq].empty())
            {
              return err(
                _(Rule),
                "`if` keyword is required before rule body",
                RegoParseError);
            }

            return Rule << (_(Default) != nullptr ? True : False)
                        << (RuleHead
                            << (RuleRef << _(RuleRef))
                            << (RuleHeadFunc << (RuleArgs << *_[RuleArgs])
                                             << _(Expr)))
                        << (RuleBodySeq << _[RuleBodySeq]);
          },

        In(Group) *
            (~T(Default)[Default] *
             (T(Expr)[Rule]([](auto& n) { return in_parent(n, Module); })
              << (T(ExprCall) << (T(Ref)[RuleRef] * T(ExprSeq)[RuleArgs]))) *
             ~T(If)[If] * T(Query, Else)++[RuleBodySeq] * T(NewLine)) >>
          [strict](Match& _) {
            if (*strict && _(If) == nullptr && !_[RuleBodySeq].empty())
            {
              return err(
                _(Rule),
                "`if` keyword is required before rule body",
                RegoParseError);
            }

            return Rule << (_(Default) != nullptr ? True : False)
                        << (RuleHead
                            << (RuleRef << _(RuleRef))
                            << (RuleHeadFunc
                                << (RuleArgs << *_[RuleArgs])
                                << (Expr
                                    << (Term << (Scalar << (True ^ "true"))))))
                        << (RuleBodySeq << _[RuleBodySeq]);
          },

        In(Group) *
            ((T(Expr)[Rule]([](auto& n) { return in_parent(n, Module); })
              << (T(Term) << (T(Ref)[RuleRef]))) *
             T(Contains) * T(Expr)[Expr] * ~T(If)[If] *
             T(Query, Else)++[RuleBodySeq] * T(NewLine)) >>
          [strict](Match& _) {
            if (*strict && _(If) == nullptr && !_[RuleBodySeq].empty())
            {
              return err(
                _(Rule),
                "`if` keyword is required before rule body",
                RegoParseError);
            }

            return Rule << False
                        << (RuleHead << (RuleRef << _(RuleRef))
                                     << (RuleHeadSet << _(Expr)))
                        << (RuleBodySeq << _[RuleBodySeq]);
          },

        In(Group) * T(If, Contains)[Op] >>
          [](Match& _) {
            return err(_(Op), "Invalid rule head", WellFormedError);
          },
      }};

    pass.pre(Version, [strict](Node) {
      *strict = true;
      return 0;
    });

    pass.post([strict](Node) {
      *strict = false;
      return 0;
    });

    return pass;
  }

  const auto wf_literals_tokens = (wf_rules_tokens | Literal) -
    (Expr | NotExpr | SomeDecl | With | NewLine | Keyword | Version);

  // clang-format off
  const auto wf_literals =
    wf_rules
    | (Literal <<= (Expr >>= Expr | NotExpr | SomeDecl) * WithSeq)
    | (WithSeq <<= With++)
    | (Group <<= wf_literals_tokens++)
    ;
  // clang-format on

  PassDef literals()
  {
    return {
      "literals",
      wf_literals,
      dir::bottomup | dir::once,
      {
        In(Group) * (T(SomeDecl)[SomeDecl] * T(With)++[WithSeq] * T(NewLine)) >>
          [](Match& _) {
            return Literal << _(SomeDecl) << (WithSeq << _[WithSeq]);
          },

        In(Group) *
            (T(Expr, NotExpr)[Expr] * T(With)++[WithSeq] * T(NewLine)) >>
          [](Match& _) {
            return Literal << _(Expr) << (WithSeq << _[WithSeq]);
          },

        In(Term) *
            (T(Ref) << ((T(RefHead) << T(Var)[Var]) * (T(RefArgSeq) << End))) >>
          [](Match& _) { return _(Var); },

        T(Keyword, NewLine) >> [](Match&) -> Node { return nullptr; },

        In(Group) * T(Expr, NotExpr, SomeDecl, With)[Val] >>
          [](Match& _) {
            return err(_(Val), "Invalid literal", WellFormedError);
          },
      }};
  }

  PassDef structure()
  {
    return {
      "structure",
      rego::wf,
      dir::bottomup | dir::once,
      {
        In(Module) *
            (T(Package)[Package] * T(Import, Version)++[ImportSeq] *
             (T(Group) << (T(Rule)++[Policy] * End))) >>
          [](Match& _) {
            Node version;
            Node importseq = NodeDef::create(ImportSeq);
            for (auto node : _[ImportSeq])
            {
              if (node == Version)
              {
                version = node;
              }
              else
              {
                importseq << node;
              }
            }

            if (version == nullptr)
            {
              version = Version ^ DefaultVersion;
            }

            return Seq << _(Package) << version << importseq
                       << (Policy << _[Policy]);
          },

        In(Module) * (T(Package)[Package] * T(Import)++[ImportSeq] * End) >>
          [](Match& _) {
            Node version;
            Node importseq = NodeDef::create(ImportSeq);
            for (auto node : _[ImportSeq])
            {
              if (node == Version)
              {
                version = node;
              }
              else
              {
                importseq << node;
              }
            }

            if (version == nullptr)
            {
              version = Version ^ DefaultVersion;
            }

            return Seq << _(Package) << version << importseq << Policy;
          },

        In(Query) * (T(Group) << (T(Literal)++[Query] * End)) >>
          [](Match& _) { return Seq << _[Query]; },

        In(RuleRef) * T(Var)[Var] >>
          [](Match& _) { return Ref << (RefHead << _(Var)) << RefArgSeq; },

        In(RuleArgs) * (T(Expr) << T(Term)[Term]) >>
          [](Match& _) { return _(Term); },

        In(Expr) * (T(Expr) << (T(Term)[Term] * End)) >>
          [](Match& _) { return _(Term); },

        In(TermSeq) * (T(Expr) << T(Term)[Term]) >>
          [](Match& _) { return _(Term); },

        In(VarSeq) * (T(Term) << T(Var)[Var]) >>
          [](Match& _) { return _(Var); },

        In(RuleHead) * (T(RuleHeadComp) << End) >>
          [](Match&) {
            return RuleHeadComp
              << (Expr << (Term << (Scalar << (True ^ "true"))));
          },

        // errors
        T(Group)[Group] >>
          [](Match& _) {
            return err(_(Group), "Syntax error", WellFormedError);
          },

        T(Module)[Module] << (T(Import, Version, Group)++ * End) >>
          [](Match& _) {
            return err(
              _(Module), "Missing package declaration", WellFormedError);
          },

        T(Module)[Module] << T(Package) * T(Import, Version)++ * T(Group) *
              T(Import, Version)[Import] >>
          [](Match& _) {
            return err(
              _(Import),
              "Import statements must come before rules",
              WellFormedError);
          },
      }};
  }

  // clang-format off
  const auto wf_input =
    (Top <<= Input)
    | (Input <<= DataTerm)
    | (DataTerm <<= Scalar | DataArray | DataObject | DataSet)
    | (DataArray <<= DataTerm++)
    | (DataSet <<= DataTerm++)
    | (DataObject <<= DataObjectItem++)
    | (DataObjectItem <<= (Key >>= DataTerm) * (Val >>= DataTerm))
    | (Scalar <<= Int | Float | True | False | Null | String)
    | (String <<= JSONString | RawString)
    ;
  // clang-format on

  PassDef to_input_()
  {
    return {
      "to_input",
      wf_input,
      dir::bottomup | dir::once,
      {
        In(Top) * (T(Query) << (T(Literal)[Literal] * End)) >>
          [](Match& _) { return Input << _(Literal)->front(); },

        T(Expr) << (T(Term) << T(Scalar)[Scalar]) >>
          [](Match& _) { return DataTerm << _(Scalar); },

        T(Expr) << (T(Term) << T(Array)[Array]) >>
          [](Match& _) { return DataTerm << (DataArray << *_[Array]); },

        T(Expr) << (T(Term) << T(Object)[Object]) >>
          [](Match& _) { return DataTerm << (DataObject << *_[Object]); },

        T(Expr) << (T(Term) << T(Set)[Set]) >>
          [](Match& _) { return DataTerm << (DataSet << *_[Set]); },

        T(ObjectItem)[ObjectItem] >>
          [](Match& _) { return DataObjectItem << *_[ObjectItem]; },
      }};
  }
}

namespace rego
{
  Reader reader()
  {
    return {
      "rego",
      {
        prep(),           keywords(),        some_every(),  ref_args(),
        refs(),           groups(),          terms(),       unary(),
        arithbin_first(), arithbin_second(), comparison(),  membership(),
        assign(),         else_not(),        collections(), lines(),
        rules(),          literals(),        structure(),
      },
      parser(),
    };
  }

  Rewriter to_input()
  {
    return {
      "to_input",
      {
        to_input_(),
      },
      rego::wf};
  }
}