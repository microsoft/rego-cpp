#include "internal.hh"

namespace
{
  using namespace rego;

  enum LookupType : std::uint8_t
  {
    LOOKUP_EMPTY = 0,
    LOOKUP_LOCAL = 1,
    LOOKUP_RULE = 2,
    LOOKUP_VDOC = 4,
    LOOKUP_BDOC = 8,
    LOOKUP_DYNAMIC = 16,
  };

  struct Prefix
  {
    LookupType type;
    size_t index;
    Location name;
    Node rule;
    Node virtualdoc;
    Node basedoc;
  };

  Prefix lookup_ref(Node ref)
  {
    Prefix prefix{LOOKUP_EMPTY, 0, {}, nullptr, nullptr, nullptr};
    Node var;
    if (ref->in({Var, LocalRef}))
    {
      var = ref;
    }
    else
    {
      var = (ref / RefHead)->front();
    }

    if (var->location().view() == "input")
    {
      // input is a special local, and thus is an endpoint
      prefix.type = LOOKUP_LOCAL;
      return prefix;
    }

    Nodes results = var->lookup();
    if (results.empty())
    {
      // probably a builtin, but definitely an endpoint
      return prefix;
    }

    if (std::find_if(results.begin(), results.end(), [](auto& n) {
          return n == Local;
        }) != results.end())
    {
      // locals are endpoints
      prefix.type = LOOKUP_LOCAL;
      return prefix;
    }

    Node rule;
    Node basedoc;
    Node virtualdoc;

    for (Node node : results)
    {
      if (node == Rule)
      {
        prefix.type = (LookupType)(prefix.type | LOOKUP_RULE);
        prefix.rule = node;

        if ((node / RuleHead)
              ->front()
              ->in(
                {RuleHeadComp,
                 RuleHeadFunc,
                 RuleHeadObjDynamic,
                 RuleHeadSetDynamic}))
        {
          // these are terminal rules
          prefix.type = LOOKUP_RULE;
          return prefix;
        }
      }
      else if (node == VirtualDocument)
      {
        prefix.type = (LookupType)(prefix.type | LOOKUP_VDOC);
        prefix.virtualdoc = node;
      }
      else if (node == BaseDocument)
      {
        prefix.type = (LookupType)(prefix.type | LOOKUP_BDOC);
        prefix.basedoc = node / BaseObject;
      }
    }

    // now we need to walk the result object until we reach an ending condition.
    Node argseq = ref / RefArgSeq;
    while (prefix.index < argseq->size())
    {
      Node arg = argseq->at(prefix.index);
      if (arg == RefArgBrack)
      {
        // done
        break;
      }

      Nodes bdoc_results;
      Nodes vdoc_results;

      Location key = arg->front()->location();

      if (prefix.type & LOOKUP_BDOC)
      {
        std::string key = to_key(arg->front());
        bdoc_results = prefix.basedoc->lookdown(key);
      }

      if (prefix.type & LOOKUP_VDOC)
      {
        vdoc_results = prefix.virtualdoc->lookdown(arg->front()->location());
      }

      if (bdoc_results.empty() && vdoc_results.empty())
      {
        break;
      }

      prefix.type = LOOKUP_EMPTY;
      prefix.rule = nullptr;
      prefix.index += 1;

      if (!bdoc_results.empty())
      {
        assert(bdoc_results.size() == 1);
        prefix.basedoc = bdoc_results.front() / DataTerm;
        prefix.basedoc = prefix.basedoc->front();
        prefix.type = LOOKUP_BDOC;
        if (prefix.basedoc != BaseObject)
        {
          break;
        }
      }

      for (Node node : vdoc_results)
      {
        if (node == Rule)
        {
          prefix.type = (LookupType)(prefix.type | LOOKUP_RULE);
          prefix.rule = node;
          if ((node / RuleHead)
                ->front()
                ->in(
                  {RuleHeadComp,
                   RuleHeadFunc,
                   RuleHeadObjDynamic,
                   RuleHeadSetDynamic}))
          {
            // these are terminal rules
            prefix.type = LOOKUP_RULE;
            return prefix;
          }
          continue;
        }

        if (node == VirtualDocument)
        {
          prefix.type = (LookupType)(prefix.type | LOOKUP_VDOC);
          prefix.virtualdoc = node;
          continue;
        }

        throw std::runtime_error("Unknown result type in virtual document");
      }
    }

    if (
      prefix.type == (LOOKUP_BDOC | LOOKUP_VDOC) && prefix.index == 0 &&
      argseq->front() == RefArgDot)
    {
      // this is a (failed) query into the base document
      // we can catch this at compile time, but the expected behaviour is for
      // this to return an undefined value at runtime
      prefix.type = LOOKUP_BDOC;
    }

    if (prefix.type == LOOKUP_VDOC && prefix.index < argseq->size())
    {
      bool has_brack = false;
      for (size_t i = prefix.index; i < argseq->size(); ++i)
      {
        if (argseq->at(i) == RefArgBrack)
        {
          has_brack = true;
          break;
        }
      }

      if (has_brack)
      {
        // this requires a dynamic lookup at runtime
        prefix.index = argseq->size();
        prefix.type = LOOKUP_DYNAMIC;
      }
      else
      {
        // this is a reference to an expected document or rule which does not
        // exist
        prefix.type = LOOKUP_EMPTY;
      }
    }

    return prefix;
  }

