#include "internal.hh"
#include "rego.hh"
#include "trieste/json.h"
#include "trieste/rewrite.h"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node err(Node r, const std::string& msg)
  {
    return Error << (ErrorMsg ^ msg) << (ErrorAst << r);
  }

  Location strip_quotes_node(const Node& n)
  {
    Location loc = n->location();
    if (
      loc.view().starts_with('"') && loc.view().starts_with('"') &&
      loc.len >= 2)
    {
      loc.pos += 1;
      loc.len -= 2;
    }

    return loc;
  }

  Node to_irstring(const Node& n)
  {
    return IRString ^ json::unescape(strip_quotes_node(n).view());
  }

  Node to_localindex(const Node& n)
  {
    return LocalIndex ^ n;
  }

  Node object_lookdown(
    Node object,
    const std::string& key,
    const Token& token,
    const std::function<Node(const Node&)>& transform = [](const Node& n) {
      return n;
    })
  {
    if (object->type() != json::Object)
    {
      std::ostringstream buf;
      buf << "Expected " << token << ", found " << object->type();
      return err(object, buf.str());
    }

    auto nodes = object->lookdown({key});
    if (nodes.empty())
    {
      // OPA bundles sometimes capitalise keys
      std::string key_capital = key;
      key_capital[0] = std::toupper(key_capital[0]);
      nodes = object->lookdown({key_capital});
      if (nodes.empty())
      {
        return err(object, "Object lookdown failed for key: " + key);
      }
    }

    Node result = nodes.front()->back();
    if (result != token)
    {
      std::ostringstream buf;
      buf << "Expected " << token << ", found " << result->type();
      return err(result, buf.str());
    }

    return transform(result);
  }

  Node to_builtinname(Node n)
  {
    return bi::Name ^ strip_quotes_node(n);
  }

  Node to_builtindescription(Node n)
  {
    return bi::Description ^ strip_quotes_node(n);
  }

  Node to_builtintype(Node n);

  Node to_builtinany(Node n)
  {
    Node of = object_lookdown(n, "of", json::Array);

    if (of == Error)
    {
      return bi::Any ^ "any";
    }

    Node typeseq = NodeDef::create(bi::TypeSeq);
    for (Node type : *of)
    {
      typeseq << to_builtintype(type);
    }

    return typeseq;
  }

  Node to_builtinarray(Node n)
  {
    Node staticarray_json = object_lookdown(n, "static", json::Array);
    Node dynamicarray_json = object_lookdown(n, "dynamic", json::Object);

    if (staticarray_json == json::Array)
    {
      Node staticarray = NodeDef::create(bi::StaticArray);
      for (Node type : *staticarray_json)
      {
        staticarray << to_builtintype(type);
      }

      return staticarray;
    }

    if (dynamicarray_json == json::Object)
    {
      return bi::DynamicArray << to_builtintype(dynamicarray_json);
    }

    return err(n, "Invalid array type decl");
  }

  Node to_builtinobjectitem(Node n)
  {
    Node key = object_lookdown(n, "key", json::String, to_builtinname);
    Node value = object_lookdown(n, "value", json::Object);

    if (key == Error)
    {
      return key;
    }

    if (value == Error)
    {
      return value;
    }

    return bi::ObjectItem << key << to_builtintype(value);
  }

  Node to_builtinobject(Node n)
  {
    Node staticobject_json = object_lookdown(n, "static", json::Array);
    Node dynamicobject_json = object_lookdown(n, "dynamic", json::Object);

    Node staticobject;
    if (staticobject_json == json::Array)
    {
      staticobject = NodeDef::create(bi::StaticObject);
      for (Node type : *staticobject_json)
      {
        staticobject << to_builtinobjectitem(type);
      }
    }

    Node dynamicobject;
    if (dynamicobject_json == json::Object)
    {
      Node key = object_lookdown(dynamicobject_json, "key", json::Object);
      Node val = object_lookdown(dynamicobject_json, "value", json::Object);
      if (key == Error)
      {
        return key;
      }

      if (val == Error)
      {
        return val;
      }

      dynamicobject = bi::DynamicObject << to_builtintype(key)
                                        << to_builtintype(val);
    }

    if (staticobject != nullptr)
    {
      if (dynamicobject != nullptr)
      {
        return bi::HybridObject << dynamicobject << staticobject;
      }

      return staticobject;
    }

    if (dynamicobject != nullptr)
    {
      return dynamicobject;
    }

    return err(n, "Invalid object type decl");
  }

  Node to_builtinset(Node n)
  {
    Node of = object_lookdown(n, "of", json::Object);

    if (of == Error)
    {
      return of;
    }

    return bi::Set << to_builtintype(of);
  }

  Node to_builtintype(Node n)
  {
    Node type = object_lookdown(n, "type", json::String);
    Location type_loc = strip_quotes_node(type);
    if (type_loc.view() == "any")
    {
      return bi::Type << to_builtinany(n);
    }

    if (type_loc.view() == "number")
    {
      return bi::Type << (bi::Number ^ type_loc);
    }

    if (type_loc.view() == "boolean")
    {
      return bi::Type << (bi::Boolean ^ type_loc);
    }

    if (type_loc.view() == "string")
    {
      return bi::Type << (bi::String ^ type_loc);
    }

    if (type_loc.view() == "null")
    {
      return bi::Type << (bi::Null ^ type_loc);
    }

    if (type_loc.view() == "array")
    {
      return bi::Type << to_builtinarray(n);
    }

    if (type_loc.view() == "object")
    {
      return bi::Type << to_builtinobject(n);
    }

    if (type_loc.view() == "set")
    {
      return bi::Type << to_builtinset(n);
    }

    return err(type, "Invalid builtin type");
  }

  Node to_builtinarg(Node arg)
  {
    assert(arg == json::Object);
    Node name = object_lookdown(arg, "name", json::String, to_builtinname);
    Node description =
      object_lookdown(arg, "description", json::String, to_builtindescription);
    Node type = to_builtintype(arg);

    if (name == Error)
    {
      name = bi::Name ^ "";
    }

    if (description == Error)
    {
      description = bi::Description ^ "";
    }

    return bi::Arg << name << description << type;
  }

  Node to_builtinresult(Node arg)
  {
    assert(arg == json::Object);
    Node name = object_lookdown(arg, "name", json::String, to_builtinname);
    Node description =
      object_lookdown(arg, "description", json::String, to_builtindescription);
    Node type = to_builtintype(arg);

    if (name == Error)
    {
      name = bi::Name ^ "";
    }

    if (description == Error)
    {
      description = bi::Description ^ "";
    }

    return bi::Result << name << description << type;
  }

  Node to_builtinargseq(Node args_json)
  {
    Node args = NodeDef::create(bi::ArgSeq);
    for (Node arg : *args_json)
    {
      args << to_builtinarg(arg);
    }

    return args;
  }

  Node to_builtindecl(Node obj)
  {
    Node args = object_lookdown(obj, "args", json::Array, to_builtinargseq);

    if (args == Error)
    {
      args = NodeDef::create(bi::VarArgs);
    }

    Node result =
      object_lookdown(obj, "result", json::Object, to_builtinresult);
    if (result == Error)
    {
      result = NodeDef::create(builtins::Void);
    }

    return bi::Decl << args << result;
  }

  Node statement(
    std::shared_ptr<std::vector<Source>> sources, Node stmt, Token token)
  {
    Node file = object_lookdown(stmt, "file", json::Number);
    Node row = object_lookdown(stmt, "row", json::Number);
    Node col = object_lookdown(stmt, "col", json::Number);

    if (file == Error || row == Error || col == Error)
    {
      return token;
    }

    size_t file_index = to_size(file);
    if (file_index >= sources->size())
    {
      return token;
    }

    Source source = sources->at(file_index);
    auto [line_start, line_length] = source->linepos(to_size(row));
    size_t pos = line_start + to_size(col);
    size_t len = line_length - pos;
    Location loc(sources->at(file_index), pos, len);
    return token ^ loc;
  }

  PassDef json_to_bundle()
  {
    auto sources = std::make_shared<std::vector<Source>>();
    PassDef pass = {
      "json_to_bundle",
      wf_bundle,
      dir::bottomup | dir::once,
      {
        In(json::Member, json::Array) *
            (T(json::Object)
             << ((T(json::Member)
                  << (T(json::Key, "type") * T(json::String, "\"local\""))) *
                 (T(json::Member)
                  << (T(json::Key, "value") * T(json::Number)[LocalIndex])))) >>
          [](Match& _) { return Operand << (LocalIndex ^ _(LocalIndex)); },

        In(json::Member, json::Array) *
            (T(json::Object)
             << ((T(json::Member)
                  << (T(json::Key, "type") * T(json::String, "\"bool\""))) *
                 (T(json::Member)
                  << (T(json::Key, "value") *
                      T(json::True, json::False)[Boolean])))) >>
          [](Match& _) { return Operand << (Boolean ^ _(Boolean)); },

        In(json::Member, json::Array) *
            (T(json::Object)
             << ((T(json::Member)
                  << (T(json::Key, "type") *
                      T(json::String, "\"string_index\""))) *
                 (T(json::Member)
                  << (T(json::Key, "value") *
                      T(json::Number)[StringIndex])))) >>
          [](Match& _) { return Operand << (StringIndex ^ _(StringIndex)); },

        In(json::Array) *
            (T(json::Object)[ArrayAppendStmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ArrayAppendStmt\"")))) >>
          [sources](Match& _) {
            Node stmt =
              object_lookdown(_(ArrayAppendStmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node array =
              object_lookdown(stmt, "array", json::Number, to_localindex);
            Node value = object_lookdown(stmt, "value", Operand);
            return statement(sources, stmt, ArrayAppendStmt) << array << value;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"AssignIntStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node value =
              object_lookdown(stmt, "value", json::Number, [](const Node& n) {
                return Int64 ^ n;
              });
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, AssignIntStmt) << value << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"AssignVarOnceStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source = object_lookdown(stmt, "source", Operand);
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, AssignVarOnceStmt)
              << source << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"AssignVarStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source = object_lookdown(stmt, "source", Operand);
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, AssignVarStmt) << source << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"BlockStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node blockseq = object_lookdown(stmt, "blocks", BlockSeq);

            return statement(sources, stmt, BlockStmt) << blockseq;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"BreakStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node index =
              object_lookdown(stmt, "index", json::Number, [](const Node& n) {
                return UInt32 ^ n;
              });
            return statement(sources, stmt, BreakStmt) << index;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"CallDynamicStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node func =
              object_lookdown(stmt, "path", json::Array, [](const Node& n) {
                return OperandSeq << *n;
              });
            Node args =
              object_lookdown(stmt, "args", json::Array, [](const Node& n) {
                return OperandSeq << *n;
              });
            Node result =
              object_lookdown(stmt, "result", json::Number, to_localindex);

            return statement(sources, stmt, CallDynamicStmt)
              << func << args << result;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"CallStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node func =
              object_lookdown(stmt, "func", json::String, to_irstring);
            Node args =
              object_lookdown(stmt, "args", json::Array, [](const Node& n) {
                return OperandSeq << *n;
              });
            Node result =
              object_lookdown(stmt, "result", json::Number, to_localindex);
            return statement(sources, stmt, CallStmt) << func << args << result;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") * T(json::String, "\"DotStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source = object_lookdown(stmt, "source", Operand);
            Node key = object_lookdown(stmt, "key", Operand);
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, DotStmt) << source << key << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"EqualStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node a = object_lookdown(stmt, "a", Operand);
            Node b = object_lookdown(stmt, "b", Operand);
            return statement(sources, stmt, EqualStmt) << a << b;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"IsArrayStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source = object_lookdown(stmt, "source", Operand);
            return statement(sources, stmt, IsArrayStmt) << source;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"IsDefinedStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source =
              object_lookdown(stmt, "source", json::Number, to_localindex);
            return statement(sources, stmt, IsDefinedStmt) << source;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"IsObjectStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source = object_lookdown(stmt, "source", Operand);
            return statement(sources, stmt, IsObjectStmt) << source;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"IsSetStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source = object_lookdown(stmt, "source", Operand);
            return statement(sources, stmt, IsSetStmt) << source;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"IsUndefinedStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source =
              object_lookdown(stmt, "source", json::Number, to_localindex);
            return statement(sources, stmt, IsUndefinedStmt) << source;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") * T(json::String, "\"LenStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source = object_lookdown(stmt, "source", Operand);
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, LenStmt) << source << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"MakeArrayStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node capacity = object_lookdown(
              stmt, "capacity", json::Number, [](const Node& n) {
                return Int32 ^ n;
              });
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, MakeArrayStmt)
              << capacity << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"MakeNullStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, MakeNullStmt) << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"MakeNumberIntStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node value =
              object_lookdown(stmt, "value", json::Number, [](const Node& n) {
                return Int64 ^ n;
              });
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, MakeNumberIntStmt)
              << value << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"MakeNumberRefStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node index =
              object_lookdown(stmt, "index", json::Number, [](const Node& n) {
                return Int32 ^ n;
              });
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, MakeNumberRefStmt)
              << index << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"MakeObjectStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, MakeObjectStmt) << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"MakeSetStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, MakeSetStmt) << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"NotEqualStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node a = object_lookdown(stmt, "a", Operand);
            Node b = object_lookdown(stmt, "b", Operand);
            return statement(sources, stmt, NotEqualStmt) << a << b;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") * T(json::String, "\"NotStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node block = object_lookdown(stmt, "block", Block);
            return statement(sources, stmt, NotStmt) << block;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ObjectInsertOnceStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node key = object_lookdown(stmt, "key", Operand);
            Node value = object_lookdown(stmt, "value", Operand);
            Node object =
              object_lookdown(stmt, "object", json::Number, to_localindex);
            return statement(sources, stmt, ObjectInsertOnceStmt)
              << key << value << object;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ObjectInsertStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node key = object_lookdown(stmt, "key", Operand);
            Node value = object_lookdown(stmt, "value", Operand);
            Node object =
              object_lookdown(stmt, "object", json::Number, to_localindex);
            return statement(sources, stmt, ObjectInsertStmt)
              << key << value << object;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ObjectMergeStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node a = object_lookdown(stmt, "a", json::Number, to_localindex);
            Node b = object_lookdown(stmt, "b", json::Number, to_localindex);
            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, ObjectMergeStmt)
              << a << b << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ResetLocalStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node target =
              object_lookdown(stmt, "target", json::Number, to_localindex);
            return statement(sources, stmt, ResetLocalStmt) << target;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ResultSetAddStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node value =
              object_lookdown(stmt, "value", json::Number, to_localindex);
            return statement(sources, stmt, ResultSetAddStmt) << value;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ReturnLocalStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source =
              object_lookdown(stmt, "source", json::Number, to_localindex);
            return statement(sources, stmt, ReturnLocalStmt) << source;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"ScanStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node source =
              object_lookdown(stmt, "source", json::Number, to_localindex);

            Node key =
              object_lookdown(stmt, "key", json::Number, to_localindex);

            Node value =
              object_lookdown(stmt, "value", json::Number, to_localindex);

            Node block = object_lookdown(stmt, "block", Block);
            return statement(sources, stmt, ScanStmt)
              << source << key << value << block;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"SetAddStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node value = object_lookdown(stmt, "value", Operand);
            Node set =
              object_lookdown(stmt, "set", json::Number, to_localindex);
            return statement(sources, stmt, SetAddStmt) << value << set;
          },

        In(json::Array) *
            (T(json::Object)[Stmt]
             << (T(json::Member)
                 << (T(json::Key, "type") *
                     T(json::String, "\"WithStmt\"")))) >>
          [sources](Match& _) {
            Node stmt = object_lookdown(_(Stmt), "stmt", json::Object);
            if (stmt == Error)
            {
              return stmt;
            }

            Node local =
              object_lookdown(stmt, "local", json::Number, to_localindex);

            Node path =
              object_lookdown(stmt, "path", json::Array, [](const Node& n) {
                Node path = NodeDef::create(Int32Seq);
                for (Node index : *n)
                {
                  if (index != json::Number)
                  {
                    path << err(index, "Not a valid index");
                  }
                  else
                  {
                    path << (Int32 ^ index);
                  }
                }

                return path;
              });

            Node value = object_lookdown(stmt, "value", Operand);

            Node block = object_lookdown(stmt, "block", Block);
            return statement(sources, stmt, WithStmt)
              << local << path << value << block;
          },

        In(json::Array, json::Member) *
            (T(json::Object)
             << (T(json::Member)
                 << (T(json::Key, "stmts") * T(json::Array)[Block]))) >>
          [](Match& _) { return Block << *_(Block); },

        In(json::Member) *
            (T(json::Key, "blocks")[json::Key] * T(json::Array)[BlockSeq]) >>
          [](Match& _) {
            return Seq << _(json::Key) << (BlockSeq << *_(BlockSeq));
          },

        In(json::Object) *
            (T(json::Member)
             << (T(json::Key, "strings") * T(json::Array)[StringSeq])) >>
          [](Match& _) {
            Node stringseq = NodeDef::create(StringSeq);
            for (auto& child : *_(StringSeq))
            {
              stringseq << object_lookdown(
                child, "value", json::String, to_irstring);
            }

            return stringseq;
          },

        In(json::Object) *
            (T(json::Member)
             << (T(json::Key, "builtin_funcs") *
                 T(json::Array)[BuiltInFunctionSeq])) >>
          [](Match& _) {
            Node builtinfunctionseq = NodeDef::create(BuiltInFunctionSeq);
            for (Node child : *_(BuiltInFunctionSeq))
            {
              Node name =
                object_lookdown(child, "name", json::String, to_irstring);
              Node decl =
                object_lookdown(child, "decl", json::Object, to_builtindecl);
              builtinfunctionseq << (BuiltInFunction << name << decl);
            }

            return builtinfunctionseq;
          },

        In(json::Object) *
            (T(json::Member)
             << (T(json::Key, "files")[json::Key] * T(json::Array)[PathSeq])) >>
          [](Match& _) {
            std::filesystem::path json_path(
              _(PathSeq)->location().source->origin());
            std::filesystem::path bundle_dir = json_path.parent_path();

            Node pathseq = NodeDef::create(PathSeq);
            for (auto& child : *_(PathSeq))
            {
              Node path =
                object_lookdown(child, "value", json::String, to_irstring);
              auto abspath = bundle_dir / path->location().view();
              pathseq << (IRString ^ abspath.string());
            }

            return pathseq;
          },

        In(json::Object) *
            (T(json::Member)
             << (T(json::Key, "plans")[json::Key] *
                 T(json::Object)[PlanSeq])) >>
          [](Match& _) {
            Node plans_array =
              object_lookdown(_(PlanSeq), "plans", json::Array);
            if (plans_array == Error)
            {
              return plans_array;
            }

            Node planseq = NodeDef::create(PlanSeq);
            for (auto& child : *plans_array)
            {
              Node name =
                object_lookdown(child, "name", json::String, to_irstring);
              Node blockseq = object_lookdown(child, "blocks", BlockSeq);
              planseq << (Plan << name << blockseq);
            }

            return planseq;
          },

        In(json::Object) *
            (T(json::Member)
             << (T(json::Key, "funcs") * T(json::Object)[FunctionSeq])) >>
          [](Match& _) {
            Node funcs_array =
              object_lookdown(_(FunctionSeq), "funcs", json::Array);
            if (funcs_array == Error)
            {
              return funcs_array;
            }

            Node funcseq = NodeDef::create(FunctionSeq);
            for (auto& child : *funcs_array)
            {
              Node name =
                object_lookdown(child, "name", json::String, to_irstring);
              Node path =
                object_lookdown(child, "path", json::Array, [](const Node& n) {
                  Node irpath = NodeDef::create(IRPath);
                  for (const auto& str : *n)
                  {
                    irpath << to_irstring(str);
                  }
                  return irpath;
                });
              Node params = object_lookdown(
                child, "params", json::Array, [](const Node& n) {
                  Node paramseq = NodeDef::create(ParameterSeq);
                  for (const auto& param : *n)
                  {
                    paramseq << (LocalIndex ^ param);
                  }
                  return paramseq;
                });
              Node local = object_lookdown(
                child, "return", json::Number, [](const Node& n) {
                  return LocalIndex ^ n;
                });
              Node blockseq = object_lookdown(child, "blocks", BlockSeq);
              funcseq
                << (Function << name << path << params << local << blockseq);
            }

            return funcseq;
          },

        In(json::Object) *
            (T(json::Member)
             << (T(json::Key, "static") * T(json::Object)[Static])) >>
          [](Match& _) {
            Node stringseq;
            Node pathseq;
            Node builtinfunctionseq;
            for (Node child : *_(Static))
            {
              if (child == StringSeq)
              {
                stringseq = child;
              }
              else if (child == PathSeq)
              {
                pathseq = child;
              }
              else if (child == BuiltInFunctionSeq)
              {
                builtinfunctionseq = child;
              }
            }

            if (stringseq == nullptr)
            {
              return rego::err(
                _(Policy), "Missing `static/strings` member", WellFormedError);
            }

            if (pathseq == nullptr)
            {
              pathseq = NodeDef::create(PathSeq);
            }

            if (builtinfunctionseq == nullptr)
            {
              builtinfunctionseq = NodeDef::create(BuiltInFunctionSeq);
            }

            return Static << stringseq << pathseq << builtinfunctionseq;
          },

        In(Top) *
            (T(json::Object)
             << ((T(json::Member)
                  << (T(json::Key, "data") * T(json::Object)[Data])) *
                 (T(json::Member)
                  << (T(json::Key, "plan") * T(json::Object)[Policy])))) >>
          [](Match& _) {
            Node static_;
            Node planseq;
            Node functionseq;
            Node query = NodeDef::create(Undefined);
            if (_(Policy)->empty())
            {
              return err(_(Policy), "Invalid policy object");
            }

            for (Node child : *_(Policy))
            {
              if (child == Static)
              {
                static_ = child;
              }
              else if (child == PlanSeq)
              {
                planseq = child;
              }
              else if (child == FunctionSeq)
              {
                functionseq = child;
              }
              else if (
                child == json::Member &&
                (child / json::Key)->location().view() == "query")
              {
                query = to_irstring(child / json::Value);
              }
            }

            if (static_ == nullptr)
            {
              return rego::err(
                _(Policy), "Missing `static` member", WellFormedError);
            }

            if (planseq == nullptr)
            {
              return rego::err(
                _(Policy), "Missing `plans` member", WellFormedError);
            }

            if (functionseq == nullptr)
            {
              return rego::err(
                _(Policy), "Missing `funcs` member", WellFormedError);
            }

            // TODO this is a hack, need a way to incorporate this in a
            // different way
            auto result = (Top << _(Data)) >> json_to_rego(true);
            if (!result.ok)
            {
              logging::Error err;
              result.print_errors(err);
              return ErrorSeq << result.errors;
            }

            Node data = Data << result.ast->front()->front();

            return RegoBundle << data
                              << (Policy << static_ << planseq
                                         << (IRQuery << query) << functionseq)
                              << ModuleFileSeq;
          },

        // errors

        In(Top) *
            T(json::Object,
              json::Array,
              json::Number,
              json::Null,
              json::True,
              json::False,
              json::String)[json::Value] >>
          [](Match& _) {
            return rego::err(
              _(json::Value), "Invalid bundle JSON", WellFormedError);
          },
      }};

    pass.pre([sources](Node n) {
      sources->clear();

      if (n->empty())
      {
        return 0;
      }

      Node plan = object_lookdown(n->front(), "plan", json::Object);
      if (plan == Error)
      {
        return 0;
      }

      Node static_ = object_lookdown(plan, "static", json::Object);
      if (static_ == Error)
      {
        return 0;
      }

      Node files = object_lookdown(static_, "files", json::Array);
      if (files == Error)
      {
        return 0;
      }

      std::filesystem::path json_path(static_->location().source->origin());
      std::filesystem::path bundle_dir = json_path.parent_path();

      for (Node file : *files)
      {
        Node value = object_lookdown(file, "value", json::String);
        std::string name = strip_quotes(value->location().view());
        std::filesystem::path source_path = bundle_dir / name;
        if (std::filesystem::exists(source_path))
        {
          sources->push_back(SourceDef::load(source_path));
        }
        else
        {
          sources->clear();
          return 0;
        }
      }

      return 0;
    });

    return pass;
  }

  Node add_loc(
    std::shared_ptr<std::vector<std::string>> files, Node src, Node stmt)
  {
    Location loc = src->location();
    if (loc.source == nullptr || loc.source->origin().empty())
    {
      // likely statements from the query, or an entrypoint (i.e., no source
      // file)
      return stmt;
    }

    auto it = std::find(files->begin(), files->end(), loc.source->origin());
    size_t index;
    if (it == files->end())
    {
      logging::Error() << "Unexpected file: " << loc.source->origin();
      return stmt;
    }

    index = std::distance(files->begin(), it);
    return stmt << (json::Member << (json::Key ^ "file")
                                 << (json::Number ^ std::to_string(index)))
                << (json::Member
                    << (json::Key ^ "row")
                    << (json::Number ^ std::to_string(loc.linecol().first)))
                << (json::Member
                    << (json::Key ^ "col")
                    << (json::Number ^ std::to_string(loc.linecol().second)));
  }

  Node member_number(const char* key, Node node)
  {
    return json::Member << (json::Key ^ key) << (json::Number ^ node);
  }

  Node member(const char* key, const char* value)
  {
    return json::Member << (json::Key ^ key)
                        << (json::String ^ add_quotes(value));
  }

  Node member_null(const char* key)
  {
    return json::Member << (json::Key ^ key) << (json::Null ^ "null");
  }

  Node member_false(const char* key)
  {
    return json::Member << (json::Key ^ key) << (json::False ^ "false");
  }

  Node member_true(const char* key)
  {
    return json::Member << (json::Key ^ key) << (json::True ^ "true");
  }

  Node member(const char* key, Node value)
  {
    return json::Member << (json::Key ^ key) << value;
  }

  Node irstring_to_jsonstring(Node irstring)
  {
    return json::String ^ add_quotes(json::escape(irstring->location().view()));
  }

  PassDef bundle_to_json()
  {
    auto files = std::make_shared<std::vector<std::string>>();
    PassDef pass = {
      "bundle_to_json",
      json::wf,
      dir::bottomup | dir::once,
      {
        T(IRString)[IRString] >>
          [](Match& _) { return irstring_to_jsonstring(_(IRString)); },

        T(Operand) << T(LocalIndex)[LocalIndex] >>
          [](Match& _) {
            return json::Object << member("type", "local")
                                << member_number("value", _(LocalIndex));
          },

        T(Operand) << T(StringIndex)[StringIndex] >>
          [](Match& _) {
            return json::Object << member("type", "string_index")
                                << member_number("value", _(StringIndex));
          },

        T(Operand) << T(Boolean, "true") >>
          [](Match& _) {
            return json::Object << member("type", "bool")
                                << member_true("value");
          },

        T(Operand) << T(Boolean, "false") >>
          [](Match& _) {
            return json::Object << member("type", "bool")
                                << member_false("value");
          },

        In(Block) *
            (T(ArrayAppendStmt)[ArrayAppendStmt]
             << (T(LocalIndex)[Array] * T(json::Object)[Operand])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ArrayAppendStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ArrayAppendStmt),
                       json::Object << member_number("array", _(Array))
                                    << member("value", _(Operand))));
          },

        In(Block) *
            (T(AssignIntStmt)[AssignIntStmt]
             << (T(Int64)[Val] * T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "AssignIntStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(AssignIntStmt),
                       json::Object << member_number("value", _(Val))
                                    << member_number("target", _(Target))));
          },

        In(Block) *
            (T(AssignVarOnceStmt)[AssignVarOnceStmt]
             << (T(json::Object)[Operand] * T(LocalIndex)[LocalIndex])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "AssignVarOnceStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(AssignVarOnceStmt),
                       json::Object << member("source", _(Operand))
                                    << member_number("target", _(LocalIndex))));
          },

        In(Block) *
            (T(AssignVarStmt)[AssignVarStmt]
             << (T(json::Object)[Operand] * T(LocalIndex)[LocalIndex])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "AssignVarStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(AssignVarStmt),
                       json::Object << member("source", _(Operand))
                                    << member_number("target", _(LocalIndex))));
          },

        In(Block) * (T(BlockStmt)[BlockStmt] << (T(json::Array)[BlockSeq])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "BlockStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(BlockStmt),
                       json::Object << member("blocks", _(BlockSeq))));
          },

        In(Block) * (T(BreakStmt)[BreakStmt] << (T(UInt32)[UInt32])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "BreakStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(BreakStmt),
                       json::Object << member_number("index", _(UInt32))));
          },

        T(OperandSeq)[OperandSeq] >>
          [](Match& _) { return json::Array << *_(OperandSeq); },

        In(Block) *
            (T(CallDynamicStmt)[CallDynamicStmt]
             << (T(json::Array)[Func] * T(json::Array)[Args] *
                 T(LocalIndex)[LocalIndex])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "CallDynamicStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(CallDynamicStmt),
                       json::Object << member("path", _(Func))
                                    << member("args", _(Args))
                                    << member_number("result", _(LocalIndex))));
          },

        In(Block) *
            (T(CallStmt)[CallStmt]
             << (T(json::String)[Func] * T(json::Array)[Args] *
                 T(LocalIndex)[LocalIndex])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "CallStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(CallStmt),
                       json::Object << member("func", _(Func))
                                    << member("args", _(Args))
                                    << member_number("result", _(LocalIndex))));
          },

        In(Block) *
            (T(DotStmt)[DotStmt]
             << (T(json::Object)[Src] * T(json::Object)[Key] *
                 T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "DotStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(DotStmt),
                       json::Object << member("source", _(Src))
                                    << member("key", _(Key))
                                    << member_number("target", _(Target))));
          },

        In(Block) *
            (T(EqualStmt)[EqualStmt]
             << (T(json::Object)[Lhs] * T(json::Object)[Rhs])) >>
          [files](Match& _) {
            return json::Object << member("type", "EqualStmt")
                                << member(
                                     "stmt",
                                     add_loc(
                                       files,
                                       _(EqualStmt),
                                       json::Object << member("a", _(Lhs))
                                                    << member("b", _(Rhs))));
          },
        In(Block) * (T(IsArrayStmt)[IsArrayStmt] << (T(json::Object)[Src])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "IsArrayStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(IsArrayStmt),
                       json::Object << member("source", _(Src))));
          },

        In(Block) *
            (T(IsDefinedStmt)[IsDefinedStmt] << (T(LocalIndex)[LocalIndex])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "IsDefinedStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(IsDefinedStmt),
                       json::Object << member_number("source", _(LocalIndex))));
          },

        In(Block) * (T(IsObjectStmt)[IsObjectStmt] << (T(json::Object)[Src])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "IsObjectStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(IsObjectStmt),
                       json::Object << member("source", _(Src))));
          },

        In(Block) * (T(IsSetStmt)[IsSetStmt] << (T(json::Object)[Src])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "IsSetStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(IsSetStmt),
                       json::Object << member("source", _(Src))));
          },

        In(Block) *
            (T(IsUndefinedStmt)[IsUndefinedStmt]
             << (T(LocalIndex)[LocalIndex])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "IsUndefinedStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(IsUndefinedStmt),
                       json::Object << member_number("source", _(LocalIndex))));
          },

        In(Block) *
            (T(LenStmt)[LenStmt]
             << (T(json::Object)[Src] * T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "LenStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(LenStmt),
                       json::Object << member("source", _(Src))
                                    << member_number("target", _(Target))));
          },

        In(Block) *
            (T(MakeArrayStmt)[MakeArrayStmt]
             << (T(Int32)[Capacity] * T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "MakeArrayStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(MakeArrayStmt),
                       json::Object << member_number("capacity", _(Capacity))
                                    << member_number("target", _(Target))));
          },

        In(Block) * (T(MakeNullStmt)[MakeNullStmt] << T(LocalIndex)[Target]) >>
          [files](Match& _) {
            return json::Object
              << member("type", "MakeNullStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(MakeNullStmt),
                       json::Object << member_number("target", _(Target))));
          },

        In(Block) *
            (T(MakeNumberIntStmt)[MakeNumberIntStmt]
             << (T(Int64)[Val] * T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "MakeNumberIntStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(MakeNumberIntStmt),
                       json::Object << member_number("value", _(Val))
                                    << member_number("target", _(Target))));
          },

        In(Block) *
            (T(MakeNumberRefStmt)[MakeNumberRefStmt]
             << (T(Int32)[Idx] * T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "MakeNumberRefStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(MakeNumberRefStmt),
                       json::Object << member_number("index", _(Idx))
                                    << member_number("target", _(Target))));
          },

        In(Block) *
            (T(MakeObjectStmt)[MakeObjectStmt] << T(LocalIndex)[Target]) >>
          [files](Match& _) {
            return json::Object
              << member("type", "MakeObjectStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(MakeObjectStmt),
                       json::Object << member_number("target", _(Target))));
          },

        In(Block) * (T(MakeSetStmt)[MakeSetStmt] << T(LocalIndex)[Target]) >>
          [files](Match& _) {
            return json::Object
              << member("type", "MakeSetStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(MakeSetStmt),
                       json::Object << member_number("target", _(Target))));
          },

        In(Block) *
            (T(NotEqualStmt)[NotEqualStmt]
             << (T(json::Object)[Lhs] * T(json::Object)[Rhs])) >>
          [files](Match& _) {
            return json::Object << member("type", "NotEqualStmt")
                                << member(
                                     "stmt",
                                     add_loc(
                                       files,
                                       _(NotEqualStmt),
                                       json::Object << member("a", _(Lhs))
                                                    << member("b", _(Rhs))));
          },

        In(Block) * (T(NotStmt)[NotStmt] << T(json::Object)[Block]) >>
          [files](Match& _) {
            return json::Object
              << member("type", "NotStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(NotStmt),
                       json::Object << member("block", _(Block))));
          },

        In(Block) *
            (T(ObjectInsertOnceStmt)[ObjectInsertOnceStmt]
             << (T(json::Object)[Key] * T(json::Object)[Val] *
                 T(LocalIndex)[Object])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ObjectInsertOnceStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ObjectInsertOnceStmt),
                       json::Object << member("key", _(Key))
                                    << member("value", _(Val))
                                    << member_number("object", _(Object))));
          },

        In(Block) *
            (T(ObjectInsertStmt)[ObjectInsertStmt]
             << (T(json::Object)[Key] * T(json::Object)[Val] *
                 T(LocalIndex)[Object])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ObjectInsertStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ObjectInsertStmt),
                       json::Object << member("key", _(Key))
                                    << member("value", _(Val))
                                    << member_number("object", _(Object))));
          },

        In(Block) *
            (T(ObjectMergeStmt)[ObjectMergeStmt]
             << (T(LocalIndex)[Lhs] * T(LocalIndex)[Rhs] *
                 T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ObjectMergeStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ObjectMergeStmt),
                       json::Object << member_number("a", _(Lhs))
                                    << member_number("b", _(Rhs))
                                    << member_number("target", _(Target))));
          },

        In(Block) *
            (T(ResetLocalStmt)[ResetLocalStmt] << (T(LocalIndex)[Target])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ResetLocalStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ResetLocalStmt),
                       json::Object << member_number("target", _(Target))));
          },

        In(Block) *
            (T(ResultSetAddStmt)[ResultSetAddStmt] << (T(LocalIndex)[Val])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ResultSetAddStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ResultSetAddStmt),
                       json::Object << member_number("value", _(Val))));
          },

        In(Block) *
            (T(ReturnLocalStmt)[ReturnLocalStmt] << (T(LocalIndex)[Src])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ReturnLocalStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ReturnLocalStmt),
                       json::Object << member_number("source", _(Src))));
          },

        In(Block) *
            (T(ScanStmt)[ScanStmt]
             << (T(LocalIndex)[Src] * T(LocalIndex)[Key] * T(LocalIndex)[Val] *
                 T(json::Object)[Block])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "ScanStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(ScanStmt),
                       json::Object << member_number("source", _(Src))
                                    << member_number("key", _(Key))
                                    << member_number("value", _(Val))
                                    << member("block", _(Block))));
          },

        In(Block) *
            (T(SetAddStmt)[SetAddStmt]
             << (T(json::Object)[Operand] * T(LocalIndex)[Set])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "SetAddStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(SetAddStmt),
                       json::Object << member("value", _(Operand))
                                    << member_number("set", _(Set))));
          },

        In(Int32Seq) * T(Int32)[Int32] >>
          [](Match& _) { return json::Number ^ _(Int32); },

        In(Block) *
            (T(WithStmt)[WithStmt]
             << (T(LocalIndex)[LocalIndex] * T(Int32Seq)[Int32Seq] *
                 T(json::Object)[Val] * T(json::Object)[Block])) >>
          [files](Match& _) {
            return json::Object
              << member("type", "WithStmt")
              << member(
                     "stmt",
                     add_loc(
                       files,
                       _(WithStmt),
                       json::Object
                         << member_number("local", _(LocalIndex))
                         << member("path", json::Array << *_(Int32Seq))
                         << member("value", _(Val))
                         << member("block", _(Block))));
          },

        T(Block)[Block] >>
          [](Match& _) {
            return json::Object << member("stmts", json::Array << *_(Block));
          },

        T(BlockSeq)[Seq] >> [](Match& _) { return json::Array << *_(Seq); },

        T(StringSeq)[Seq] >>
          [](Match& _) {
            Node stringarray = NodeDef::create(json::Array);
            for (Node str : *_(Seq))
            {
              stringarray << (json::Object << member("value", str));
            }
            return stringarray;
          },

        T(bi::Name, bi::Description)[json::String] >>
          [](Match& _) {
            return json::String ^
              add_quotes(json::escape(_(json::String)->location().view()));
          },

        T(bi::Type) << T(bi::Any) >>
          [](Match& _) { return json::Object << member("type", "any"); },

        T(bi::Type) << T(bi::String) >>
          [](Match& _) { return json::Object << member("type", "string"); },

        T(bi::Type) << T(bi::Number) >>
          [](Match& _) { return json::Object << member("type", "number"); },

        T(bi::Type) << T(bi::Boolean) >>
          [](Match& _) { return json::Object << member("type", "boolean"); },

        T(bi::Type) << T(bi::Null) >>
          [](Match& _) { return json::Object << member("type", "null"); },

        T(bi::Type) << (T(bi::DynamicArray) << T(json::Object)[bi::Type]) >>
          [](Match& _) {
            return json::Object << member("type", "array")
                                << member("dynamic", _(bi::Type));
          },

        T(bi::Type) << T(bi::StaticArray)[bi::StaticArray] >>
          [](Match& _) {
            return json::Object
              << member("type", "array")
              << member("static", json::Array << *_(bi::StaticArray));
          },

        T(bi::Type)
            << (T(bi::DynamicObject)
                << (T(json::Object)[Key] * T(json::Object)[Val])) >>
          [](Match& _) {
            return json::Object << member("type", "object")
                                << member(
                                     "dynamic",
                                     json::Object << member("key", _(Key))
                                                  << member("value", _(Val)));
          },

        T(bi::ObjectItem)
            << (T(json::String)[Name] * T(json::Object)[bi::Type]) >>
          [](Match& _) {
            return json::Object << member("key", _(Name))
                                << member("value", _(bi::Type));
          },

        T(bi::Type) << T(bi::StaticObject)[bi::StaticObject] >>
          [](Match& _) {
            return json::Object
              << member("type", "object")
              << member("static", json::Array << *_(bi::StaticObject));
          },

        T(bi::Type)
            << (T(bi::HybridObject)
                << ((T(bi::DynamicObject)
                     << (T(json::Object)[Key] * T(json::Object)[Val])) *
                    T(bi::StaticObject)[bi::StaticObject])) >>
          [](Match& _) {
            return json::Object
              << member("type", "object")
              << member(
                     "dynamic",
                     json::Object << member("key", _(Key))
                                  << member("value", _(Val)))
              << member("static", json::Array << *_(bi::StaticObject));
          },

        T(bi::Type) << (T(bi::Set) << T(json::Object)[bi::Type]) >>
          [](Match& _) {
            return json::Object << member("type", "set")
                                << member("of", _(bi::Type));
          },

        T(bi::Type) << T(bi::TypeSeq)[bi::TypeSeq] >>
          [](Match& _) {
            return json::Object << member("type", "any")
                                << member("of", json::Array << *_(bi::TypeSeq));
          },

        T(bi::Arg, bi::Result)
            << (T(json::String)[bi::Name] * T(json::String)[bi::Description] *
                T(json::Object)[bi::Type]) >>
          [](Match& _) {
            Node obj = _(bi::Type);
            if (_(bi::Name)->location().len > 2)
            {
              obj << member("name", _(bi::Name));
            }

            if (_(bi::Description)->location().len > 2)
            {
              obj << member("description", _(bi::Description));
            }

            return obj;
          },

        T(bi::Decl)
            << (T(bi::ArgSeq)[bi::ArgSeq] * T(json::Object)[bi::Result]) >>
          [](Match& _) {
            return json::Object << member("args", json::Array << *_(bi::ArgSeq))
                                << member("result", _(bi::Result));
          },

        T(bi::Decl) << (T(bi::ArgSeq)[bi::ArgSeq] * T(bi::Void)) >>
          [](Match& _) {
            return json::Object
              << member("args", json::Array << *_(bi::ArgSeq));
          },

        T(bi::Decl) << (T(bi::VarArgs) * T(json::Object)[bi::Result]) >>
          [](Match& _) {
            return json::Object << member("result", _(bi::Result));
          },

        T(bi::Decl) << (T(bi::VarArgs) * T(bi::Void)) >>
          [](Match& _) { return NodeDef::create(json::Object); },

        In(BuiltInFunctionSeq) *
            (T(BuiltInFunction)
             << (T(json::String)[Name] * T(json::Object)[bi::Decl])) >>
          [](Match& _) {
            return json::Object << member("name", _(Name))
                                << member("decl", _(bi::Decl))
                                << member("type", "function");
          },

        In(Policy) *
            (T(Static)
             << (T(json::Array)[StringSeq] * T(PathSeq) *
                 T(BuiltInFunctionSeq)[BuiltInFunctionSeq])) >>
          [files](Match& _) {
            Node file_array = NodeDef::create(json::Array);
            for (auto& file : *files)
            {
              std::filesystem::path path(file);
              file_array
                << (json::Object << member(
                      "value",
                      json::String ^ add_quotes(path.filename().string())));
            }
            return json::Object
              << member("strings", _(StringSeq))
              << member("builtin_funcs", json::Array << *_(BuiltInFunctionSeq))
              << member("files", file_array);
          },

        In(PlanSeq) *
            (T(Plan) << (T(json::String)[Name] * T(json::Array)[BlockSeq])) >>
          [](Match& _) {
            return json::Object << member("name", _(Name))
                                << member("blocks", _(BlockSeq));
          },

        In(ParameterSeq) * T(LocalIndex)[LocalIndex] >>
          [](Match& _) { return json::Number ^ _(LocalIndex); },

        In(FunctionSeq) *
            (T(Function)
             << (T(json::String)[Name] * T(IRPath)[IRPath] *
                 T(ParameterSeq)[ParameterSeq] * T(LocalIndex)[Return] *
                 T(json::Array)[BlockSeq])) >>
          [](Match& _) {
            return json::Object
              << member("name", _(Name))
              << member("path", json::Array << *_(IRPath))
              << member("params", json::Array << *_(ParameterSeq))
              << member_number("return", _(Return))
              << member("blocks", _(BlockSeq));
          },

        (T(Term) << (T(Scalar) << T(Int, Float)[json::Number])) >>
          [](Match& _) { return json::Number ^ _(json::Number); },

        (T(Term) << (T(Scalar) << T(JSONString)[json::String])) >>
          [](Match& _) {
            return json::String ^
              add_quotes(_(json::String)->location().view());
          },

        (T(Term) << (T(Scalar) << T(True)[json::True])) >>
          [](Match& _) { return json::True ^ _(json::True); },

        (T(Term) << (T(Scalar) << T(False)[json::False])) >>
          [](Match& _) { return json::False ^ _(json::False); },

        (T(Term) << (T(Scalar) << T(Null)[json::Null])) >>
          [](Match& _) { return json::Null ^ _(json::Null); },

        (T(Term) << (T(Array, Set)[json::Array])) >>
          [](Match& _) {
            Node array = _(json::Array);
            Nodes children(array->begin(), array->end());
            if (array == Set)
            {
              NodeMap<std::string> strs;
              std::sort(
                children.begin(), children.end(), [&strs](Node a, Node b) {
                  std::string lhs;
                  if (strs.contains(a))
                  {
                    lhs = strs[a];
                  }
                  else
                  {
                    lhs = json::to_string(a);
                    strs[a] = lhs;
                  }

                  std::string rhs;
                  if (strs.contains(b))
                  {
                    rhs = strs[b];
                  }
                  else
                  {
                    rhs = json::to_string(b);
                    strs[b] = rhs;
                  }

                  return lhs < rhs;
                });
            }
            return json::Array << children;
          },

        (T(Term, Data) << (T(Object)[json::Object])) >>
          [](Match& _) {
            Node object = _(json::Object);
            std::vector<std::pair<std::string, Node>> members;
            for (auto& child : *object)
            {
              std::string key = strip_quotes(json::to_string(child / Key));
              members.push_back({key, child / Val});
            }

            std::sort(members.begin(), members.end(), [](auto& a, auto& b) {
              return a.first < b.first;
            });

            Nodes children;
            std::transform(
              members.begin(),
              members.end(),
              std::back_inserter(children),
              [](auto& member) {
                return json::Member << (json::Key ^ member.first)
                                    << member.second;
              });

            return json::Object << children;
          },

        In(RegoBundle) * (T(Data) << T(json::Object)[json::Object]) >>
          [](Match& _) { return _(json::Object); },

        In(RegoBundle) *
            (T(Policy)
             << (T(json::Object)[Static] * T(PlanSeq)[PlanSeq] *
                 T(IRQuery)[IRQuery] * T(FunctionSeq)[FunctionSeq])) >>
          [](Match& _) {
            Node policy = json::Object
              << member("static", _(Static))
              << member("plans",
                        json::Object
                          << member("plans", json::Array << *_(PlanSeq)))
              << member("funcs",
                        json::Object
                          << member("funcs", json::Array << *_(FunctionSeq)));
            if (_(IRQuery)->front() == json::String)
            {
              policy << member("query", _(IRQuery)->front());
            }

            return policy;
          },

        In(RegoBundle) * T(ModuleFileSeq)[ModuleFileSeq] >>
          [](Match& _) -> Node { return nullptr; },

        In(Top) *
            (T(RegoBundle)
             << (T(json::Object)[Data] * T(json::Object)[Policy])) >>
          [](Match& _) { return Seq << _(Data) << _(Policy); },
      },
    };

    pass.pre([files](Node top) {
      files->clear();

      Node rego = top / RegoBundle;
      Node pathseq = top / RegoBundle / Policy / Static / PathSeq;
      for (Node file : *pathseq)
      {
        std::filesystem::path path(file->location().view());
        files->push_back(path.string());
      }

      return 0;
    });

    return pass;
  }
}

namespace rego
{
  Rewriter json_to_bundle()
  {
    return {"json_to_bundle", {::json_to_bundle()}, json::wf};
  }

  Rewriter bundle_to_json()
  {
    return {"bundle_to_json", {::bundle_to_json()}, wf_bundle};
  }
}