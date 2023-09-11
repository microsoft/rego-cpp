#include "test_case.h"

#include "rego_test.h"

namespace rego_test
{

  void TestCase::write_ast(
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
      output = debug_path /
        ("0" + std::to_string(index) + "_yaml_" + pass + ".trieste");
    }
    else
    {
      output =
        debug_path / (std::to_string(index) + "_yaml_" + pass + ".trieste");
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

  std::optional<Node> TestCase::maybe_get_file(
    const Node& mapping, const std::string& name)
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
      if (val->type() != File)
      {
        return err(val, "Expected a File node");
      }
      return val;
    }

    return std::nullopt;
  }

  std::optional<std::string> TestCase::maybe_get_string(
    const Node& mapping, const std::string& name)
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

  std::string TestCase::get_string(const Node& mapping, const std::string& name)
  {
    auto maybe_string = maybe_get_string(mapping, name);
    if (maybe_string.has_value())
    {
      return *maybe_string;
    }

    return "";
  }

  Node TestCase::get_node(const Node& mapping, const std::string& name)
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

  bool TestCase::get_bool(const Node& mapping, const std::string& name)
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

  std::vector<std::string> TestCase::get_modules(
    const std::filesystem::path& dir, const Node& mapping)
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
        std::string code = std::string(module->location().view());
        if (code.ends_with(".rego"))
        {
          if (std::filesystem::exists(dir / code))
          {
            std::ifstream f(dir / code);
            std::string str(
              (std::istreambuf_iterator<char>(f)),
              std::istreambuf_iterator<char>());
            modules.push_back(str);
          }
          else
          {
            std::cerr << "Could not find module " << code << std::endl;
          }
        }
        else
        {
          modules.push_back(std::string(module->location().view()));
        }
      }
    }

    return modules;
  }

  BindingMap TestCase::to_binding_map(const Node& node) const
  {
    BindingMap bindings;
    for (auto& binding : *node)
    {
      if (binding->type() != rego::Binding)
      {
        // raw term
        continue;
      }

      std::string key = std::string((binding / rego::Var)->location().view());
      std::string value =
        rego::to_json((binding / rego::Term), m_sort_bindings, false);
      bindings[key] = value;
    }

    return bindings;
  }

  void TestCase::diff(
    const std::string& actual, const std::string& wanted_raw, std::ostream& os)
  {
    std::set<std::size_t> errors;
    std::size_t index = 0;
    std::size_t length = std::min(actual.size(), wanted_raw.size());
    std::string_view wanted = wanted_raw;
    if (wanted.size() > length)
    {
      wanted = wanted.substr(wanted.size() - length);
    }

    for (; index < length; ++index)
    {
      if (actual[index] != wanted[index])
      {
        errors.insert(index);
      }
    }

    length = actual.size();
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
    os << std::endl;
  }

  bool TestCase::compare(
    const Node& actual, const Node& wanted, std::ostream& os) const
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

    for (auto& [key, value] : actual_bindings)
    {
      if (!wanted_bindings.contains(key))
      {
        os << "Unexpected binding for " << key << std::endl;
        return false;
      }
    }

    return true;
  }

  bool TestCase::compare(
    const std::string& actual,
    const std::string& wanted,
    std::ostream& os) const
  {
    // some rego tests cases add a non-standard prefix to the expected error
    if (wanted.ends_with(actual))
    {
      return true;
    }

    diff(actual, wanted, os);
    return false;
  }

  std::vector<TestCase> TestCase::load(
    const std::filesystem::path& path, const std::filesystem::path& debug_path)
  {
    if (!debug_path.empty())
    {
      if (std::filesystem::is_directory(debug_path))
      {
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
    std::vector<TestCase> test_cases;

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
        buf << "Error with input file " << path << std::endl;
        std::cerr << buf.str() << std::endl;
        return test_cases;
      }
    }

    Node case_seq = ast->front()->front()->back();
    for (Node entry : *case_seq)
    {
      auto maybe_testcase = TestCase::create_from_node(path, entry->front());
      if (maybe_testcase.has_value())
      {
        test_cases.push_back(maybe_testcase.value());
      }
    }

    return test_cases;
  }

  std::optional<TestCase> TestCase::create_from_node(
    const std::filesystem::path& filename, const Node& test_case_map)
  {
    try
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

      test_case.filename(filename)
        .modules(get_modules(filename.parent_path(), test_case_map))
        .input_term(get_string(test_case_map, "input_term"))
        .want_defined(get_bool(test_case_map, "want_defined"))
        .want_result(get_node(test_case_map, "want_result"))
        .want_error_code(get_string(test_case_map, "want_error_code"))
        .want_error(get_string(test_case_map, "want_error"))
        .sort_bindings(get_bool(test_case_map, "sort_bindings"))
        .strict_error(get_bool(test_case_map, "strict_error"));

      // --- Special Cases --- //
      // these test cases require some modification due to differences between
      // C++ and Go, or due to issues with the testcases themselves.

      if (test_case.note() == "withkeyword/builtin-builtin: arity 0")
      {
        // as written, this test case can never pass (the result is the
        // opa.runtime object, which is not equal to the empty object) so we
        // write in the result of calling rego::version()
        Node want_result = NodeDef::create(WantResult);
        Node result = NodeDef::create(rego::Binding);
        result << (rego::Var ^ "x");
        result << (rego::Term << rego::version());
        test_case.want_result(want_result << result);
      }

      if (test_case.note() == "regexmatch/re_match: bad pattern err")
      {
        // the std::regex uses a slightly different error syntax than Go, and
        // reproducing the exact error is out of scope at this present time.
        test_case.want_error("");
      }

      return test_case;
    }
    catch (const std::exception& e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << test_case_map->location().str() << std::endl;
      return std::nullopt;
    }
  }

  Result TestCase::run(
    const std::filesystem::path& executable_path,
    const std::filesystem::path& debug_path,
    bool wf_checks) const
  {
    rego::Interpreter interpreter;
    interpreter.executable(executable_path);
    interpreter.well_formed_checks_enabled(wf_checks);
    interpreter.builtins().strict_errors(m_strict_error);
    if (!debug_path.empty())
    {
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
      interpreter.set_input_json(m_input_term);
    }
    else
    {
      interpreter.set_input(m_input);
    }

    bool pass = true;
    std::ostringstream error;
    Node actual;
    try
    {
      actual = interpreter.raw_query(m_query);
    }
    catch (const std::exception& e)
    {
      return {false, e.what()};
    }

    if (actual->type() == ErrorSeq)
    {
      if (actual->size() > 1)
      {
        error << "expected one error, actual: " << actual << std::endl;
        return {false, error.str()};
      }
      else if (actual->size() == 0)
      {
        error << actual << std::endl;
        return {false, error.str()};
      }
      else
      {
        actual = actual->front();
      }
    }

    if (m_want_error.length() > 0)
    {
      if (actual->type() != Error)
      {
        pass = false;
        error << "wanted an error, actual: " << actual << std::endl;
      }
      else
      {
        std::string actual_error =
          std::string((actual / ErrorMsg)->location().view());
        std::string actual_code =
          std::string((actual / ErrorCode)->location().view());
        bool pass_error = compare(actual_error, m_want_error, error);
        bool pass_code = true;
        if (m_want_error_code.length() > 0)
        {
          pass_code = compare(actual_code, m_want_error_code, error);
        }
        pass = pass_error && pass_code;
      }
    }
    else if (m_want_error_code.length() > 0)
    {
      std::string actual_code =
        std::string((actual / ErrorCode)->location().view());
      pass = compare(actual_code, m_want_error_code, error);
    }
    else if (m_want_result)
    {
      if (actual->size() > 0)
      {
        if (actual->type() == Error)
        {
          pass = false;
          error << "wanted a result, received: " << std::endl;
          error << "  error: " << (actual / ErrorMsg)->location().view()
                << std::endl;
          error << "  code: " << (actual / ErrorCode)->location().view()
                << std::endl;
        }
        else
        {
          pass = compare(actual, m_want_result, error);
        }
      }
      else
      {
        if (m_want_result->size() == 0)
        {
          pass = true;
        }
        else
        {
          pass = false;
          error << "wanted a result, but was undefined";
        }
      }
    }
    else if (m_want_defined)
    {
      if (actual->size() > 0)
      {
        pass = true;
      }
      else
      {
        pass = false;
        error << "wanted a defined result, but was undefined";
      }
    }
    else
    {
      if (actual->size() == 0)
      {
        pass = true;
      }
      else
      {
        std::cout << actual << std::endl;
        pass = false;
        error << "wanted an undefined result, but was defined";
      }
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
    if (pos == std::string::npos)
    {
      m_category = "";
    }
    else
    {
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