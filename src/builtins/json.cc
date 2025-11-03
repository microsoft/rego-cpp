#include "builtins.hh"

#include <trieste/json.h>

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node marshal(const Nodes& args)
  {
    Node x = Resolver::to_term(args[0]);
    if (x->type() == Error)
    {
      return x;
    }

    auto result = rego_to_json().wf_check_enabled(true).rewrite(Top << x);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(args[0], "failed to marshal JSON", EvalBuiltInError);
    }

    std::string json = json::to_string(result.ast);
    return JSONString ^ add_quotes(json::escape(json));
  }

  BuiltIn marshal_factory()
  {
    const Node marshal_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "the term to serialize")
                               << (bi::Type << bi::Any)))
               << (bi::Result << (bi::Name ^ "y")
                              << (bi::Description ^
                                  "the JSON string representation of `x`")
                              << (bi::Type << bi::String));
    return BuiltInDef::create({"json.marshal"}, marshal_decl, marshal);
  }

  Node marshal_with_options(const Nodes& args)
  {
    Node x = Resolver::to_term(args[0]);
    if (x->type() == Error)
    {
      return x;
    }

    Node opts = unwrap_arg(args, UnwrapOpt(1).type(Object));
    if (opts == Error)
    {
      return opts;
    }

    std::optional<bool> maybe_pretty = std::nullopt;
    std::string indent = "\t";
    std::string prefix = "";

    for (auto& item : *opts)
    {
      auto maybe_key = unwrap(item / Key, JSONString);
      if (!maybe_key.success)
      {
        return err(item, "key must be a string", EvalBuiltInError);
      }

      std::string key = get_string(maybe_key.node);
      if (key == "pretty")
      {
        auto maybe_bool = try_get_bool(item / Val);
        if (!maybe_bool.has_value())
        {
          return err(item, "value must be a boolean", EvalBuiltInError);
        }

        maybe_pretty = maybe_bool.value();
      }
      else if (key == "indent")
      {
        auto maybe_indent = try_get_string(item / Val);
        if (!maybe_indent.has_value())
        {
          return err(item, "value must be a string", EvalBuiltInError);
        }

        indent = maybe_indent.value();
        if (!maybe_pretty.has_value())
        {
          maybe_pretty = true;
        }
      }
      else if (key == "prefix")
      {
        auto maybe_prefix = try_get_string(item / Val);
        if (!maybe_prefix.has_value())
        {
          return err(item, "value must be a string", EvalBuiltInError);
        }

        prefix = maybe_prefix.value();
        if (!maybe_pretty.has_value())
        {
          maybe_pretty = true;
        }
      }
      else
      {
        std::string message = "object contained unknown key \"" + key + '"';
        return err(item, message, EvalTypeError);
      }
    }

    bool pretty = maybe_pretty.value_or(false);

    auto result = rego_to_json().wf_check_enabled(true).rewrite(Top << x);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(args[0], "failed to marshal JSON", EvalBuiltInError);
    }

    std::string json = json::to_string(result.ast, pretty, false, indent);

    if (pretty && !prefix.empty())
    {
      std::ostringstream prefixed;
      prefixed << prefix;
      size_t start = 0;
      size_t end = json.find('\n');
      while (end != std::string::npos)
      {
        prefixed << json.substr(start, end - start) << '\n' << prefix;
        start = end + 1;
        end = json.find('\n', start);
      }
      prefixed << json.substr(start);
      json = prefixed.str();
    }

    return JSONString ^ add_quotes(json::escape(json));
  }

  BuiltIn marshal_with_options_factory()
  {
    const Node marshal_with_options_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "x")
                      << (bi::Description ^ "the term to serialize")
                      << (bi::Type << bi::Any))
          << (bi::Arg
              << (bi::Name ^ "opts") << (bi::Description ^ "encoding options")
              << (bi::Type
                  << (bi::HybridObject
                      << (bi::DynamicObject << (bi::Type << bi::String)
                                            << (bi::Type << bi::Any))
                      << (bi::StaticObject
                          << (bi::ObjectItem << (bi::Name ^ "indent")
                                             << (bi::Type << bi::String))
                          << (bi::ObjectItem << (bi::Name ^ "prefix")
                                             << (bi::Type << bi::String))
                          << (bi::ObjectItem << (bi::Name ^ "pretty")
                                             << (bi::Type << bi::Boolean)))))))
      << (bi::Result
          << (bi::Name ^ "y")
          << (bi::Description ^
              "the JSON string representation of `x`, with configured "
              "prefix/indent string(s) as appropriate")
          << (bi::Type << bi::String));
    return BuiltInDef::create(
      {"json.marshal_with_options"},
      marshal_with_options_decl,
      marshal_with_options);
  }

  Node unmarshal(const Nodes& args)
  {
    Node x = unwrap_arg(args, UnwrapOpt(0).type(JSONString));
    if (x->type() == Error)
    {
      return x;
    }

    std::string x_json = get_string(x);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = json::reader().synthetic(x_raw).wf_check_enabled(true) >>
      json_to_rego(true);
    if (!result.ok)
    {
      logging::Error log;
      result.print_errors(log);
      return err(x, "failed to parse JSON", EvalBuiltInError);
    }

    return result.ast->front();
  }

  BuiltIn unmarshal_factory()
  {
    const Node unmarshal_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "a JSON string")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "y")
                   << (bi::Description ^ "the term deserialized from `x`")
                   << (bi::Type << bi::Any));
    return BuiltInDef::create({"json.unmarshal"}, unmarshal_decl, unmarshal);
  }

  Node is_valid(const Nodes& args)
  {
    auto maybe_x = unwrap(args[0], JSONString);
    if (!maybe_x.success)
    {
      return False ^ "false";
    }

    std::string x_json = get_string(maybe_x.node);
    std::string x_raw = json::unescape(strip_quotes(x_json));
    auto result = json::reader().synthetic(x_raw).wf_check_enabled(true).read();
    return result.ok ? True ^ "true" : False ^ "false";
  }

  BuiltIn is_valid_factory()
  {
    const Node is_valid_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "x")
                               << (bi::Description ^ "a JSON string")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "y")
                   << (bi::Description ^
                       "`true` if `x` is valid JSON, `false` otherwise")
                   << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"json.is_valid"}, is_valid_decl, is_valid);
  }

  namespace pointer
  {
    class Pointer : public std::vector<Location>
    {
    public:
      Pointer(Location path) noexcept : m_path(path)
      {
        size_t pos = 0;
        while (pos < path.len)
        {
          auto maybe_key = next_key(path, pos);
          if (!maybe_key.has_value())
          {
            m_error = err(JSONString ^ path, "Invalid pointer");
            break;
          }

          logging::Trace() << "Pointer[" << size()
                           << "] = " << maybe_key.value().view();

          push_back(maybe_key.value());
        }
      }

      const Location& path() const
      {
        return m_path;
      }

      bool is_valid() const
      {
        return m_error == nullptr;
      }

      Node error() const
      {
        return m_error;
      }

    private:
      static std::optional<Location> next_key(const Location& path, size_t& pos)
      {
        const std::string_view& path_view = path.view();

        if (path_view[pos] != '/')
        {
          if (pos > 0)
          {
            // OPA allows paths without an initial preceding slash
            return std::nullopt;
          }
        }
        else
        {
          pos += 1; // remove the prepending `/`
        }

        size_t end = pos;
        bool needs_replacement = false;
        while (end < path.len)
        {
          if (path_view[end] == '/')
          {
            break;
          }

          if (path_view[end] == '~')
          {
            needs_replacement = true;
          }

          end += 1;
        }

        Location key(path.source, path.pos + pos, end - pos);
        pos = end;
        if (!needs_replacement)
        {
          return key;
        }

        auto maybe_unescaped = unescape_key(key.view());
        if (!maybe_unescaped.has_value())
        {
          return std::nullopt;
        }

        return Location(maybe_unescaped.value());
      }

      static std::optional<std::string> unescape_key(
        const std::string_view& pointer)
      {
        std::ostringstream os;
        size_t index = 0;
        while (index < pointer.size())
        {
          char c = pointer[index];
          assert(c != '/');
          if (c == '~')
          {
            if (index + 1 == pointer.size())
            {
              logging::Error() << "Invalid `~` in pointer";
              return std::nullopt;
            }

            char i = pointer[index + 1];
            if (i == '0')
            {
              os << '~';
            }
            else if (i == '1')
            {
              os << '/';
            }
            else
            {
              logging::Error() << "Invalid escape value '" << i << "'";
              return std::nullopt;
            }

            index += 2;
            continue;
          }

          os << c;
          index += 1;
        }

        return os.str();
      }

      Node m_error;
      Location m_path;
    };

    enum class Action
    {
      Insert,
      Read,
      Replace,
      Remove,
      Compare
    };

    struct DebugAction
    {
      Action action;
    };

    static std::ostream& operator<<(std::ostream& os, const DebugAction debug)
    {
      switch (debug.action)
      {
        case Action::Insert:
          return os << "insert";

        case Action::Read:
          return os << "read";

        case Action::Replace:
          return os << "replace";

        case Action::Remove:
          return os << "remove";

        case Action::Compare:
          return os << "compare";
      }

      throw std::runtime_error("Unrecognized Action value");
    }

    class Operation
    {
    public:
      Operation(Location path, Action action, Node value) :
        m_pointer(path), m_action(action), m_value(value)
      {}

      Operation(Location path, Action action) : Operation(path, action, nullptr)
      {}

      Node run(Node document) const
      {
        if (!m_pointer.is_valid())
        {
          return m_pointer.error();
        }

        WFContext ctx(wf_result);
        std::map<Location, Node> set_lookup;

        if (m_pointer.empty())
        {
          switch (m_action)
          {
            case Action::Read:
              return document;

            case Action::Replace:
              return m_value;

            case Action::Remove:
              return err(
                JSONString ^ m_pointer.path(), "Cannot remove the root node");

            case Action::Compare:
              return compare(document, m_value);

            case Action::Insert:
              return m_value;
          }
        }

        Node current = document;
        for (size_t i = 0; i < m_pointer.size() - 1; ++i)
        {
          const Location& key = m_pointer[i];

          auto maybe_indexable = unwrap(current, {Array, Object, Set});

          if (!maybe_indexable.success)
          {
            return err(current, "Cannot index into value");
          }

          current = maybe_indexable.node;

          if (current == Object)
          {
            current = object_index(current, key);
            if (current == Error)
            {
              return current;
            }

            continue;
          }

          if (current == Set)
          {
            set_lookup.clear();
            current = set_index(current, key, set_lookup);
            if (current == Error)
            {
              return current;
            }

            continue;
          }

          size_t index = 0;
          current = array_index(current, key, index);
          if (current == Error)
          {
            return current;
          }
        }

        auto maybe_indexable = unwrap(current, {Object, Array, Set});

        if (!maybe_indexable.success)
        {
          return err(current, "Cannot index into value");
        }

        current = maybe_indexable.node;
        const Location& key = m_pointer.back();
        if (current == Object)
        {
          return object_action(current, key);
        }

        if (current == Set)
        {
          return set_action(current, key);
        }

        return array_action(current, key);
      }

    private:
      static Node compare(Node actual, Node expected)
      {
        if (actual == Error)
        {
          return actual;
        }

        std::string actual_key = to_key(actual);
        std::string expected_key = to_key(expected);
        if (actual_key != expected_key)
        {
          std::ostringstream os;
          os << actual_key << " != " << expected_key;
          return err(actual, os.str());
        }

        return actual;
      }

      Node object_action(Node object, const Location& key) const
      {
        assert(object == Object);

        logging::Trace() << "Pointer: Object action " << DebugAction{m_action}
                         << " on object " << object << " at key " << key.view();

        Node member;
        Node existing = object_index(object, key);
        if (existing == Error)
        {
          existing = nullptr;
        }
        else
        {
          member = existing->parent();
        }

        switch (m_action)
        {
          case Action::Compare:
            if (existing == nullptr)
            {
              return err(
                object,
                "Member does not exist with key: " + std::string(key.view()));
            }
            return compare(existing, m_value);

          case Action::Insert:
            if (existing == nullptr)
            {
              object << object_item((JSONString ^ key), m_value->clone());
              return m_value;
            }
            else
            {
              member / Val = m_value->clone();
              return existing;
            }

          case Action::Read:
            if (existing == nullptr)
            {
              return err(
                object,
                "Member does not exist with key: " + std::string(key.view()));
            }
            return existing;

          case Action::Remove:
            if (existing == nullptr)
            {
              return err(
                object,
                "Member does not exist with key: " + std::string(key.view()));
            }
            object->replace(member);
            return existing;

          case Action::Replace:
            if (existing == nullptr)
            {
              return err(
                object,
                "Member does not exist with key: " + std::string(key.view()));
            }
            member / Val = m_value->clone();
            return existing;
        }

        throw std::runtime_error("Unsupported Action value");
      }

      Node array_action(Node array, const Location& key) const
      {
        assert(array == Array);
        size_t index = 0;

        logging::Trace() << "Pointer: Array action " << DebugAction{m_action}
                         << " on array " << array << " at index " << key.view();

        Node element = array_index(array, key, index);

        if (m_action == Action::Insert && index == array->size())
        {
          array << m_value->clone();
          return m_value;
        }

        if (element == Error)
        {
          return element;
        }

        switch (m_action)
        {
          case Action::Compare:
            return compare(element, m_value->clone());

          case Action::Insert:
            array->insert(array->begin() + index, m_value->clone());
            return m_value;

          case Action::Read:
            return element;

          case Action::Remove:
            array->replace(element);
            return element;

          case Action::Replace:
            array->replace_at(index, m_value->clone());
            return element;
        }

        throw std::runtime_error("Unsupported Action value");
      }

      Node set_action(Node set, const Location& key) const
      {
        assert(set == Set);

        std::map<Location, Node> set_lookup;
        Node existing = set_index(set, key, set_lookup);

        if (!set_lookup.contains(key))
        {
          existing = nullptr;
        }

        Location value_key;
        if (m_value != nullptr)
        {
          value_key = Location(strip_quotes(to_key(m_value)));
        }

        switch (m_action)
        {
          case Action::Compare:
            if (existing == nullptr)
            {
              return err(
                set,
                "Member does not exist with key: " + std::string(key.view()));
            }
            return compare(existing, m_value);

          case Action::Insert:
            if (existing == nullptr)
            {
              if (key != value_key)
              {
                return err(m_value, "Value does not match specified path");
              }

              set << m_value;
              return m_value;
            }
            else
            {
              return existing;
            }

          case Action::Read:
            if (existing == nullptr)
            {
              return err(
                set,
                "Member does not exist with key: " + std::string(key.view()));
            }
            return existing;

          case Action::Remove:
            if (existing == nullptr)
            {
              return err(
                set,
                "Member does not exist with key: " + std::string(key.view()));
            }
            set->replace(existing);
            return existing;

          case Action::Replace:
            if (existing == nullptr)
            {
              return err(
                set,
                "Member does not exist with key: " + std::string(key.view()));
            }

            if (key != value_key)
            {
              return err(m_value, "Value does not match specified path");
            }

            return existing;
        }

        throw std::runtime_error("Unsupported Action value");
      }

      static Node object_index(const Node& object, const Location& key)
      {
        for (auto& object_item : *object)
        {
          assert(object_item == ObjectItem);
          auto maybe_string = unwrap(object_item / Key, JSONString);
          if (!maybe_string.success)
          {
            continue;
          }

          if (maybe_string.node->location() == key)
          {
            return object_item / Val;
          }
        }

        std::ostringstream error;
        error << "No child with key `" << key.view() << "` exists in object";
        return err(object, error.str());
      }

      static Node set_index(
        Node set, Location key, std::map<Location, Node>& lookup)
      {
        Node result;
        for (Node element : *set)
        {
          Location e_key(strip_quotes(to_key(element)));
          lookup[e_key] = element;
          if (e_key == key)
          {
            result = element;
          }
        }

        if (result == nullptr)
        {
          std::ostringstream error;
          error << "No child with key `" << key.view() << "` exists in set";
          return err(set, error.str());
        }

        return result;
      }

      static Node array_index(Node array, Location key, size_t& index)
      {
        std::string_view index_view = key.view();
        if (index_view == "-")
        {
          index = array->size();
          return err(
            JSONString ^ key,
            "End-of-array selector `-` cannot appear inside a pointer, only "
            "at the end");
        }

        if (index_view.front() == '-')
        {
          return err(
            JSONString ^ key, "unable to parse array index (prepended by `-`)");
        }

        if (index_view.front() == '0' && index_view.size() > 1)
        {
          return err(JSONString ^ key, "Leading zeros");
        }

        index = 0;
        for (auto it = index_view.begin(); it < index_view.end(); ++it)
        {
          char c = *it;
          if (!std::isdigit(c))
          {
            return err(
              JSONString ^ key, "unable to parse array index (not a digit)");
          }

          index =
            index * 10 + static_cast<size_t>(c) - static_cast<size_t>('0');
        }

        if (index >= array->size())
        {
          return err(
            Int ^ key, "index is greater than number of items in array");
        }

        return array->at(index);
      }

      Pointer m_pointer;
      Action m_action;
      Node m_value;
    };

    Node select(const Node& object, const Location& path)
    {
      return pointer::Operation(path, pointer::Action::Read).run(object);
    }

    std::optional<Location> select_string(
      const Node& document, const Location& path)
    {
      Node node = select(document, path);
      if (node == Error)
      {
        return std::nullopt;
      }

      auto maybe_string = unwrap(node, JSONString);
      if (!maybe_string.success)
      {
        return std::nullopt;
      }

      return maybe_string.node->location();
    }
  }

  namespace patch
  {
    const Location OpKey{"/op"};
    const Location PathKey{"/path"};
    const Location ValueKey{"/value"};
    const Location FromKey{"/from"};
    const Location MissingOp{"missing `op`"};
    const Location InvalidOp{"invalid `op` value"};
    const Location MissingPath{"missing `path`"};
    const Location MissingValue{"missing `value`"};
    const Location MissingFrom{"missing `from`"};

    enum class Type
    {
      Error,
      Test,
      Add,
      Remove,
      Replace,
      Copy,
      Move
    };

    const std::map<Location, Type> TypeLookup{
      {{"test"}, Type::Test},
      {{"add"}, Type::Add},
      {{"remove"}, Type::Remove},
      {{"replace"}, Type::Replace},
      {{"copy"}, Type::Copy},
      {{"move"}, Type::Move}};

    class Op
    {
    public:
      static Op from_node(Node node)
      {
        auto maybe_type = pointer::select_string(node, OpKey);
        if (!maybe_type.has_value())
        {
          return Op(node, Type::Error, MissingOp);
        }

        auto it = TypeLookup.find(maybe_type.value());
        if (it == TypeLookup.end())
        {
          return Op(node, Type::Error, InvalidOp);
        }

        Type type = it->second;

        Node path_node = pointer::select(node, PathKey);
        if (path_node == Error)
        {
          return Op(node, Type::Error, MissingPath);
        }

        auto maybe_string = unwrap(path_node, JSONString);

        Location path;
        if (maybe_string.success)
        {
          path = maybe_string.node->location();
        }
        else
        {
          auto maybe_array = unwrap(path_node, Array);
          if (maybe_array.success)
          {
            std::ostringstream os;
            for (Node term : *maybe_array.node)
            {
              os << '/' << strip_quotes(to_key(term));
            }

            path = Location(os.str());
          }
          else
          {
            return Op(node, Type::Error, MissingPath);
          }
        }

        switch (type)
        {
          case Type::Remove:
            return Op(node, Type::Remove, path);

          case Type::Add:
          case Type::Replace:
          case Type::Test: {
            Node value = pointer::select(node, ValueKey);
            if (value == Error)
            {
              return Op(node, Type::Error, MissingValue);
            }

            return Op(node, type, path, value);
          }

          case Type::Copy:
          case Type::Move: {
            auto maybe_from = pointer::select_string(node, FromKey);
            if (!maybe_from.has_value())
            {
              return Op(node, Type::Error, MissingFrom);
            }

            return Op(node, type, path, maybe_from.value());
          }

          default:
            return {node, Type::Error, {"Unknown error"}};
        }
      }

      bool operator<(const Op& other) const
      {
        return m_type < other.m_type;
      }

      Node apply(const Node& document) const
      {
        logging::Trace() << "Applying patch " << DebugKey{m_node};
        switch (m_type)
        {
          case Type::Test:
            return test(document);

          case Type::Add:
            return add(document);

          case Type::Remove:
            return remove(document);

          case Type::Replace:
            return replace(document);

          case Type::Move:
            return move(document);

          case Type::Copy:
            return copy(document);

          default:
            return err(m_node, "Unsupported operation");
        }
      }

      Type type() const
      {
        return m_type;
      }

      Node node() const
      {
        return m_node;
      }

      Location path() const
      {
        return m_path;
      }

    private:
      Op(Node node, Type type, Location path) :
        m_node(node), m_type(type), m_path(path)
      {}

      Op(Node node, Type type, Location path, Node value) :
        m_node(node), m_type(type), m_path(path), m_value(value)
      {}

      Op(Node node, Type type, Location path, Location from) :
        m_node(node), m_type(type), m_path(path), m_from(from)
      {}

      Node test(Node document) const
      {
        Node result =
          pointer::Operation(m_path, pointer::Action::Compare, m_value)
            .run(document);
        return result == Error ? result : document;
      }

      Node add(Node document) const
      {
        if (m_path.len == 0)
        {
          return m_value->clone();
        }

        Node result =
          pointer::Operation(m_path, pointer::Action::Insert, m_value)
            .run(document);
        return result == Error ? result : document;
      }

      Node remove(Node document) const
      {
        Node result =
          pointer::Operation(m_path, pointer::Action::Remove).run(document);
        return result == Error ? result : document;
      }

      Node replace(Node document) const
      {
        if (m_path.len == 0)
        {
          return m_value->clone();
        }

        Node result =
          pointer::Operation(m_path, pointer::Action::Replace, m_value)
            .run(document);
        return result == Error ? result : document;
      }

      Node move(Node document) const
      {
        if (m_path == m_from)
        {
          return document;
        }

        Node existing =
          pointer::Operation(m_from, pointer::Action::Remove).run(document);
        if (existing == Error)
        {
          return existing;
        }

        Node result =
          pointer::Operation(m_path, pointer::Action::Insert, existing)
            .run(document);
        return result == Error ? result : document;
      }

      Node copy(Node document) const
      {
        if (m_path == m_from)
        {
          return document;
        }

        Node existing =
          pointer::Operation(m_from, pointer::Action::Read).run(document);
        if (existing == Error)
        {
          return existing;
        }

        Node result =
          pointer::Operation(m_path, pointer::Action::Insert, existing)
            .run(document);
        return result == Error ? result : document;
      }

      Node m_node;
      Type m_type;
      Location m_path;
      Node m_value;
      Location m_from;
    };
  }

  namespace ptree
  {
    struct PNodeDef;

    typedef std::shared_ptr<PNodeDef> PNode;
    struct PNodeDef
    {
      bool terminal;
      std::map<Location, PNode> children;
    };

    void insert(PNode root, const std::vector<Location>& path)
    {
      PNode current = root;
      for (const Location& loc : path)
      {
        logging::Trace() << "path: " << loc.view();
        if (!current->children.contains(loc))
        {
          current->children[loc] = std::make_shared<PNodeDef>();
        }

        current = current->children[loc];
      }

      current->terminal = true;
    }

    void insert(PNode root, const Node& array)
    {
      assert(array == Array);
      std::vector<Location> path;
      std::transform(
        array->begin(),
        array->end(),
        std::back_inserter(path),
        [](const Node& n) {
          auto maybe_key = unwrap(n, {Int, JSONString});
          if (maybe_key.success)
          {
            return maybe_key.node->location();
          }

          return Location(to_key(n));
        });
      insert(root, path);
    }

    void insert(PNode root, const Location& path)
    {
      std::string_view path_view = path.view();
      std::vector<Location> parts;
      size_t pos = 0;
      size_t end = path_view.find('/');
      while (end != std::string::npos)
      {
        Location part = path;
        part.pos += pos;
        part.len = end - pos;
        parts.push_back(part);
        pos = end + 1;
        end = path_view.find('/', pos);
      }

      Location last = path;
      last.pos += pos;
      last.len = path.len - pos;
      parts.push_back(last);
      insert(root, parts);
    }
  }

  typedef std::tuple<ptree::PNode, Node, Node> filter_state;

  Node filter(Nodes args)
  {
    Node object =
      unwrap_arg(args, UnwrapOpt(0).type(Object).func("json.filter"));
    if (object == Error)
    {
      return object;
    }

    Node paths =
      unwrap_arg(args, UnwrapOpt(1).types({Set, Array}).func("json.filter"));
    if (paths == Error)
    {
      return paths;
    }

    ptree::PNode root = std::make_shared<ptree::PNodeDef>();
    for (Node path : *paths)
    {
      auto maybe_string = unwrap(path, JSONString);
      if (maybe_string.success)
      {
        insert(root, maybe_string.node->location());
        continue;
      }

      auto maybe_array = unwrap(path, Array);
      if (maybe_array.success)
      {
        insert(root, maybe_array.node);
        continue;
      }

      return err(
        path,
        "json.filter: operand 2 must be one of {set, array} containing string "
        "paths or array of path segments but got " +
          type_name(path, false),
        EvalTypeError);
    }

    Node result = NodeDef::create(Object);
    std::vector<filter_state> frontier;
    frontier.emplace_back(root, object, result);

    while (!frontier.empty())
    {
      auto [pnode, src, dst] = frontier.back();
      frontier.pop_back();

      if (src == Object)
      {
        for (Node src_item : *src)
        {
          auto maybe_key = unwrap(src_item / Key, JSONString);
          if (!maybe_key.success)
          {
            continue;
          }

          Location key = maybe_key.node->location();
          if (!pnode->children.contains(key))
          {
            continue;
          }

          auto pchild = pnode->children[key];
          if (pchild->terminal)
          {
            dst << src_item->clone();
            continue;
          }

          auto maybe_indexable = unwrap(src_item / Val, {Object, Array, Set});
          if (!maybe_indexable.success)
          {
            dst << src_item->clone();
            continue;
          }

          Node src_term = maybe_indexable.node;
          Node dst_term = NodeDef::create(src_term->type());
          dst
            << (ObjectItem << (Term << (Scalar << (JSONString ^ key)))
                           << (Term << dst_term));

          frontier.emplace_back(pchild, src_term, dst_term);
        }
        continue;
      }

      if (src == Array)
      {
        for (size_t i = 0; i < src->size(); ++i)
        {
          Location key(std::to_string(i));
          if (!pnode->children.contains(key))
          {
            continue;
          }

          Node src_term = src->at(i);

          auto pchild = pnode->children[key];
          if (pchild->terminal)
          {
            dst << src_term->clone();
            continue;
          }

          auto maybe_indexable = unwrap(src_term, {Object, Array, Set});
          if (!maybe_indexable.success)
          {
            dst << src_term->clone();
            continue;
          }

          src_term = maybe_indexable.node;
          Node dst_term = NodeDef::create(src_term->type());
          dst << (Term << dst_term);

          frontier.emplace_back(pchild, src_term, dst_term);
        }
      }

      if (src == Set)
      {
        for (Node src_term : *src)
        {
          Location key(to_key(src_term));
          if (!pnode->children.contains(key))
          {
            continue;
          }

          auto pchild = pnode->children[key];
          if (pchild->terminal)
          {
            dst << src_term->clone();
            continue;
          }

          auto maybe_indexable = unwrap(src_term, {Object, Array, Set});
          if (!maybe_indexable.success)
          {
            dst << src_term->clone();
            continue;
          }

          src_term = maybe_indexable.node;
          Node dst_term = NodeDef::create(src_term->type());
          dst << (Term << dst_term);

          frontier.emplace_back(pchild, src_term, dst_term);
        }
      }
    }

    return result;
  }

  BuiltIn filter_factory()
  {
    const Node filter_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "object")
                      << (bi::Description ^ "object to filter")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg
              << (bi::Name ^ "paths") << (bi::Description ^ "JSON string paths")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type
                          << (bi::DynamicArray
                              << (bi::Type
                                  << (bi::TypeSeq
                                      << (bi::Type << bi::String)
                                      << (bi::Type
                                          << (bi::DynamicArray
                                              << (bi::Type << bi::Any)))))))
                      << (bi::Type
                          << (bi::Set
                              << (bi::Type
                                  << (bi::TypeSeq
                                      << (bi::Type << bi::String)
                                      << (bi::Type
                                          << (bi::DynamicArray
                                              << (bi::Type << bi::Any)))))))))))
      << (bi::Result << (bi::Name ^ "filtered")
                     << (bi::Description ^
                         "remaining data from `object` with only keys "
                         "specified in `paths`")
                     << (bi::Type << bi::Any));
    return BuiltInDef::create({"json.filter"}, filter_decl, filter);
  }

  Node remove_(Nodes args)
  {
    Node object =
      unwrap_arg(args, UnwrapOpt(0).type(Object).func("json.remove"));
    if (object == Error)
    {
      return object;
    }

    Node paths =
      unwrap_arg(args, UnwrapOpt(1).types({Set, Array}).func("json.remove"));
    if (paths == Error)
    {
      return paths;
    }

    ptree::PNode root = std::make_shared<ptree::PNodeDef>();
    for (Node path : *paths)
    {
      auto maybe_string = unwrap(path, JSONString);
      if (maybe_string.success)
      {
        insert(root, maybe_string.node->location());
        continue;
      }

      auto maybe_array = unwrap(path, Array);
      if (maybe_array.success)
      {
        insert(root, maybe_array.node);
        continue;
      }

      return err(
        path,
        "json.remove: operand 2 must be one of {set, array} containing string "
        "paths or array of path segments but got " +
          type_name(path, false),
        EvalTypeError);
    }

    Node result = NodeDef::create(Object);
    std::vector<filter_state> frontier;
    frontier.emplace_back(root, object, result);

    while (!frontier.empty())
    {
      auto [pnode, src, dst] = frontier.back();
      frontier.pop_back();

      if (src == Object)
      {
        for (Node src_item : *src)
        {
          auto maybe_key = unwrap(src_item / Key, JSONString);
          if (!maybe_key.success)
          {
            continue;
          }

          Location key = maybe_key.node->location();
          if (!pnode->children.contains(key))
          {
            dst << src_item->clone();
            continue;
          }

          auto pchild = pnode->children[key];
          if (pchild->terminal)
          {
            continue;
          }

          auto maybe_indexable = unwrap(src_item / Val, {Object, Array, Set});
          if (!maybe_indexable.success)
          {
            continue;
          }

          Node src_term = maybe_indexable.node;
          Node dst_term = NodeDef::create(src_term->type());
          dst
            << (ObjectItem << (Term << (Scalar << (JSONString ^ key)))
                           << (Term << dst_term));

          frontier.emplace_back(pchild, src_term, dst_term);
        }
        continue;
      }

      if (src == Array)
      {
        for (size_t i = 0; i < src->size(); ++i)
        {
          Location key(std::to_string(i));
          Node src_term = src->at(i);

          if (!pnode->children.contains(key))
          {
            dst << src_term->clone();
            continue;
          }

          auto pchild = pnode->children[key];
          if (pchild->terminal)
          {
            continue;
          }

          auto maybe_indexable = unwrap(src_term, {Object, Array, Set});
          if (!maybe_indexable.success)
          {
            continue;
          }

          src_term = maybe_indexable.node;
          Node dst_term = NodeDef::create(src_term->type());
          dst << (Term << dst_term);

          frontier.emplace_back(pchild, src_term, dst_term);
        }
      }

      if (src == Set)
      {
        for (Node src_term : *src)
        {
          Location key(to_key(src_term));
          if (!pnode->children.contains(key))
          {
            dst << src_term->clone();
            continue;
          }

          auto pchild = pnode->children[key];
          if (pchild->terminal)
          {
            continue;
          }

          auto maybe_indexable = unwrap(src_term, {Object, Array, Set});
          if (!maybe_indexable.success)
          {
            continue;
          }

          src_term = maybe_indexable.node;
          Node dst_term = NodeDef::create(src_term->type());
          dst << (Term << dst_term);

          frontier.emplace_back(pchild, src_term, dst_term);
        }
      }
    }

    return result;
  }

  BuiltIn remove_factory()
  {
    const Node remove_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "object")
                      << (bi::Description ^ "object to remove paths from")
                      << (bi::Type
                          << (bi::DynamicObject << (bi::Type << bi::Any)
                                                << (bi::Type << bi::Any))))
          << (bi::Arg
              << (bi::Name ^ "paths") << (bi::Description ^ "JSON string paths")
              << (bi::Type
                  << (bi::TypeSeq
                      << (bi::Type
                          << (bi::DynamicArray
                              << (bi::Type
                                  << (bi::TypeSeq
                                      << (bi::Type << bi::String)
                                      << (bi::Type
                                          << (bi::DynamicArray
                                              << (bi::Type << bi::Any)))))))
                      << (bi::Type
                          << (bi::Set
                              << (bi::Type
                                  << (bi::TypeSeq
                                      << (bi::Type << bi::String)
                                      << (bi::Type
                                          << (bi::DynamicArray
                                              << (bi::Type << bi::Any)))))))))))
      << (bi::Result << (bi::Name ^ "filtered")
                     << (bi::Description ^
                         "result of removing all keys specified in `paths`")
                     << (bi::Type << bi::Any));
    return BuiltInDef::create({"json.remove"}, remove_decl, remove_);
  }

  BuiltIn match_schema_factory()
  {
    const Node match_schema_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "document")
                      << (bi::Description ^ "document to verify by schema")
                      << (bi::Type
                          << (bi::TypeSeq << (bi::Type << bi::String)
                                          << (bi::Type
                                              << (bi::DynamicObject
                                                  << (bi::Type << bi::Any)
                                                  << (bi::Type << bi::Any))))))
          << (bi::Arg << (bi::Name ^ "schema")
                      << (bi::Description ^ "schema to verify document by")
                      << (bi::Type
                          << (bi::TypeSeq << (bi::Type << bi::String)
                                          << (bi::Type
                                              << (bi::DynamicObject
                                                  << (bi::Type << bi::Any)
                                                  << (bi::Type << bi::Any)))))))
      << (bi::Result
          << (bi::Name ^ "output")
          << (bi::Description ^
              "`output` is of the form `[match, errors]`. If the document is "
              "valid given the schema, then `match` is `true`, and `errors` is "
              "an empty array. Otherwise, `match` is `false` and `errors` is "
              "an "
              "array of objects describing the error(s).")
          << (bi::Type
              << (bi::StaticArray
                  << (bi::Type << bi::Boolean)
                  << (bi::Type
                      << (bi::DynamicArray
                          << (bi::Type
                              << (bi::StaticObject
                                  << (bi::ObjectItem
                                      << (bi::Name ^ "desc")
                                      << (bi::Type << bi::String))
                                  << (bi::ObjectItem
                                      << (bi::Name ^ "error")
                                      << (bi::Type << bi::String))
                                  << (bi::ObjectItem
                                      << (bi::Name ^ "field")
                                      << (bi::Type << bi::String))
                                  << (bi::ObjectItem
                                      << (bi::Name ^ "type")
                                      << (bi::Type << bi::String)))))))));
    return BuiltInDef::placeholder(
      {"json.match_schema"}, match_schema_decl, "JSON schema is not supported");
  }

  Node patch_(Nodes args)
  {
    Node object = args[0];

    Node patch = UnwrapOpt(1).type(Array).func("json.patch").unwrap(args);
    if (patch == Error)
    {
      return patch;
    }

    if (patch->empty())
    {
      return object->clone();
    }

    std::vector<patch::Op> ops;

    for (const auto& node : *patch)
    {
      auto op = patch::Op::from_node(node);
      if (op.type() == patch::Type::Error)
      {
        return err(op.node(), std::string(op.path().view()));
      }

      if (op.type() == patch::Type::Test)
      {
        Node result = op.apply(object);
        if (result == Error)
        {
          return result;
        }

        continue;
      }

      ops.push_back(op);
    }

    Node patched = object->clone();

    for (const patch::Op& op : ops)
    {
      patched = op.apply(patched);
      if (patched == Error)
      {
        return patched;
      }

      logging::Trace() << "After: " << DebugKey{patched};
    }

    return patched;
  }

  BuiltIn verify_schema_factory()
  {
    const Node verify_schema_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "schema")
                               << (bi::Description ^ "the schema to verify")
                               << (bi::Type
                                   << (bi::TypeSeq
                                       << (bi::Type << bi::String)
                                       << (bi::Type
                                           << (bi::DynamicObject
                                               << (bi::Type << bi::Any)
                                               << (bi::Type << bi::Any)))))))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^
                       "`output` is of the form `[valid, error]`. If the "
                       "schema is valid, "
                       "then `valid` is `true`, and `error` is `null`. "
                       "Otherwise, `valid` "
                       "is false and `error` is a string describing the error.")
                   << (bi::Type
                       << (bi::StaticArray
                           << (bi::Type << bi::Boolean)
                           << (bi::Type
                               << (bi::TypeSeq << (bi::Type << bi::Null)
                                               << (bi::Type << bi::String))))));
    return BuiltInDef::placeholder(
      {"json.verify_schema"},
      verify_schema_decl,
      "JSON Schema is not supported");
  }

  BuiltIn patch_factory()
  {
    const Node patch_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "object")
                                 << (bi::Description ^ "the object to patch")
                                 << (bi::Type << bi::Any))
                     << (bi::Arg
                         << (bi::Name ^ "patches")
                         << (bi::Description ^ "the JSON patches to apply")
                         << (bi::Type
                             << (bi::DynamicArray
                                 << (bi::Type
                                     << (bi::HybridObject
                                         << (bi::DynamicObject
                                             << (bi::Type << bi::Any)
                                             << (bi::Type << bi::Any))
                                         << (bi::StaticObject
                                             << (bi::ObjectItem
                                                 << (bi::Name ^ "op")
                                                 << (bi::Type << bi::String))
                                             << (bi::ObjectItem
                                                 << (bi::Name ^ "path")
                                                 << (bi::Type
                                                     << bi::String)))))))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "result obtained after consecutively applying "
                         "all patch operations in `patches`")
                     << (bi::Type << bi::Any));
    return BuiltInDef::create({"json.patch"}, patch_decl, patch_);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn json(const Location& name)
    {
      assert(name.view().substr(0, 5) == "json.");
      std::string_view view = name.view().substr(5); // skip "json."
      if (view == "marshal")
      {
        return marshal_factory();
      }

      if (view == "marshal_with_options")
      {
        return marshal_with_options_factory();
      }

      if (view == "unmarshal")
      {
        return unmarshal_factory();
      }

      if (view == "is_valid")
      {
        return is_valid_factory();
      }

      if (view == "filter")
      {
        return filter_factory();
      }

      if (view == "remove")
      {
        return remove_factory();
      }

      if (view == "match_schema")
      {
        return match_schema_factory();
      }

      if (view == "verify_schema")
      {
        return verify_schema_factory();
      }

      if (view == "patch")
      {
        return patch_factory();
      }

      return nullptr;
    }
  }
}