  Prefix add_prefix(Node block, Node ref)
  {
    Node head = (ref / RefHead)->front();

    Prefix prefix{LOOKUP_EMPTY, 0, {}, nullptr, nullptr, nullptr};
    if (head == ArgVal)
    {
      Node opblock = head->front();
      block << *(opblock / Block);
      Node localref = (opblock / Operand)->front();
      assert(localref == LocalRef);
      prefix.name = localref->location();
      return prefix;
    }

    prefix = lookup_ref(ref);
    if (prefix.type == LOOKUP_EMPTY)
    {
      if (prefix.index == 0)
      {
        prefix.name = head->location();
        return prefix;
      }

      Node head = (ref / RefHead)->front();
      Node argseq = ref / RefArgSeq;
      assert(
        head->location().view() == "data" ||
        head->location().view() == "input");
      Location current_name = head->location();
      for (size_t i = 0; i < prefix.index; ++i)
      {
        Node arg = argseq->at(i);
        Location child_name = ref->fresh({"dot"});
        block
          << (DotStmt << (Operand << (LocalRef ^ current_name))
                      << (Operand << (IRString ^ arg->front()))
                      << (LocalRef ^ child_name));
        current_name = child_name;
      }

      prefix.name = current_name;
      return prefix;
    }

    if (prefix.type == LOOKUP_LOCAL)
    {
      prefix.name = head->location();
      return prefix;
    }

    prefix.name = ref->fresh({"prefix"});

    // rules are always terminals

    if (prefix.type & LOOKUP_RULE)
    {
      block << rule_stmt(prefix.rule, prefix.name);
      return prefix;
    }

    if (prefix.type == LOOKUP_DYNAMIC)
    {
      block << *rule_dynamic_block(ref, prefix.name);
      return prefix;
    }

    if (prefix.type & LOOKUP_VDOC)
    {
      block << (MakeObjectStmt << (LocalRef ^ prefix.name))
            << document_stmt(
                 prefix.virtualdoc, prefix.name, ref / RefArgSeq, prefix.index);
      if (prefix.type & LOOKUP_BDOC)
      {
        Location bdoc_name = ref->fresh({"bdoc"});
        block << *base_block(prefix.basedoc, bdoc_name)
              << (ObjectMergeStmt << (LocalRef ^ bdoc_name)
                                  << (LocalRef ^ prefix.name)
                                  << (LocalRef ^ prefix.name));
      }
      else
      {
        Nodes idents;
        Node current = prefix.virtualdoc;
        while (current)
        {
          idents.push_back(current / Ident);
          current = current->parent(VirtualDocument);
        }

        Node base_block = NodeDef::create(Block);
        idents.pop_back();
        Location base_name("data");
        while (!idents.empty())
        {
          Node ident = idents.back();
          idents.pop_back();
          Location child_name = ref->fresh({"base"});
          base_block
            << (DotStmt << (Operand << (LocalRef ^ base_name))
                        << (Operand << (IRString ^ ident))
                        << (LocalRef ^ child_name));
          base_name = child_name;
        }

        block
          << (BlockStmt
              << (BlockSeq
                  << base_block
                  << (Block << (IsDefinedStmt << (LocalRef ^ base_name))
                            << (ObjectMergeStmt << (LocalRef ^ base_name)
                                                << (LocalRef ^ prefix.name)
                                                << (LocalRef ^ prefix.name)))));
      }
    }
    else if (prefix.type == LOOKUP_BDOC)
    {
      block << *base_block(prefix.basedoc, prefix.name);
    }

    return prefix;
  }
}

