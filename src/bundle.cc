#include "internal.hh"
#include "rego.hh"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

namespace rego
{
  namespace bundle
  {
    std::ostream& operator<<(std::ostream& stream, const Statement& stmt)
    {
      switch (stmt.type)
      {
        case StatementType::MakeObject:
          return stream << "MakeObjectStmt(target=" << stmt.target << ")";

        case StatementType::MakeArray:
          return stream << "MakeArrayStmt(target=" << stmt.target << ")";

        case StatementType::MakeSet:
          return stream << "MakeSetStmt(target=" << stmt.target << ")";

        case StatementType::MakeNull:
          return stream << "MakeNullStmt(target=" << stmt.target << ")";

        case StatementType::MakeNumberRef:
          return stream << "MakeNumberRefStmt(target=" << stmt.target
                        << ", index=" << stmt.op0.index << ")";

        case StatementType::MakeNumberInt:
          return stream << "MakeNumberIntStmt(target=" << stmt.target
                        << ", value=" << stmt.op0.value << ")";

        case StatementType::AssignInt:
          return stream << "AssignIntStmt(target=" << stmt.target
                        << ", value=" << stmt.op0.value << ")";

        case StatementType::Len:
          return stream << "LenStmt(source=" << stmt.op0
                        << ", target=" << stmt.target << ")";

        case StatementType::IsObject:
          return stream << "IsObjectStmt(source=" << stmt.op0 << ")";

        case StatementType::IsArray:
          return stream << "IsArrayStmt(source=" << stmt.op0 << ")";

        case StatementType::IsSet:
          return stream << "IsSetStmt(source=" << stmt.op0 << ")";

        case StatementType::ResetLocal:
          return stream << "ResetLocalStmt(target=" << stmt.target << ")";

        case StatementType::AssignVarOnce:
          return stream << "AssignVarOnceStmt(target=" << stmt.target
                        << ", source=" << stmt.op0 << ")";

        case StatementType::IsDefined:
          return stream << "IsDefinedStmt(source=" << stmt.target << ")";

        case StatementType::IsUndefined:
          return stream << "IsUndefinedStmt(source=" << stmt.target << ")";

        case StatementType::ReturnLocal:
          return stream << "ReturnLocalStmt(source=" << stmt.target << ")";

        case StatementType::ObjectInsert:
          return stream << "ObjectInsertStmt(object=" << stmt.target
                        << ", key=" << stmt.op0 << ", value=" << stmt.op1
                        << ")";

        case StatementType::ArrayAppend:
          return stream << "ArrayAppendStmt(array=" << stmt.target
                        << ", value=" << stmt.op0 << ")";

        case StatementType::SetAdd:
          return stream << "SetAddStmt(set=" << stmt.target
                        << ", value=" << stmt.op0 << ")";

        case StatementType::Dot:
          return stream << "DotStmt(source=" << stmt.op0 << ", key=" << stmt.op1
                        << ", target=" << stmt.target << ")";

        case StatementType::AssignVar:
          return stream << "AssignVarStmt(target=" << stmt.target
                        << ", source=" << stmt.op0 << ")";

        case StatementType::ResultSetAdd:
          return stream << "ResultSetAddStmt(value=" << stmt.target << ")";

        case StatementType::Equal:
          return stream << "EqualStmt(a=" << stmt.op0 << ", b=" << stmt.op1
                        << ")";

        case StatementType::NotEqual:
          return stream << "NotEqualStmt(a=" << stmt.op0 << ", b=" << stmt.op1
                        << ")";

        case StatementType::ObjectInsertOnce:
          return stream << "ObjectInsertOnceStmt(key=" << stmt.op0
                        << ", value=" << stmt.op1 << ", object=" << stmt.target
                        << ")";

        case StatementType::ObjectMerge:
          return stream << "ObjectMergeStmt(a=" << stmt.op0.index
                        << ", b=" << stmt.op1.index
                        << ", target=" << stmt.target << ")";

        case StatementType::Block:
          return stream << "Block(#blocks=" << stmt.ext->blocks().size() << ")";

        case StatementType::Call:
          stream << "CallStmt(func_name=" << stmt.ext->call().func.view()
                 << ", args=[";
          for (const auto& arg : stmt.ext->call().ops)
          {
            stream << " " << arg;
          }
          return stream << "], result=" << stmt.target << ")";

        case StatementType::CallDynamic:
          stream << "CallDynamicStmt(func_ops=[";
          for (const auto& op : stmt.ext->call_dynamic().path)
          {
            stream << " " << op;
          }

          stream << "], args=[";

          for (const auto& arg : stmt.ext->call_dynamic().ops)
          {
            stream << " " << arg;
          }
          return stream << "], result=" << stmt.target << ")";

        case StatementType::Not:
          return stream << "NotStmt()";

        case StatementType::Scan:
          return stream << "ScanStmt(source=" << stmt.target
                        << ", key=" << stmt.op0 << ", value=" << stmt.op1
                        << ")";

        case StatementType::With:
          stream << "WithStmt(local=" << stmt.target << ", path=[";
          for (size_t index : stmt.ext->with().path)
          {
            stream << " " << index;
          }
          return stream << " ], value=" << stmt.op0 << ")";

        case StatementType::Break:
          return stream << "BreakStmt() is unsupported";

        case StatementType::Nop:
          return stream;
      }

      return stream;
    }

