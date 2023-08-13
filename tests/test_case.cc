#include "test_case.h"

#include "rego_test.h"

namespace
{
  using namespace rego_test;

  void write_ast(
    const std::filesystem::path& debug_path,
    std::size_t index,
    const std::string& pass,
    const Node& ast)
  {
    if (debug_path.empty())
    {
      return;
    }

    std::filesystem::path output;
    if (index < 10)
    {
      output =
        debug_path / ("0" + std::to_string(index) + "_" + pass + ".trieste");
    }
    else
    {
      output = debug_path / (std::to_string(index) + "_" + pass + ".trieste");
    }
    std::ofstream f(output, std::ios::binary | std::ios::out);

    if (f)
    {
      // Write the AST to the output file.
      f << "rego_test_cases" << std::endl << pass << std::endl << ast;
      return;
    }

    std::cerr << "Could not open " << output << " for writing." << std::endl;
  }

  bool has_error(const Node& node)
  {
    if (node->type() == Error)
    {
      return true;
    }

    for (auto& child : *node)
    {
      if (has_error(child))
      {
        return true;
      }
    }

    return false;
  }

  std::optional<Node> maybe_get_file(Node mapping, const std::string& name)
  {
    Location loc(name);
    Nodes defs = mapping->lookdown(loc);
    if (defs.size() > 1)
    {
      throw std::runtime_error("Expected exactly one " + name + " block");
    }

    if (defs.size() == 1)
    {
      Node val = defs[0]->back();
      assert(val->type() == File);
      return val;
    }

    return std::nullopt;
  }

  std::optional<std::string> maybe_get_string(
    Node mapping, const std::string& name)
  {
    Location loc(name);
    Nodes defs = mapping->lookdown(loc);
    if (defs.size() > 1)
    {
      throw std::runtime_error("Expected exactly one " + name + " block");
    }

    if (defs.size() == 1)
    {
      Node val = defs[0]->back();
      assert(val->type() == rego_test::Scalar);
      val = val->front();
      assert(val->type() == rego_test::String);
      return std::string(val->location().view());
    }

    return std::nullopt;
  }

  std::string get_string(Node mapping, const std::string& name)
  {
    auto maybe_string = maybe_get_string(mapping, name);
    if (maybe_string.has_value())
    {
      return *maybe_string;
    }

    return "";
  }

  Node get_node(Node mapping, const std::string& name)
  {
    Location loc(name);
    Nodes defs = mapping->lookdown(loc);
    if (defs.size() > 1)
    {
      throw std::runtime_error("Expected exactly one " + name + " block");
    }

    if (defs.size() == 1)
    {
      return defs[0]->back();
    }

    return {};
  }

  bool get_bool(Node mapping, const std::string& name)
  {
    Location loc(name);
    Nodes defs = mapping->lookdown(loc);
    if (defs.size() > 1)
    {
      throw std::runtime_error("Expected exactly one " + name + " block");
    }

    if (defs.size() == 1)
    {
      Node val = defs[0]->back();
      assert(val->type() == rego_test::Scalar);
      val = val->front();
      assert(val->type() == rego_test::True || val->type() == rego_test::False);
      return val->type() == rego_test::True;
    }

    return false;
  }

  std::vector<std::string> get_modules(Node mapping)
  {
    Location loc("modules");
    Nodes defs = mapping->lookdown(loc);
    if (defs.size() > 1)
    {
      throw std::runtime_error("Expected exactly one modules block");
    }

    std::vector<std::string> modules;
    if (defs.size() == 1)
    {
      Node module_nodes = defs[0]->back();
      assert(module_nodes->type() == rego_test::Sequence);
      for (auto& entry : *module_nodes)
      {
        Node scalar = entry->front();
        assert(scalar->type() == rego_test::Scalar);
        Node module = scalar->front();
        assert(module->type() == rego_test::String);
        modules.push_back(std::string(module->location().view()));
      }
    }

    return modules;
  }

  using BindingMap = std::map<std::string, std::string>;

  BindingMap to_binding_map(const Node& node)
  {
    BindingMap bindings;
    for (auto& binding : *node)
    {
      std::string key = std::string((binding / rego::Var)->location().view());
      std::string value = rego::to_json((binding / rego::Term));
      bindings[key] = value;
    }

    return bindings;
  }