namespace rego
{
  Node jsonstring_to_opblock(Node term)
  {
    std::string json_string(term->location().view());
    json_string = strip_quotes(json_string);
    return OpBlock << (Operand << (IRString ^ json_string)) << Block;
  }

  Node boolean_to_opblock(Node term)
  {
    return OpBlock << (Operand << (Boolean ^ term)) << Block;
  }

  Node number_to_opblock(Node term)
  {
    Location number_name = term->fresh({"number"});
    return OpBlock << (Operand << (LocalRef ^ number_name))
                   << (Block
                       << (MakeNumberRefStmt << (IRString ^ term)
                                             << (LocalRef ^ number_name)));
  }

  Node null_to_opblock(Node term)
  {
    Location null_name = term->fresh({"null"});
    return OpBlock << (Operand << (LocalRef ^ null_name))
                   << (Block << (MakeNullStmt << (LocalRef ^ null_name)));
  }

  Node scalar_to_opblock(Node scalar)
  {
    assert(scalar == Scalar);
    Node value = scalar->front();
    if (value == JSONString)
    {
      return jsonstring_to_opblock(value);
    }

    if (value->in({Int, Float}))
    {
      return number_to_opblock(value);
    }

    if (value->in({True, False}))
    {
      return boolean_to_opblock(value);
    }

    if (value == Null)
    {
      return null_to_opblock(value);
    }

    return err(value, "Not a scalar");
  }

  Node object_to_opblock(Node object)
  {
    Location obj_name = object->fresh({"object"});
    Node block = Block << (MakeObjectStmt << (LocalRef ^ obj_name));
    for (Node item : *object)
    {
      Node key = to_operand(block, (item / Key));
      Node val = to_operand(block, (item / Val));
      block << (ObjectInsertStmt << key << val << (LocalRef ^ obj_name));
    }

    return OpBlock << (Operand << (LocalRef ^ obj_name)) << block;
  }

  Node array_to_opblock(Node array)
  {
    Location array_name = array->fresh({"array"});
    size_t capacity = array->size() / 2;
    Node block = Block
      << (MakeArrayStmt << (Int32 ^ std::to_string(capacity))
                        << (LocalRef ^ array_name));
    for (Node expr : *array)
    {
      Node element = to_operand(block, expr);
      block << (ArrayAppendStmt << (LocalRef ^ array_name) << element);
    }

    return OpBlock << (Operand << (LocalRef ^ array_name)) << block;
  }

  Node set_to_opblock(Node set)
  {
    Location set_name = set->fresh({"set"});
    size_t capacity = set->size() / 2;
    Node block = Block << (MakeSetStmt << (LocalRef ^ set_name));
    for (Node expr : *set)
    {
      Node element = to_operand(block, expr);
      block << (SetAddStmt << element << (LocalRef ^ set_name));
    }

    return OpBlock << (Operand << (LocalRef ^ set_name)) << block;
  }

