#include "internal.hh"

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

  BigInt get_int(const Node& node)
  {
    return BigInt(node->location());
  }

  double get_double(const Node& node)
  {
    return std::stod(to_key(node));
  }
}

namespace rego
{
  bool is_in(const Node& node, const std::set<Token>& types)
  {
    if (contains(types, node->type()))
    {
      return true;
    }

    if (node->type() == Top)
    {
      return false;
    }

    return is_in(node->parent()->shared_from_this(), types);
  }

  bool is_constant(const Node& term)
  {
    if (term->type() == NumTerm)
    {
      return true;
    }

    if (term->type() == RefTerm)
    {
      return false;
    }

    Node node = term;
    if (node->type() == Expr)
    {
      node = node->front();
    }

    if (node->type() == Term)
    {
      node = node->front();
    }

    if (node->type() == Scalar)
    {
      return true;
    }

    if (node->type() == Array || node->type() == Set)
    {
      for (auto& child : *node)
      {
        if (!is_constant(child->front()))
        {
          return false;
        }
      }

      return true;
    }

    if (node->type() == Object)
    {
      for (auto& item : *node)
      {
        Node key = item / Key;
        if (!is_constant(key->front()))
        {
          return false;
        }

        Node val = item / Val;
        if (!is_constant(val->front()))
        {
          return false;
        }
      }

      return true;
    }

    return false;
  }

  std::ostream& operator<<(std::ostream& os, const std::set<Location>& locs)
  {
    os << "{";
    join(
      os,
      locs.begin(),
      locs.end(),
      ", ",
      [](std::ostream& stream, const Location& loc) {
        stream << loc.view();
        return true;
      });
    os << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const std::vector<Location>& locs)
  {
    os << "[";
    join(
      os,
      locs.begin(),
      locs.end(),
      ", ",
      [](std::ostream& stream, const Location& loc) {
        stream << loc.view();
        return true;
      });
    os << "]";
    return os;
  }

  std::string strip_quotes(const std::string_view& str)
  {
    if (is_quoted(str))
    {
      return std::string(str.substr(1, str.size() - 2));
    }

    return std::string(str);
  }

  Node err(NodeRange& r, const std::string& msg, const std::string& code)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << r) << (ErrorCode ^ code);
  }

  Node err(Node node, const std::string& msg, const std::string& code)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << node->clone())
                 << (ErrorCode ^ code);
  }

  bool all_alnum(const std::string_view& str)
  {
    for (char c : str)
    {
      if (!std::isalnum(c))
      {
        return false;
      }
    }
    return true;
  }

  Node concat_refs(const Node& lhs, const Node& rhs)
  {
    Node ref;
    if (lhs->type() == Var)
    {
      ref = Ref << (RefHead << lhs->clone()) << RefArgSeq;
    }
    else if (lhs->type() == Ref)
    {
      ref = lhs->clone();
    }
    else
    {
      return err(lhs, "invalid reference");
    }

    Node refhead = (rhs / RefHead)->front();
    Node refargseq = rhs / RefArgSeq;
    if (refhead->type() != Var)
    {
      return err(rhs, "cannot concatenate non-var refhead refs");
    }

    (ref / RefArgSeq) << (RefArgDot << refhead->clone());
    for (auto& arg : *refargseq)
    {
      (ref / RefArgSeq) << arg->clone();
    }

    return ref;
  }

  std::string flatten_ref(const Node& ref)
  {
    std::ostringstream buf;
    buf << (ref / RefHead)->front()->location().view();
    for (auto& arg : *(ref / RefArgSeq))
    {
      if (arg->type() == RefArgDot)
      {
        buf << "." << arg->front()->location().view();
      }
      else
      {
        Node index = arg->front();
        if(index == Expr)
        {
          index = index->front();
        }

        if (index == Term)
        {
          index = index->front();
        }

        if (index->type() == Scalar)
        {
          index = index->front();
        }

        Location key = index->location();
        if (index->type() == JSONString)
        {
          key.pos += 1;
          key.len -= 2;
        }

        if (all_alnum(key.view()))
        {
          buf << "." << key.view();
        }
        else
        {
          buf << "[" << index->location().view() << "]";
        }
      }
    }
    return buf.str();
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
    if (node == Int)
    {
      return ::get_int(node);
    }

    if (node == Float)
    {
      double value = ::get_double(node);
      double floor = std::floor(value);
      if (value == floor)
      {
        return BigInt(static_cast<size_t>(floor));
      }
    }

    return std::nullopt;
  }

  std::string get_string(const Node& node)
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

    if (value->type() == JSONString)
    {
      return strip_quotes(value->location().view());
    }

    return std::string(value->location().view());
  }

  bool get_bool(const Node& node)
  {
    assert(node->type() == True || node->type() == False);
    return node->type() == True;
  }

  bool is_falsy(const Node& node)
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
    if (starts_with(name, "rego-"))
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
    auto result = rego::unwrap(node, std::set(m_types.begin(), m_types.end()));
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
      else if (m_types.size() == 1)
      {
        error << "must be " << type_name(m_types[0], m_specify_number);
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

  UnwrapResult unwrap(const Node& node, const std::set<Token>& types)
  {
    Node value = node;
    if (contains(types, value->type()))
    {
      return {value, true};
    }

    if (value->type() == Term)
    {
      value = value->front();
    }

    if (contains(types, value->type()))
    {
      return {value, true};
    }

    if (value->type() == Scalar)
    {
      value = value->front();
    }

    if (contains(types, value->type()))
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
    m_types.push_back(type);
    return *this;
  }

  const std::vector<Token>& UnwrapOpt::types() const
  {
    return m_types;
  }

  UnwrapOpt& UnwrapOpt::types(const std::vector<Token>& types)
  {
    m_types.insert(m_types.end(), types.begin(), types.end());
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

  bool is_ref_to_type(const Node& var, const std::set<Token>& types)
  {
    Nodes defs = var->lookup();
    if (defs.size() > 0)
    {
      return contains(types, defs[0]->type());
    }
    else
    {
      return false;
    }
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

  Node scalar()
  {
    return Resolver::scalar();
  }

  Node object_item(const Node& key_term, const Node& val_term)
  {
    return ObjectItem << Resolver::to_term(key_term)
                      << Resolver::to_term(val_term);
  }

  Node object(const Nodes& object_items)
  {
    return Object << object_items;
  }

  Node array(const Nodes& array_members)
  {
    return Array << array_members;
  }

  Node set(const Nodes& set_members)
  {
    return Set << set_members;
  }

  ActionMetrics::ActionMetrics(const char* file, std::size_t line) :
    m_key{file, line}, m_start(ActionMetrics::clock::now())
  {}

  ActionMetrics::~ActionMetrics()
  {
    if (!contains(ActionMetrics::s_action_info, m_key))
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
}