  void diff(std::string actual, std::string wanted, std::ostream& os)
  {
    std::set<std::size_t> errors;
    std::size_t index = 0;
    std::size_t length = std::min(actual.size(), wanted.size());
    for (; index < length; ++index)
    {
      if (actual[index] != wanted[index])
      {
        errors.insert(index);
      }
    }

    length = std::max(actual.size(), wanted.size());
    for (; index < length; ++index)
    {
      errors.insert(index);
    }

    os << "wanted: " << wanted << std::endl;
    os << "  actual: " << actual << std::endl;
    os << "          ";
    for (std::size_t i = 0; i < length; ++i)
    {
      if (errors.contains(i))
      {
        os << "^";
      }
      else
      {
        os << " ";
      }
    }
  }

  bool compare(Node actual, Node wanted, std::ostream& os)
  {
    BindingMap actual_bindings = to_binding_map(actual);
    BindingMap wanted_bindings = to_binding_map(wanted);
    for (auto& [key, value] : wanted_bindings)
    {
      if (!actual_bindings.contains(key))
      {
        os << "Missing binding for " << key << std::endl;
        return false;
      }

      if (actual_bindings[key] != value)
      {
        diff(key + " = " + actual_bindings[key], key + " = " + value, os);
        return false;
      }
    }

    return true;
  }
}

namespace rego_test
{
  std::optional<std::vector<TestCase>> TestCase::load(
    const std::filesystem::path& path, const std::filesystem::path& debug_path)
  {
    if(!debug_path.empty()){
    if(std::filesystem::is_directory(debug_path)){
      std::filesystem::remove_all(debug_path);
    }

    std::filesystem::create_directory(debug_path);
    }

    auto ast = parser().parse(path);
    bool ok = wf_parser.build_st(ast, std::cerr);
    ok = wf_parser.check(ast, std::cerr) && ok;

    write_ast(debug_path, 0, "parse", ast);
    if (!ok)
    {
      std::ostringstream buf;
      ast->errors(buf);
      throw std::runtime_error(buf.str());
    }

    auto passes = rego_test::passes();

    for (std::size_t i = 0; i < passes.size(); ++i)
    {
      auto& [pass_name, pass, wf] = passes[i];
      wf::push_back(wf);
      auto [new_ast, count, changes] = pass->run(ast);
      wf::pop_front();
      ast = new_ast;

      ok = wf->build_st(ast, std::cout);
      write_ast(debug_path, i + 1, pass_name, ast);

      ok = wf->check(ast, std::cout) && ok;

      if (has_error(ast))
      {
        ok = false;
      }

      if (!ok)
      {
        std::ostringstream buf;
        buf << "Failed at pass " << pass_name << std::endl;
        ast->errors(buf);
        std::cerr << buf.str() << std::endl;
        return std::nullopt;
      }
    }

    std::vector<TestCase> test_cases;
    Node case_seq = ast->front()->front()->back();
    for (Node entry : *case_seq)
    {
      Node test_case = entry->front();
      test_cases.push_back(
        TestCase::create_from_node(test_case).filename(path));
    }

    return test_cases;
  }

  TestCase TestCase::create_from_node(const Node& test_case_map)
  {
    TestCase test_case;
    auto note = maybe_get_string(test_case_map, "note");
    if (note.has_value())
    {
      test_case = test_case.note(*note);
    }
    else
    {
      throw std::runtime_error("Note is required");
    }

    auto query = maybe_get_string(test_case_map, "query");
    if (query.has_value())
    {
      test_case = test_case.query(*query);
    }
    else
    {
      throw std::runtime_error("Query is required");
    }

    auto data = maybe_get_file(test_case_map, "data");
    if (data.has_value())
    {
      test_case = test_case.data(*data);
    }

    auto input = maybe_get_file(test_case_map, "input");
    if (input.has_value())
    {
      test_case = test_case.input(*input);
    }

    return test_case.modules(get_modules(test_case_map))
      .input_term(get_string(test_case_map, "input_term"))
      .want_defined(get_bool(test_case_map, "want_defined"))
      .want_result(get_node(test_case_map, "want_result"))
      .want_error_code(get_string(test_case_map, "want_error_code"))
      .want_error(get_string(test_case_map, "want_error"))
      .sort_bindings(get_bool(test_case_map, "sort_bindings"))
      .strict_error(get_bool(test_case_map, "strict_error"));
  }