  Node membership_to_opblock(Node membership)
  {
    Location membership_name = membership->fresh({"membership"});
    Node block = NodeDef::create(Block);
    Node key = membership / Key;
    Node val = membership / Val;
    Node itemseq = to_operand(block, membership / Expr);

    if (key == Undefined)
    {
      Node item = to_operand(block, val);
      block
        << (BuiltInCallStmt << (IRString ^ "internal.member_2")
                            << (OperandSeq << item << itemseq)
                            << (LocalRef ^ membership_name));
      return OpBlock << (Operand << (LocalRef ^ membership_name)) << block;
    }

    Node index = to_operand(block, key);
    Node item = to_operand(block, val);
    block
      << (BuiltInCallStmt << (IRString ^ "internal.member_3")
                          << (OperandSeq << index << item << itemseq)
                          << (LocalRef ^ membership_name));
    return OpBlock << (Operand << (LocalRef ^ membership_name)) << block;
  }

  Node arraycompr_to_opblock(Node arraycompr)
  {
    Location arraycompr_name = arraycompr->fresh({"arraycompr"});
    Node query = arraycompr / Query;
    Node block =
      Block << (MakeArrayStmt << (Int32 ^ "1") << (LocalRef ^ arraycompr_name));

    Node queryblock = NodeDef::create(Block);
    for (Node literal : *query)
    {
      add_literal_to_block(queryblock, literal);
    }

    Node inner = get_inner(queryblock);

    Node item = to_operand(inner, arraycompr / Val);
    inner << (ArrayAppendStmt << (LocalRef ^ arraycompr_name) << item);
    block << (BlockStmt << (BlockSeq << queryblock));
    return OpBlock << (Operand << (LocalRef ^ arraycompr_name)) << block;
  }

  Node setcompr_to_opblock(Node setcompr)
  {
    Location setcompr_name = setcompr->fresh({"setcompr"});
    Node query = setcompr / Query;
    Node block = Block << (MakeSetStmt << (LocalRef ^ setcompr_name));

    Node queryblock = NodeDef::create(Block);
    for (Node literal : *query)
    {
      add_literal_to_block(queryblock, literal);
    }

    Node inner = get_inner(queryblock);

    Node item = to_operand(inner, setcompr / Val);
    inner << (SetAddStmt << item << (LocalRef ^ setcompr_name));
    block << (BlockStmt << (BlockSeq << queryblock));
    return OpBlock << (Operand << (LocalRef ^ setcompr_name)) << block;
  }

  Node objectcompr_to_opblock(Node objectcompr)
  {
    Location objectcompr_name = objectcompr->fresh({"objectcompr"});
    Node query = objectcompr / Query;
    Node block = Block << (MakeObjectStmt << (LocalRef ^ objectcompr_name));

    Node queryblock = NodeDef::create(Block);
    for (Node literal : *query)
    {
      add_literal_to_block(queryblock, literal);
    }

    Node inner = get_inner(queryblock);

    Node key = to_operand(inner, objectcompr / Key);
    Node value = to_operand(inner, objectcompr / Val);
    inner
      << (ObjectInsertOnceStmt << key << value
                               << (LocalRef ^ objectcompr_name));
    block << (BlockStmt << (BlockSeq << queryblock));
    return OpBlock << (Operand << (LocalRef ^ objectcompr_name)) << block;
  }

  Node var_to_opblock(Node var)
  {
    assert(var->in({Var, UnifyVar}));
    Nodes results = var->lookup();
    if (results.empty() || results.front() == Local)
    {
      return OpBlock << (Operand << (LocalRef ^ var)) << Block;
    }

    Location var_name = var->fresh({"var"});
    if (results.front() == Rule)
    {
      return OpBlock << (Operand << (LocalRef ^ var_name))
                     << (Block << rule_stmt(results.front(), var_name));
    }

    Node vdoc = results.front();
    if (vdoc == BaseDocument && results.size() == 2)
    {
      assert(var->location().view() == "data");
      vdoc = results[1];
    }

    if (vdoc == VirtualDocument)
    {
      return OpBlock << (Operand << (LocalRef ^ var_name))
                     << (Block << (MakeObjectStmt << (LocalRef ^ var_name))
                               << document_stmt(vdoc, var_name, nullptr, 0));
    }

    return err(var, "Invalid var");
  }