    std::ostream& operator<<(std::ostream& stream, const Operand& op)
    {
      switch (op.type)
      {
        case OperandType::Local:
          return stream << "local(" << op.index << ")";

        case OperandType::String:
          return stream << "string(" << op.index << ")";

        case OperandType::False:
          return stream << "boolean(false)";

        case OperandType::True:
          return stream << "boolean(true)";

        case OperandType::Index:
          return stream << "index(" << op.index << ")";

        case OperandType::Value:
          return stream << "value(" << op.value << ")";

        default:
          return stream << "invalid op type:" << (int)op.type;
      }

      return stream;
    }

    StatementExt::StatementExt(CallExt&& ext) : contents(ext) {}

    const CallExt& StatementExt::call() const
    {
      return std::get<CallExt>(contents);
    }

    StatementExt::StatementExt(CallDynamicExt&& ext) : contents(ext) {}

    const CallDynamicExt& StatementExt::call_dynamic() const
    {
      return std::get<CallDynamicExt>(contents);
    }

    StatementExt::StatementExt(WithExt&& ext) : contents(ext) {}

    const WithExt& StatementExt::with() const
    {
      return std::get<WithExt>(contents);
    }

    StatementExt::StatementExt(std::vector<Block>&& ext) : contents(ext) {}

    const std::vector<Block>& StatementExt::blocks() const
    {
      return std::get<std::vector<Block>>(contents);
    }

    StatementExt::StatementExt(Block&& ext) : contents(ext) {}

    const Block& StatementExt::block() const
    {
      return std::get<Block>(contents);
    }

    Statement::Statement() : type(StatementType::Nop), target(0) {}

    Operand::Operand() : type(OperandType::None), value(0) {}

    Operand Operand::from_op(const Node& n)
    {
      Operand op;
      Node v = n->front();
      if (v == LocalIndex)
      {
        op.type = OperandType::Local;
        op.index = to_size(v);
        return op;
      }

      if (v == StringIndex)
      {
        op.type = OperandType::String;
        op.index = to_size(v);
        return op;
      }

      if (v == Boolean)
      {
        if (v->location().view() == "true")
        {
          op.type = OperandType::True;
        }
        else
        {
          op.type = OperandType::False;
        }

        return op;
      }

      throw std::runtime_error("Not an operand");
    }

    Operand Operand::from_index(const Node& n)
    {
      Operand op;
      op.type = OperandType::Index;
      op.index = to_size(n);
      return op;
    }

    Operand Operand::from_value(const Node& n)
    {
      Operand op;
      op.type = OperandType::Value;
      op.value = to_int64(n);
      return op;
    }

