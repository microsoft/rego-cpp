#include "internal.hh"

#include "rego.hh"

#include <initializer_list>
#include <trieste/json.h>

namespace logging = trieste::logging;

#ifdef _WIN32
#include <codecvt>
#include <windows.h>

typedef std::map<std::string, std::string> environment_t;
environment_t get_env()
{
  environment_t env;

  using convert_type = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_type, wchar_t> converter;

  auto free = [](wchar_t* p) { FreeEnvironmentStringsW(p); };
  auto env_block =
    std::unique_ptr<wchar_t, decltype(free)>{GetEnvironmentStringsW(), free};

  for (const wchar_t* name = env_block.get(); *name != L'\0';)
  {
    const wchar_t* equal = wcschr(name, L'=');
    std::wstring keyw(name, equal - name);

    const wchar_t* pValue = equal + 1;
    std::wstring valuew(pValue);

    std::string key = converter.to_bytes(keyw);
    std::string value = converter.to_bytes(valuew);

    env[key] = value;

    name = pValue + value.length() + 1;
  }

  return env;
}
#else
extern char** environ;

std::map<std::string, std::string> get_env()
{
  std::map<std::string, std::string> env;
  for (size_t i = 0; environ[i] != NULL; i++)
  {
    std::string env_str = environ[i];
    size_t pos = env_str.find('=');
    if (pos != std::string::npos)
    {
      std::string key = env_str.substr(0, pos);
      std::string value = env_str.substr(pos + 1);
      env[key] = value;
    }
  }
  return env;
}
#endif

namespace
{
  using namespace rego;

  bool is_undefined(const Node& node)
  {
    Nodes frontier({node});
    while (!frontier.empty())
    {
      Node current = frontier.back();
      frontier.pop_back();

      if (node->type() == DataModule)
      {
        return false;
      }

      if (node->type() == Undefined)
      {
        return true;
      }

      frontier.insert(frontier.end(), current->begin(), current->end());
    }

    return false;
  }

  BigInt get_int(const Node& node)
  {
    return BigInt(node->location());
  }

  double get_double(const Node& node)
  {
    return std::stod(to_key(node));
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
}

namespace rego
{
  bool is_constant(const Node& term)
  {
    Nodes frontier({term});
    while (!frontier.empty())
    {
      Node node = frontier.back();
      frontier.pop_back();

      if (node == Expr)
      {
        node = node->front();
      }

      if (node == Term)
      {
        node = node->front();
      }

      if (node == Scalar)
      {
        continue;
      }

      if (node->in({Array, Object, ObjectItem, Set}))
      {
        frontier.insert(frontier.end(), node->begin(), node->end());
        continue;
      }

      return false;
    }

    return true;
  }

  std::string strip_quotes(const std::string_view& str)
  {
    if (is_quoted(str))
    {
      return std::string(str.substr(1, str.size() - 2));
    }

    return std::string(str);
  }

  std::string add_quotes(const std::string_view& str)
  {
    if (is_quoted(str))
    {
      return std::string(str);
    }

    std::string quoted(str.size() + 2, '"');
    std::copy(str.begin(), str.end(), quoted.begin() + 1);
    return quoted;
  }

  std::string get_code(const std::string& msg, const std::string& code)
  {
    if (code == UnknownError)
    {
      if (msg.starts_with("Recursion"))
      {
        return RecursionError;
      }

      return WellFormedError;
    }

    return code;
  }