  Node ref_to_opblock(Node ref)
  {
    assert(ref == Ref);

    Node block = NodeDef::create(Block);
    Prefix prefix = add_prefix(block, ref);

    Node argseq = ref / RefArgSeq;
    Location ref_name = prefix.name;
    for (size_t index = prefix.index; index < argseq->size(); ++index)
    {
      Node arg = argseq->at(index);
      if (arg == RefArgDot)
      {
        Location dot_name = ref->fresh({"dot"});
        block
          << (DotStmt << (Operand << (LocalRef ^ ref_name))
                      << (Operand << (IRString ^ arg->front()))
                      << (LocalRef ^ dot_name));
        ref_name = dot_name;
        continue;
      }

      assert(arg == RefArgBrack);
      Node opblock = arg->front();
      assert(opblock == OpBlock);
      Location brack_name = ref->fresh({"brack"});
      block << *(opblock / Block)
            << (DotStmt << (Operand << (LocalRef ^ ref_name))
                        << (opblock / Operand) << (LocalRef ^ brack_name));
      ref_name = brack_name;
    }

    return OpBlock << (Operand << (LocalRef ^ ref_name)) << block;
  }

  Node term_to_opblock(Node term)
  {
    assert(term == Term);
    Node value = term->front();
    if (value == Scalar)
    {
      return scalar_to_opblock(value);
    }

    if (value == Array)
    {
      return array_to_opblock(value);
    }

    if (value == Object)
    {
      return object_to_opblock(value);
    }

    if (value == Set)
    {
      return set_to_opblock(value);
    }

    if (value == Membership)
    {
      return membership_to_opblock(value);
    }

    if (value == ArrayCompr)
    {
      return arraycompr_to_opblock(value);
    }

    if (value == ObjectCompr)
    {
      return objectcompr_to_opblock(value);
    }

    if (value == SetCompr)
    {
      return setcompr_to_opblock(value);
    }

    if (value->in({Var, UnifyVar}))
    {
      return var_to_opblock(value);
    }

    if (value->in({Ref}))
    {
      return ref_to_opblock(value);
    }

    return err(value, "Invalid term");
  }

  Node call_to_opblock(Node call)
  {
    Node ref = to_absolute_path(call / Ref);
    Node exprseq = call / ExprSeq;
    Location call_name = call->fresh({"call"});
    Node block = NodeDef::create(Block);
    Node argseq = OperandSeq << (Operand << (LocalRef ^ "input"))
                             << (Operand << (LocalRef ^ "data"));
    for (Node expr : *exprseq)
    {
      argseq << to_operand(block, expr);
    }

    block << (CallStmt << ref << argseq << (LocalRef ^ call_name));

    return OpBlock << (Operand << (LocalRef ^ call_name)) << block;
  }

  Node to_operand(Node block, Node operand)
  {
    if (operand == Expr)
    {
      operand = operand->front();
    }

    Node opblock;
    if (operand == Term)
    {
      auto maybe_simple = unwrap(operand, {True, False, JSONString});
      if (maybe_simple.success)
      {
        Node value = maybe_simple.node;
        if (value->in({True, False}))
        {
          return Operand << (Boolean ^ value);
        }

        return Operand << (IRString ^ value);
      }

      opblock = term_to_opblock(operand);
    }
    else if (operand == OpBlock)
    {
      opblock = operand;
    }
    else if (operand == Var)
    {
      opblock = var_to_opblock(operand);
    }
    else if (operand == ExprCall)
    {
      opblock = call_to_opblock(operand);
    }
    else
    {
      logging::Error() << "Invalid operand: " << operand;
      return err(operand, "Invalid operand");
    }

    if (opblock == Error)
    {
      return opblock;
    }

    Node value_op = opblock / Operand;
    Node value_block = opblock / Block;
    block << *value_block;
    return value_op;
  }
}