    Statement node_to_statement(Node n, std::shared_ptr<size_t> max_index)
    {
      Statement stmt;
      if (n == ArrayAppendStmt)
      {
        stmt.type = StatementType::ArrayAppend;
        stmt.target = to_size(n / Array);
        stmt.op0 = Operand::from_op(n / Val);
        return stmt;
      }

      if (n == AssignIntStmt)
      {
        stmt.type = StatementType::AssignInt;
        stmt.op0 = Operand::from_value(n / Val);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == AssignVarOnceStmt)
      {
        stmt.type = StatementType::AssignVarOnce;
        stmt.op0 = Operand::from_op(n / Src);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == AssignVarStmt)
      {
        stmt.type = StatementType::AssignVar;
        stmt.op0 = Operand::from_op(n / Src);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == BlockStmt)
      {
        stmt.type = StatementType::Block;
        std::vector<Block> blocks;
        Node blockseq = n->front();
        std::transform(
          blockseq->begin(),
          blockseq->end(),
          std::back_inserter(blocks),
          [&](const Node& node) { return node_to_block(node, max_index); });
        stmt.ext = std::make_shared<const StatementExt>(std::move(blocks));
        return stmt;
      }

      if (n == BreakStmt)
      {
        stmt.type = StatementType::Break;
        stmt.op0 = Operand::from_index(n / Idx);
        return stmt;
      }

      if (n == CallStmt)
      {
        stmt.type = StatementType::Call;
        CallExt call;
        call.func = (n / Func)->location();
        Node argseq = n / Args;
        std::transform(
          argseq->begin(),
          argseq->end(),
          std::back_inserter(call.ops),
          [](Node& node) { return Operand::from_op(node); });
        stmt.target = to_size(n / Result);
        stmt.ext = std::make_shared<const StatementExt>(std::move(call));
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == CallDynamicStmt)
      {
        stmt.type = StatementType::CallDynamic;
        CallDynamicExt call;
        Node path = n / Func;
        std::transform(
          path->begin(),
          path->end(),
          std::back_inserter(call.path),
          [](Node& node) { return Operand::from_op(node); });

        Node argseq = n / Args;
        std::transform(
          argseq->begin(),
          argseq->end(),
          std::back_inserter(call.ops),
          [](Node& node) { return Operand::from_op(node); });

        stmt.target = to_size(n / Result);
        stmt.ext = std::make_shared<const StatementExt>(std::move(call));
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == DotStmt)
      {
        stmt.type = StatementType::Dot;
        stmt.op0 = Operand::from_op(n / Src);
        stmt.op1 = Operand::from_op(n / Key);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == EqualStmt)
      {
        stmt.type = StatementType::Equal;
        stmt.op0 = Operand::from_op(n / Lhs);
        stmt.op1 = Operand::from_op(n / Rhs);
        return stmt;
      }

      if (n == IsArrayStmt)
      {
        stmt.type = StatementType::IsArray;
        stmt.op0 = Operand::from_op(n / Src);
        return stmt;
      }

      if (n == IsDefinedStmt)
      {
        stmt.type = StatementType::IsDefined;
        stmt.target = to_size(n / Src);
        return stmt;
      }

      if (n == IsObjectStmt)
      {
        stmt.type = StatementType::IsObject;
        stmt.op0 = Operand::from_op(n / Src);
        return stmt;
      }

      if (n == IsSetStmt)
      {
        stmt.type = StatementType::IsSet;
        stmt.op0 = Operand::from_op(n / Src);
        return stmt;
      }

      if (n == IsUndefinedStmt)
      {
        stmt.type = StatementType::IsUndefined;
        stmt.target = to_size(n / Src);
        return stmt;
      }

      if (n == LenStmt)
      {
        stmt.type = StatementType::Len;
        stmt.op0 = Operand::from_op(n / Src);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == MakeArrayStmt)
      {
        stmt.type = StatementType::MakeArray;
        stmt.op0 = Operand::from_value(n / Capacity);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == MakeNullStmt)
      {
        stmt.type = StatementType::MakeNull;
        stmt.target = to_size(n->front());
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == MakeNumberIntStmt)
      {
        stmt.type = StatementType::MakeNumberInt;
        stmt.op0 = Operand::from_value(n / Val);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == MakeNumberRefStmt)
      {
        stmt.type = StatementType::MakeNumberRef;
        stmt.op0 = Operand::from_index(n / Idx);
        stmt.target = to_size(n / Target);
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == MakeObjectStmt)
      {
        stmt.type = StatementType::MakeObject;
        stmt.target = to_size(n->front());
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == MakeSetStmt)
      {
        stmt.type = StatementType::MakeSet;
        stmt.target = to_size(n->front());
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == NotEqualStmt)
      {
        stmt.type = StatementType::NotEqual;
        stmt.op0 = Operand::from_op(n / Lhs);
        stmt.op1 = Operand::from_op(n / Rhs);
        return stmt;
      }

      if (n == NotStmt)
      {
        stmt.type = StatementType::Not;
        stmt.ext = std::make_shared<const StatementExt>(
          node_to_block(n->front(), max_index));
        return stmt;
      }

      if (n == ObjectInsertOnceStmt)
      {
        stmt.type = StatementType::ObjectInsertOnce;
        stmt.op0 = Operand::from_op(n / Key);
        stmt.op1 = Operand::from_op(n / Val);
        stmt.target = to_size(n / Object);
        return stmt;
      }

      if (n == ObjectInsertStmt)
      {
        stmt.type = StatementType::ObjectInsert;
        stmt.op0 = Operand::from_op(n / Key);
        stmt.op1 = Operand::from_op(n / Val);
        stmt.target = to_size(n / Object);
        return stmt;
      }

      if (n == ObjectMergeStmt)
      {
        stmt.type = StatementType::ObjectMerge;
        stmt.op0 = Operand::from_index(n / Lhs);
        stmt.op1 = Operand::from_index(n / Rhs);
        stmt.target = to_size(n / Target);
        return stmt;
      }

      if (n == ResetLocalStmt)
      {
        stmt.type = StatementType::ResetLocal;
        stmt.target = to_size(n->front());
        *max_index = MAX(*max_index, stmt.target);
        return stmt;
      }

      if (n == ResultSetAddStmt)
      {
        stmt.type = StatementType::ResultSetAdd;
        stmt.target = to_size(n / Val);
        return stmt;
      }

      if (n == ReturnLocalStmt)
      {
        stmt.type = StatementType::ReturnLocal;
        stmt.target = to_size(n / Src);
        return stmt;
      }

      if (n == ScanStmt)
      {
        stmt.type = StatementType::Scan;
        stmt.target = to_size(n / Src);
        stmt.op0 = Operand::from_index(n / Key);
        *max_index = MAX(*max_index, stmt.op0.index);
        stmt.op1 = Operand::from_index(n / Val);
        *max_index = MAX(*max_index, stmt.op1.index);
        stmt.ext = std::make_shared<const StatementExt>(
          node_to_block(n->back(), max_index));
        return stmt;
      }

      if (n == SetAddStmt)
      {
        stmt.type = StatementType::SetAdd;
        stmt.op0 = Operand::from_op(n / Val);
        stmt.target = to_size(n / Set);
        return stmt;
      }

      if (n == WithStmt)
      {
        stmt.type = StatementType::With;
        WithExt with;
        stmt.target = to_size(n / LocalIndex);
        Node path = n / Path;
        std::transform(
          path->begin(), path->end(), std::back_inserter(with.path), to_size);
        stmt.op0 = Operand::from_op(n / Val);
        with.block = node_to_block(n->back(), max_index);
        stmt.ext = std::make_shared<const StatementExt>(std::move(with));
        return stmt;
      }

      throw std::runtime_error("Unsupported statement");
    }