  Result TestCase::run(const std::filesystem::path& executable_path, const std::filesystem::path& debug_path) const
  {
    rego::Interpreter interpreter;
    interpreter.executable(executable_path);
    if(!debug_path.empty() > 0){
      interpreter.debug_enabled(true);
      interpreter.debug_path(debug_path);
    }

    for (std::size_t i = 0; i < m_modules.size(); ++i)
    {
      std::string name = "module" + std::to_string(i);
      interpreter.add_module(name, m_modules[i]);
    }
    interpreter.add_data(m_data);
    if (m_input_term.size() > 0)
    {
      interpreter.add_input_json(m_input_term);
    }
    else
    {
      interpreter.add_input(m_input);
    }

    bool pass = true;
    std::ostringstream error;
    Node actual = interpreter.raw_query(m_query);
    if (m_want_error.length() > 0)
    {
      pass = false;
      error << "Unsupported test option";
    }
    else if (m_want_error_code.length() > 0)
    {
      pass = false;
      error << "Unsupported test option";
    }
    else if (m_want_result)
    {
      if(actual->front()->type() != Undefined){
        pass = compare(actual, m_want_result, error);
      }else{
        pass = false;
        error << "undefined";
      }
    }
    else if (m_want_defined)
    {
      pass = actual->front()->type() != Undefined;
    }
    else
    {
      pass = actual->front()->type() == Undefined;
    }

    return {pass, error.str()};
  }

  const std::filesystem::path& TestCase::filename() const
  {
    return m_filename;
  }

  TestCase& TestCase::filename(const std::filesystem::path& filename)
  {
    m_filename = filename;
    return *this;
  }

  const std::string& TestCase::note() const
  {
    return m_note;
  }

  const std::string& TestCase::category() const
  {
    return m_category;
  }

  TestCase& TestCase::note(const std::string& note)
  {
    m_note = note;
    auto pos = m_note.find('/');
    if(pos == std::string::npos){
      m_category = "";
    }else{
      m_category = m_note.substr(0, pos);
    }

    return *this;
  }

  const std::string& TestCase::query() const
  {
    return m_query;
  }

  TestCase& TestCase::query(const std::string& query)
  {
    m_query = query;
    return *this;
  }

  const std::vector<std::string>& TestCase::modules() const
  {
    return m_modules;
  }

  TestCase& TestCase::modules(const std::vector<std::string>& modules)
  {
    m_modules = modules;
    return *this;
  }

  const Node& TestCase::data() const
  {
    return m_data;
  }

  TestCase& TestCase::data(const Node& data)
  {
    m_data = data;
    return *this;
  }

  const Node& TestCase::input() const
  {
    return m_input;
  }

  TestCase& TestCase::input(const Node& input)
  {
    m_input = input;
    return *this;
  }

  const std::string& TestCase::input_term() const
  {
    return m_input_term;
  }

  TestCase& TestCase::input_term(const std::string& input_term)
  {
    m_input_term = input_term;
    return *this;
  }

  bool TestCase::want_defined() const
  {
    return m_want_defined;
  }

  TestCase& TestCase::want_defined(bool want_defined)
  {
    m_want_defined = want_defined;
    return *this;
  }

  const Node& TestCase::want_result() const
  {
    return m_want_result;
  }

  TestCase& TestCase::want_result(const Node& want_result)
  {
    m_want_result = want_result;
    return *this;
  }

  const std::string& TestCase::want_error_code() const
  {
    return m_want_error_code;
  }

  TestCase& TestCase::want_error_code(const std::string& want_error_code)
  {
    m_want_error_code = want_error_code;
    return *this;
  }

  const std::string& TestCase::want_error() const
  {
    return m_want_error;
  }

  TestCase& TestCase::want_error(const std::string& want_error)
  {
    m_want_error = want_error;
    return *this;
  }

  bool TestCase::sort_bindings() const
  {
    return m_sort_bindings;
  }

  TestCase& TestCase::sort_bindings(bool sort_bindings)
  {
    m_sort_bindings = sort_bindings;
    return *this;
  }

  bool TestCase::strict_error() const
  {
    return m_strict_error;
  }

  TestCase& TestCase::strict_error(bool strict_error)
  {
    m_strict_error = strict_error;
    return *this;
  }
}