  Node err(NodeRange& r, const std::string& msg, const std::string& code)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << r)
                 << (ErrorCode ^ get_code(msg, code));
  }

  Node err(Node node, const std::string& msg, const std::string& code)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << node->clone())
                 << (ErrorCode ^ get_code(msg, code));
  }

  Node version()
  {
    Node object = NodeDef::create(Object);
    object->push_back(
      ObjectItem << Resolver::term("commit")
                 << Resolver::term(REGOCPP_GIT_HASH));
    object->push_back(
      ObjectItem << Resolver::term("regocpp_version")
                 << Resolver::term(REGOCPP_VERSION));
    object->push_back(
      ObjectItem << Resolver::term("version")
                 << Resolver::term(REGOCPP_OPA_VERSION));
    Node env = NodeDef::create(Object);
    std::map<std::string, std::string> env_map = get_env();
    for (auto& [key, value] : env_map)
    {
      env->push_back(
        ObjectItem << Resolver::term(key) << Resolver::term(value));
    }
    object->push_back(ObjectItem << Resolver::term("env") << env);
    return object;
  }

  BigInt get_int(const Node& node)
  {
    assert(node->type() == Int);
    return ::get_int(node);
  }

  double get_double(const Node& node)
  {
    assert(node->type() == Float || node->type() == Int);
    return ::get_double(node);
  }

  std::optional<BigInt> try_get_int(const Node& node)
  {
    auto maybe_int = unwrap(node, {Int, Float});
    if (!maybe_int.success)
    {
      return std::nullopt;
    }

    if (node == Int)
    {
      return ::get_int(maybe_int.node);
    }

    double value = ::get_double(maybe_int.node);
    double floor = std::floor(value);
    if (value == floor)
    {
      return BigInt(static_cast<size_t>(floor));
    }

    return std::nullopt;
  }

  std::optional<double> try_get_double(const Node& node)
  {
    auto maybe_float = unwrap(node, {Int, Float});
    if (!maybe_float.success)
    {
      return std::nullopt;
    }

    return ::get_double(maybe_float.node);
  }

  std::string get_string(const Node& node)
  {
    assert(node == JSONString);
    return strip_quotes(node->location().view());
  }

  std::string_view get_raw_string(const Node& node)
  {
    if (node->location().len > 0)
    {
      return node->location().view();
    }

    Node value = node;
    while (value->size() == 1)
    {
      value = value->front();
      if (value->location().len > 0)
      {
        return value->location().view();
      }
    }

    return value->location().view();
  }

  std::optional<std::string> try_get_string(const Node& node)
  {
    auto maybe_string = unwrap(node, JSONString);
    if (!maybe_string.success)
    {
      return std::nullopt;
    }

    return get_string(maybe_string.node);
  }

  bool get_bool(const Node& node)
  {
    assert(node->type() == True || node->type() == False);
    return node->type() == True;
  }

  std::optional<bool> try_get_bool(const Node& node)
  {
    auto maybe_bool = unwrap(node, {True, False});
    if (!maybe_bool.success)
    {
      return std::nullopt;
    }

    return get_bool(maybe_bool.node);
  }

  std::optional<Node> try_get_item(
    const Node& node, const std::string_view& key)
  {
    auto maybe_object = unwrap(node, Object);
    if (!maybe_object.success)
    {
      return std::nullopt;
    }

    WFContext ctx(wf_result);
    auto defs = Resolver::object_lookdown(maybe_object.node, key);
    if (defs.empty())
    {
      return std::nullopt;
    }

    return defs[0];
  }

  bool is_falsy(const Node& node)
  {
    Node value = node;

    if (value->type() == Error)
    {
      return false;
    }

    if (value->type() == Term)
    {
      value = value->front();
    }

    if (value->type() == Scalar)
    {
      value = value->front();
    }

    if (value->type() == False)
    {
      return true;
    }

    if (is_undefined(value))
    {
      return true;
    }

    return false;
  }

  std::string type_name(const Token& type, bool specify_number)
  {
    if (type == Int)
    {
      if (specify_number)
      {
        return "integer number";
      }
      return "number";
    }

    if (type == Float)
    {
      if (specify_number)
      {
        return "floating-point number";
      }
      return "number";
    }

    if (type == JSONString)
    {
      return "string";
    }

    if (type == True || type == False)
    {
      return "boolean";
    }

    std::string name(type.str());
    if (name.starts_with("rego-"))
    {
      name = name.substr(5);
    }

    return name;
  }

  std::string type_name(const Node& node, bool specify_number)
  {
    Node value = node;
    if (value->type() == Term)
    {
      value = value->front();
    }

    if (value->type() == Scalar)
    {
      value = value->front();
    }

    return type_name(value->type(), specify_number);
  }

  Node unwrap_arg(const Nodes& args, const UnwrapOpt& options)
  {
    return options.unwrap(args);
  }

  Node UnwrapOpt::unwrap(const Nodes& args) const
  {
    Node node = args[m_index];
    UnwrapResult result;
    if (m_types.empty())
    {
      result = rego::unwrap(node, m_type);
    }
    else
    {
      result =
        rego::unwrap(node, std::set<Token>(m_types.begin(), m_types.end()));
    }

    if (result.success)
    {
      return result.node;
    }

    if (!m_message.empty())
    {
      return err(node, m_message, m_code);
    }

    std::ostringstream error;
    if (!m_func.empty())
    {
      error << m_func << ": ";
    }

    if (m_prefix.empty())
    {
      error << "operand " << m_index + 1 << " ";
      if (m_types.size() > 1)
      {
        error << "must be one of {";
        join(
          error,
          m_types.begin(),
          m_types.end(),
          ", ",
          [&](std::ostream& stream, const Token& type) {
            stream << type_name(type, m_specify_number);
            return true;
          });
        error << "}";
      }
      else if (m_types.empty() && m_type.def != nullptr)
      {
        error << "must be " << type_name(m_type, m_specify_number);
      }
      else
      {
        error << "must be <type unspecified>";
      }
    }

    if (!m_exclude_got)
    {
      error << " but got " << type_name(result.node->type(), m_specify_number);
    }
    return err(node, error.str(), m_code);
  }

  UnwrapResult unwrap(const Node& node, const Token& type)
  {
    Node value = node;
    if (value == type)
    {
      return {value, true};
    }

    if (value == Term)
    {
      value = value->front();
    }

    if (value == type)
    {
      return {value, true};
    }

    if (value == Scalar)
    {
      value = value->front();
    }

    if (value == type)
    {
      return {value, true};
    }

    return {value, false};
  }

  UnwrapResult unwrap(const Node& node, const std::set<Token>& types)
  {
    Node value = node;
    if (types.find(value->type()) != types.end())
    {
      return {value, true};
    }

    if (value->type() == Term)
    {
      value = value->front();
    }

    if (types.find(value->type()) != types.end())
    {
      return {value, true};
    }

    if (value->type() == Scalar)
    {
      value = value->front();
    }

    if (types.find(value->type()) != types.end())
    {
      return {value, true};
    }

    return {value, false};
  }

  bool is_instance(const Node& value, const std::set<Token>& types)
  {
    return unwrap(value, types).success;
  }

  UnwrapOpt::UnwrapOpt(std::size_t index) :
    m_exclude_got(false),
    m_specify_number(false),
    m_code(EvalTypeError),
    m_index(index)
  {}

  bool UnwrapOpt::exclude_got() const
  {
    return m_exclude_got;
  }

  UnwrapOpt& UnwrapOpt::exclude_got(bool exclude_got)
  {
    m_exclude_got = exclude_got;
    return *this;
  }

  bool UnwrapOpt::specify_number() const
  {
    return m_specify_number;
  }

  UnwrapOpt& UnwrapOpt::specify_number(bool specify_number)
  {
    m_specify_number = specify_number;
    return *this;
  }

  const std::string& UnwrapOpt::code() const
  {
    return m_code;
  }

  UnwrapOpt& UnwrapOpt::code(const std::string& code)
  {
    m_code = code;
    return *this;
  }

  const std::string& UnwrapOpt::pre() const
  {
    return m_prefix;
  }

  UnwrapOpt& UnwrapOpt::pre(const std::string& prefix)
  {
    m_prefix = prefix;
    return *this;
  }

  const std::string& UnwrapOpt::message() const
  {
    return m_message;
  }

  UnwrapOpt& UnwrapOpt::message(const std::string& message)
  {
    m_message = message;
    return *this;
  }

  UnwrapOpt& UnwrapOpt::type(const Token& type)
  {
    m_types.clear();
    m_type = type;
    return *this;
  }

  const std::vector<Token>& UnwrapOpt::types() const
  {
    return m_types;
  }

  UnwrapOpt& UnwrapOpt::types(const std::vector<Token>& types)
  {
    if (types.size() == 1)
    {
      m_type = types.front();
      m_types.clear();
      return *this;
    }

    m_types = std::vector<Token>(types.begin(), types.end());
    return *this;
  }

  const std::string& UnwrapOpt::func() const
  {
    return m_func;
  }

  UnwrapOpt& UnwrapOpt::func(const std::string& func)
  {
    m_func = func;
    return *this;
  }

  Node number(double value)
  {
    return Resolver::scalar(value);
  }

  Node boolean(bool value)
  {
    return Resolver::scalar(value);
  }

  Node string(const char* value)
  {
    return Resolver::scalar(value);
  }

  Node string(const std::string& value)
  {
    return Resolver::scalar(value);
  }

  Node null()
  {
    return Resolver::scalar();
  }

  Node scalar(BigInt value)
  {
    return Resolver::scalar(value);
  }

  Node scalar(double value)
  {
    return Resolver::scalar(value);
  }

  Node scalar(bool value)
  {
    return Resolver::scalar(value);
  }

  Node scalar(const char* value)
  {
    return Resolver::scalar(value);
  }

  Node scalar(const std::string& value)
  {
    return Resolver::scalar(value);
  }

  Node object_item(const Node& key_term, const Node& val_term)
  {
    return ObjectItem << Resolver::to_term(key_term)
                      << Resolver::to_term(val_term);
  }

  Node object(const std::initializer_list<Node>& object_items)
  {
    Node result = NodeDef::create(Object);
    for (auto& node : object_items)
    {
      result << node;
    }

    return result;
  }

  Node array(const std::initializer_list<Node>& array_members)
  {
    Node result = NodeDef::create(Array);
    for (auto& node : array_members)
    {
      result << Resolver::to_term(node);
    }

    return result;
  }

  Node set(const std::initializer_list<Node>& set_members)
  {
    Node result = NodeDef::create(Set);
    for (auto& node : set_members)
    {
      result << Resolver::to_term(node);
    }

    return result;
  }

  ActionMetrics::ActionMetrics(const char* file, std::size_t line) :
    m_key{file, line}, m_start(ActionMetrics::clock::now())
  {}

  ActionMetrics::~ActionMetrics()
  {
    if (!ActionMetrics::s_action_info.contains(m_key))
    {
      ActionMetrics::s_action_info.insert(
        {m_key, {0, ActionMetrics::duration::zero()}});
    }

    auto elapsed = ActionMetrics::clock::now() - m_start;
    ActionMetrics::s_action_info[m_key].count++;
    ActionMetrics::s_action_info[m_key].time_spent += elapsed;
  }

  std::map<ActionMetrics::key_t, ActionMetrics::info_t>
    ActionMetrics::s_action_info = {};

  void ActionMetrics::print()
  {
    logging::Output() << "Action\tCount\tTime(ms)";
    for (auto& [key, info] : ActionMetrics::s_action_info)
    {
      std::chrono::duration<double, std::milli> fp_ms = info.time_spent;
      logging::Output() << key.file << ":" << key.line << "\t" << info.count
                        << "\t" << fp_ms.count();
    }
  }

  bool ActionMetrics::key_t::operator<(const ActionMetrics::key_t& other) const
  {
    if (file != other.file)
    {
      return file < other.file;
    }

    return line < other.line;
  }

  bool is_truthy(const Node& node)
  {
    assert(node->type() == Term);

    Node value = node->front();
    if (value->type() == Scalar)
    {
      value = value->front();
      return value->type() != False;
    }

    if (
      value->type() == Object || value->type() == Array || value->type() == Set)
    {
      return true;
    }

    return false;
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

  Node to_constant_term(Node expr)
  {
    Node term = expr->front();
    if (term != Term)
    {
      return nullptr;
    }

    Node value = term->front();
    if (value->in({ArrayCompr, ObjectCompr, SetCompr, Ref, Var, Membership}))
    {
      return nullptr;
    }

    if (value == Scalar)
    {
      return Term << value->clone();
    }

    if (value == Array)
    {
      Node array = NodeDef::create(Array);
      for (Node expr : *value)
      {
        Node term = to_constant_term(expr);
        if (term == nullptr)
        {
          return nullptr;
        }

        array << term;
      }

      return Term << array;
    }

    if (value == Object)
    {
      Node object = NodeDef::create(Object);
      for (Node item : *value)
      {
        Node key = to_constant_term(item / Key);
        if (key == nullptr)
        {
          return nullptr;
        }

        Node value = to_constant_term(item / Val);
        if (value == nullptr)
        {
          return nullptr;
        }

        object << (ObjectItem << key << value);
      }

      return Term << object;
    }

    if (value == Set)
    {
      Node set = NodeDef::create(Array);
      for (Node expr : *value)
      {
        Node term = to_constant_term(expr);
        if (term == nullptr)
        {
          return nullptr;
        }

        set << term;
      }

      return Term << set;
    }

    throw std::runtime_error("Invalid term");
  }

  Node get_inner(Node block)
  {
    if (block->empty())
    {
      return block;
    }

    Node inner = block;
    while (!inner->empty() && inner->back()->in({ScanStmt, WithStmt}))
    {
      Node stmt = inner->back();
      if (stmt == ScanStmt)
      {
        inner = stmt / Block;
        continue;
      }

      // this gets called with two different WF definitions
      // during which the structure of WithStmt may have changed
      Node back = stmt->back();
      if (back == Block)
      {
        inner = back;
        continue;
      }

      if (back == ScanStmt)
      {
        inner = back / Block;
        continue;
      }

      if (back != WithStmt)
      {
        break;
      }

      while (back == WithStmt)
      {
        back = back / Expr;
      }

      if (back == ScanStmt)
      {
        inner = back / Block;
        continue;
      }

      break;
    }

    return inner;
  }

  void add_literal_to_block(Node block, Node literal)
  {
    assert(block == Block);
    assert(literal == Literal);
    Node expr = literal / Expr;
    if (expr == OpBlock)
    {
      block << *(expr / Block);
      get_inner(block)
        << (NotEqualStmt << (expr / Operand)->clone()
                         << (Operand << (Boolean ^ "false")));
    }
    else if (expr == AssignBlock)
    {
      block << *(expr / Block);
    }
    else
    {
      block << *literal;
    }
  }

  Node to_absolute_path(Node ref)
  {
    Node head;
    if (ref == Var)
    {
      head = ref;
    }
    else
    {
      head = (ref / RefHead)->front();
    }

    if (head == ArgVal)
    {
      return nullptr;
    }

    if (head->location().view() == "data")
    {
      return ref;
    }

    Nodes results = head->lookup();
    if (results.empty())
    {
      return ref;
    }

    Node current = results.front();
    assert(current->in({VirtualDocument, Rule, Local}));

    if (current == Local)
    {
      return nullptr;
    }

    Nodes idents;
    while (current)
    {
      idents.push_back(current / Ident);
      current = current->parent(VirtualDocument);
    }

    head = RefHead << (Var ^ idents.back());
    Node args = NodeDef::create(RefArgSeq);
    idents.pop_back();
    while (!idents.empty())
    {
      args << (RefArgDot << (Var ^ idents.back()));
      idents.pop_back();
    }

    if (ref == Ref)
    {
      args << *(ref / RefArgSeq);
    }

    return Ref << head << args;
  }

  Node base_block(Node base_term, const Location& value_name)
  {
    Nodes idents;
    Node current = base_term;
    while (current)
    {
      Node item = current->parent({BaseObjectItem, BaseDocument});
      idents.push_back(item / Ident);
      current = item->parent(BaseObject);
    }

    Location base_name(idents.back()->location());
    idents.pop_back();

    if (idents.empty())
    {
      // looking up the root data document
      return Block
        << (AssignVarStmt << (Operand << (LocalRef ^ base_name))
                          << (LocalRef ^ value_name));
    }

    Node block = NodeDef::create(Block);
    while (!idents.empty())
    {
      Location dot_name =
        idents.size() > 1 ? base_term->fresh({"dot"}) : value_name;
      block
        << (DotStmt << (Operand << (LocalRef ^ base_name))
                    << (Operand << (IRString ^ idents.back()))
                    << (LocalRef ^ dot_name));
      base_name = dot_name;
      idents.pop_back();
    }

    return block;
  }

  bool is_ruletype(Node rule, Token type)
  {
    return (rule / RuleHead)->front() == type;
  }

  bool is_ruletype(Node rule, const std::initializer_list<Token>& types)
  {
    return (rule / RuleHead)->front()->in(types);
  }

  Node rule_stmt(Node rule, const Location& value_name)
  {
    Nodes idents;
    Node current = rule;
    while (current)
    {
      idents.push_back(current / Ident);
      current = current->parent(VirtualDocument);
    }

    Node head = RefHead << (Var ^ idents.back());
    idents.pop_back();
    Node argseq = NodeDef::create(RefArgSeq);
    while (!idents.empty())
    {
      argseq << (RefArgDot << (Var ^ idents.back()));
      idents.pop_back();
    }

    return CallStmt << (Ref << head << argseq)
                    << (OperandSeq << (Operand << (LocalRef ^ "input"))
                                   << (Operand << (LocalRef ^ "data")))
                    << (LocalRef ^ value_name);
  }

  Node rule_dynamic_block(Node ref, const Location& value_name)
  {
    Node func_ops = NodeDef::create(OperandSeq);
    Node head = (ref / RefHead)->front();
    if (head->location().view() != "data")
    {
      Nodes results = head->lookup();
      assert(results.size() == 1 && results.front() == VirtualDocument);
      Node current = results.front();
      std::vector<Location> idents;
      while (current != nullptr)
      {
        idents.push_back((current / Ident)->location());
        current = current->parent(VirtualDocument);
      }

      assert(idents.back().view() == "data");
      while (!idents.empty())
      {
        func_ops << (Operand << (IRString ^ idents.back()));
        idents.pop_back();
      }
    }
    else
    {
      func_ops << (Operand << (IRString ^ "data"));
    }

    Node block = NodeDef::create(Block);
    for (Node arg : *(ref / RefArgSeq))
    {
      if (arg == RefArgDot)
      {
        func_ops << (Operand << (IRString ^ arg->front()));
        continue;
      }

      assert(arg == RefArgBrack && arg->front() == OpBlock);
      Node opblock = arg->front();
      block << *(opblock / Block);
      func_ops << (opblock / Operand)->clone();
    }

    return block
      << (CallDynamicStmt << func_ops
                          << (OperandSeq << (Operand << (LocalRef ^ "input"))
                                         << (Operand << (LocalRef ^ "data")))
                          << (LocalRef ^ value_name));
  }

  struct DocumentPath
  {
    std::string path;
    std::vector<Location> parts;

    bool operator<(const DocumentPath& other) const
    {
      return path < other.path;
    }

    const Location& back() const
    {
      return parts.back();
    }

    size_t size() const
    {
      return parts.size();
    }

    const Location& operator[](size_t i) const
    {
      return parts[i];
    }

    static DocumentPath from_doc(Node doc)
    {
      Location ident = (doc / Ident)->location();
      std::string path(ident.view());
      return {path, {ident}};
    }

    bool match(Node refargseq, size_t start)
    {
      assert(refargseq != nullptr);
      for (size_t i = 1, j = start; i < parts.size() && j < refargseq->size();
           ++i, ++j)
      {
        if (refargseq->at(j) == RefArgBrack)
        {
          continue;
        }

        if (parts[i] != refargseq->at(j)->front()->location())
        {
          return false;
        }
      }

      return true;
    }
  };

  static DocumentPath operator/(const DocumentPath& lhs, const Location& rhs)
  {
    std::vector<Location> parts(lhs.parts.begin(), lhs.parts.end());
    parts.push_back(rhs);
    std::ostringstream path_buf;
    path_buf << lhs.path << '/' << rhs.view();
    return {path_buf.str(), parts};
  }

  typedef std::pair<DocumentPath, Node> DocumentNode;

  Node document_stmt(
    Node document, const Location& doc_name, Node refargseq, size_t start)
  {
    std::map<Location, Location> items;
    Node blockseq = NodeDef::create(BlockSeq);

    std::map<DocumentPath, Nodes> rules_to_add;

    // first we walk the document and find all rules, with the paths
    // through the virtual document to each rule.
    std::vector<DocumentNode> frontier;
    frontier.push_back({DocumentPath::from_doc(document), document});
    while (!frontier.empty())
    {
      auto [path, vdoc] = frontier.back();
      frontier.pop_back();

      if (path.back().view().starts_with("querymodule$"))
      {
        // the query module is never included in these constructions
        continue;
      }

      if (refargseq && !path.match(refargseq, start))
      {
        continue;
      }

      Node rules = vdoc / RuleSeq;
      Nodes docrules;
      if (!rules->empty())
      {
        docrules.insert(docrules.end(), rules->begin(), rules->end());
      }

      rules_to_add[path] = docrules;

      Node children = vdoc / DocumentSeq;
      for (Node child : *children)
      {
        frontier.push_back({path / (child / Ident)->location(), child});
      }
    }

    if (rules_to_add.empty())
    {
      // no rules to add
      return BlockStmt << blockseq;
    }

    // for each rule, add the rule to the document tree
    for (const auto& [path, rules] : rules_to_add)
    {
      // first we need to find the node in the tree at which to add
      // these rules.
      std::vector<Location> name_stack({doc_name});
      for (size_t i = 1; i < path.size(); ++i)
      {
        // we either need to find the existing child document as it
        // was created by a previous ruleset in the list, or create
        // a new one when needed.
        Location child_name = document->fresh({"vdoc"});
        blockseq << (Block
                     << (DotStmt << (Operand << (LocalRef ^ name_stack.back()))
                                 << (Operand << (IRString ^ path[i]))
                                 << (LocalRef ^ child_name)))
                 << (Block << (IsUndefinedStmt << (LocalRef ^ child_name))
                           << (MakeObjectStmt << (LocalRef ^ child_name)));
        name_stack.push_back(child_name);
      }

      // add the rules. Each rule cannot overwrite a value in the document
      // tree, that is, it must be the only thing that writes to this path.
      for (Node rule : rules)
      {
        if (is_ruletype(rule, RuleHeadFunc))
        {
          continue;
        }

        Location rule_ident = (rule / Ident)->location();

        if (refargseq && !(path / rule_ident).match(refargseq, start))
        {
          continue;
        }

        Location value_name = document->fresh({"value"});
        blockseq << (Block << rule_stmt(rule, value_name))
                 << (Block << (IsDefinedStmt << (LocalRef ^ value_name))
                           << (ObjectInsertOnceStmt
                               << (Operand << (IRString ^ rule_ident))
                               << (Operand << (LocalRef ^ value_name))
                               << (LocalRef ^ name_stack.back())));
      }

      if (name_stack.size() == 1)
      {
        // nothing to cleanup
        continue;
      }

      // as we go back up the tree, we need to replace the documents
      // at each level with the updated version from below.
      Node cleanup_block = NodeDef::create(Block);
      for (int i = static_cast<int>(path.size()) - 1; i > 0; --i)
      {
        Location child_name = name_stack.back();
        name_stack.pop_back();
        cleanup_block
          << (ObjectInsertStmt << (Operand << (IRString ^ path[i]))
                               << (Operand << (LocalRef ^ child_name))
                               << (LocalRef ^ name_stack.back()));
      }

      blockseq << cleanup_block;
    }

    return BlockStmt << blockseq;
  }
}