    Block node_to_block(Node block, std::shared_ptr<size_t> max_index)
    {
      Block b;
      std::transform(
        block->begin(),
        block->end(),
        std::back_inserter(b),
        [&](const Node& node) { return node_to_statement(node, max_index); });
      return b;
    }

    Function node_to_function(Node func, std::shared_ptr<size_t> max_index)
    {
      Function f;
      f.name = (func / Name)->location();
      for (Node s : *(func / IRPath))
      {
        f.path.push_back(s->location());
      }

      for (Node p : *(func / ParameterSeq))
      {
        f.parameters.push_back(to_size(p));
      }

      f.arity = f.parameters.size();
      f.cacheable = f.arity == 2;
      f.result = to_size(func / Return);

      Node blockseq = func / BlockSeq;
      std::transform(
        blockseq->begin(),
        blockseq->end(),
        std::back_inserter(f.blocks),
        [&](const Node& node) { return node_to_block(node, max_index); });
      return f;
    }

    Plan node_to_plan(Node plan, std::shared_ptr<size_t> max_index)
    {
      Plan p;
      p.name = (plan / Name)->location();
      Node blockseq = plan / BlockSeq;
      std::transform(
        blockseq->begin(),
        blockseq->end(),
        std::back_inserter(p.blocks),
        [&](const Node& node) { return node_to_block(node, max_index); });
      return p;
    }
  }

  Bundle BundleDef::from_node(Node node)
  {
    BundleDef bundle;
    WFContext ctx(wf_bundle);
    bundle.data = (node / Data)->front();
    Location query_entrypoint = (node / Policy / IRQuery)->front()->location();
    Node policy = node / Policy;
    Node query = policy / IRQuery;
    if (query->front() != Undefined)
    {
      bundle.query = SourceDef::synthetic(
        std::string(query->front()->location().view()), "query");
    }

    Node strings = policy / Static / StringSeq;
    logging::Debug() << "Strings:";
    {
      logging::LocalIndent indent;
      for (size_t i = 0; i < strings->size(); ++i)
      {
        const Location& loc = strings->at(i)->location();
        logging::Debug() << i << " " << loc.view();
        bundle.strings.push_back(loc);
      }
    }

    logging::Debug() << "Files:";
    Node modulefileseq = node / ModuleFileSeq;
    {
      logging::LocalIndent indent;
      for (Node file : *modulefileseq)
      {
        std::string name((file / Name)->location().view());
        logging::Debug() << name;
        std::string contents((file / Contents)->location().view());
        bundle.files.push_back(SourceDef::synthetic(contents, name));
      }
    }

    auto max_index = std::make_shared<size_t>(2);

    logging::Debug() << "Functions: ";
    Node functions = policy / FunctionSeq;
    {
      logging::LocalIndent indent;

      for (Node func : *functions)
      {
        Location name = (func / Name)->location();
        logging::Debug() << name.view();
        bundle.name_to_func[name] = bundle.name_to_func.size();
      }

      std::transform(
        functions->begin(),
        functions->end(),
        std::back_inserter(bundle.functions),
        [&, max_index](const Node& n) {
          return bundle::node_to_function(n, max_index);
        });
    }

    logging::Debug() << "Plans:";
    Node plans = policy / PlanSeq;
    {
      logging::LocalIndent indent;

      for (Node plan : *plans)
      {
        Location name = (plan / Name)->location();
        logging::Debug() << name.view();
        size_t index = bundle.name_to_plan.size();
        bundle.name_to_plan[name] = index;
        if (name == query_entrypoint)
        {
          bundle.query_plan = index;
        }
      }

      std::transform(
        plans->begin(),
        plans->end(),
        std::back_inserter(bundle.plans),
        [&, max_index](const Node& n) {
          return bundle::node_to_plan(n, max_index);
        });
    }

    bundle.local_count = *max_index + 1;

    logging::Debug() << "Built-In Functions:";
    Node builtins = policy / Static / BuiltInFunctionSeq;

    for (Node bi : *builtins)
    {
      Location name = (bi / Name)->location();
      logging::Debug() << name.view();
      bundle.builtin_functions[name] = bi / builtins::Decl;
    }

    return std::make_shared<BundleDef>(std::move(bundle));
  }

  std::optional<size_t> BundleDef::find_plan(const Location& name) const
  {
    logging::Debug() << name.view();
    for (auto& [name, _] : name_to_plan)
    {
      logging::Debug() << name.view();
    }
    auto it = name_to_plan.find(name);
    if (it == name_to_plan.end())
    {
      return std::nullopt;
    }

    return it->second;
  }

  std::optional<size_t> BundleDef::find_function(const Location& name) const
  {
    auto it = name_to_func.find(name);
    if (it == name_to_func.end())
    {
      return std::nullopt;
    }

    return it->second;
  }

  bool BundleDef::is_function(const Location& name) const
  {
    return name_to_func.find(name) != name_to_func.end();
  }

  void BundleDef::save(const std::filesystem::path& path) const
  {
    std::ofstream stream(path, std::ios::out | std::ios::binary);
    stream.imbue(std::locale::classic());
    save(stream);
  }

  Bundle BundleDef::load(const std::filesystem::path& path)
  {
    std::ifstream stream(path, std::ios::in | std::ios::binary);
    if (!stream.is_open())
    {
      logging::Error() << "Unable to open bundle file: " << path;
      return nullptr;
    }

    stream.imbue(std::locale::classic());
    return load(stream);
  }
}