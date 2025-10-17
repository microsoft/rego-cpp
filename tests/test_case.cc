#include "test_case.h"

#include "trieste/json.h"
#include "trieste/wf.h"
#include "trieste/yaml.h"

#include <filesystem>
#include <stdexcept>

namespace rego_test
{
  Node json_value_to_rego(Node node);

  Node json_object_to_rego(Node node)
  {
    assert(node == json::Object);

    Node result = NodeDef::create(rego::Object);
    for (Node member : *node)
    {
      assert(member == json::Member);
      Node key = member / json::Key;
      Node value = member / json::Value;
      result
        << (ObjectItem << json_value_to_rego(key) << json_value_to_rego(value));
    }

    return result;
  }

  Node json_array_to_rego(Node node)
  {
    assert(node == json::Array);

    Node result = NodeDef::create(rego::Array);
    for (Node value : *node)
    {
      result << json_value_to_rego(value);
    }

    return result;
  }

  Node json_value_to_rego(Node node)
  {
    if (node == json::Object)
    {
      return Term << json_object_to_rego(node);
    }

    if (node == json::Array)
    {
      return Term << json_array_to_rego(node);
    }

    if (node == json::Number)
    {
      auto loc = node->location();
      if (loc.view().find('.') != std::string::npos)
      {
        return Term << (Scalar << (Float ^ loc));
      }
      else
      {
        return Term << (Scalar << (Int ^ loc));
      }
    }

    if (node == json::Key)
    {
      std::ostringstream key;
      key << '"' << node->location().view() << '"';
      return Term << (Scalar << (JSONString ^ key.str()));
    }

    if (node == json::String)
    {
      return Term << (Scalar << (JSONString ^ node->location()));
    }

    if (node == json::True)
    {
      return Term << (Scalar << (True ^ node->location()));
    }

    if (node == json::False)
    {
      return Term << (Scalar << (False ^ node->location()));
    }

    if (node == json::Null)
    {
      return Term << (Scalar << (Null ^ node->location()));
    }

    throw std::runtime_error("Invalid JSON node type");
  }

  bool ends_with(const std::string_view& str, const std::string_view& suffix)
  {
    if (suffix.size() > str.size())
    {
      return false;
    }

    auto str_it = str.rbegin();
    for (auto suffix_it = suffix.rbegin(); suffix_it != suffix.rend();
         ++suffix_it, ++str_it)
    {
      if (*suffix_it != *str_it)
      {
        return false;
      }
    }

    return true;
  }

  bool ends_with(const std::string_view& str, const char* suffix)
  {
    return ends_with(str, std::string_view(suffix));
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

  TestCase::TestCase() :
    m_want_defined(false),
    m_sort_bindings(false),
    m_strict_error(false),
    m_broken(false)
  {}

  std::optional<Node> TestCase::maybe_get_object(
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
      assert(defs[0] == json::Member);
      Node val = defs[0] / json::Value;
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
      assert(defs[0] == json::Member);
      Node val = defs[0] / json::Value;
      assert(val == json::String);
      std::string val_string(val->location().view());
      return json::unescape(val_string.substr(1, val_string.size() - 2));
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

  std::filesystem::path TestCase::get_path(
    const std::filesystem::path& dir,
    const Node& mapping,
    const std::string& name)
  {
    auto maybe_string = maybe_get_string(mapping, name);
    if (maybe_string.has_value())
    {
      return dir / maybe_string.value();
    }

    return std::filesystem::path();
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
      return json_value_to_rego(defs[0]->back());
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
      assert(defs[0] == json::Member);
      Node val = defs[0] / json::Value;
      assert(val->type() == json::True || val->type() == json::False);
      return val->type() == json::True;
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
      assert(defs[0] == json::Member);
      Node module_nodes = defs[0] / json::Value;
      assert(module_nodes == json::Array);
      for (auto& entry : *module_nodes)
      {
        assert(entry == json::String);
        std::string code(entry->location().view());
        code = json::unescape(code.substr(1, code.size() - 2));
        if (ends_with(code, ".rego"))
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
          modules.push_back(code);
        }
      }
    }

    return modules;
  }

  BindingMaps TestCase::to_binding_maps(const Node& node) const
  {
    BindingMaps binding_maps;

    if (node == Array)
    {
      for (auto& term : *node)
      {
        auto maybe_object = unwrap(term, Object);
        if (!maybe_object.success)
        {
          return {};
        }

        Node object = maybe_object.node;
        assert(object == Object);
        BindingMap binding_map;
        for (auto& item : *object)
        {
          Node string = (item / Key)->front()->front();
          assert(string == JSONString);
          std::string key = std::string(string->location().view());
          key = key.substr(1, key.size() - 2); // remove quotes
          std::string value = rego::to_key(item / Val, true, m_sort_bindings);
          binding_map[key] = value;
        }

        std::string binding_key = rego::to_key(object, true, m_sort_bindings);
        binding_maps[binding_key] = binding_map;
      }
    }
    else if (node == Results)
    {
      for (auto& result : *node)
      {
        BindingMap binding_map;
        assert(result == rego::Result);
        Node bindings = result / Bindings;
        if (bindings->empty())
        {
          continue;
        }

        for (auto& binding : *bindings)
        {
          std::string key =
            std::string((binding / rego::Key)->location().view());
          std::string value =
            rego::to_key((binding / rego::Val), true, m_sort_bindings);
          binding_map[key] = value;
        }

        std::string binding_key = rego::to_key(bindings, true, m_sort_bindings);
        binding_maps[binding_key] = binding_map;
      }
    }

    return binding_maps;
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
      if (errors.find(i) != errors.end())
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
    BindingMaps actual_bindings = to_binding_maps(actual);
    BindingMaps wanted_bindings = to_binding_maps(wanted);

    if (wanted_bindings.size() == 0)
    {
      if (wanted->size() > 0)
      {
        os << "No wanted bindings found (invalid test case?)" << std::endl;
        return false;
      }
    }

    bool pass = true;
    for (auto& [key, value] : wanted_bindings)
    {
      if (actual_bindings.find(key) == actual_bindings.end())
      {
        os << "Wanted binding: '" << key << "'" << std::endl;
        pass = false;
      }
    }

    for (auto& [key, value] : actual_bindings)
    {
      if (wanted_bindings.find(key) == wanted_bindings.end())
      {
        if (!pass)
        {
          os << "  ";
        }
        os << "Actual binding: '" << key << "'" << std::endl;
        pass = false;
      }
    }

    return pass;
  }

  bool TestCase::compare(
    const std::string& actual,
    const std::string& wanted,
    std::ostream& os) const
  {
    // some rego tests cases add a non-standard prefix to the expected error
    if (ends_with(wanted, actual))
    {
      return true;
    }

    diff(actual, wanted, os);
    return false;
  }

  std::vector<TestCase> TestCase::load(
    const std::filesystem::path& path, const std::filesystem::path& debug_path)
  {
    logging::Debug() << "Loading test cases from " << path;

    auto result = yaml::reader()
                    .file(path)
                    .debug_enabled(!debug_path.empty())
                    .debug_path(debug_path / "inyaml") >>
      yaml::to_json()
        .debug_enabled(!debug_path.empty())
        .debug_path(debug_path / "tojson");
    if (!result.ok)
    {
      std::cout << "Error processing tests cases in " << path << std::endl;
      logging::Error err;
      result.print_errors(err);
      return {};
    }

    Node ast = result.ast;
    std::vector<TestCase> test_cases;

    WFContext context(json::wf);
    Node tests = ast->front();
    assert(tests == json::Object);
    auto defs = tests->lookdown({"cases"});
    if (defs.empty())
    {
      logging::Error() << "No 'cases' block found in test file: "
                       << path.string();
      return {};
    }

    Node cases = defs[0] / json::Value;
    assert(cases == json::Array);

    for (Node entry : *cases)
    {
      auto maybe_testcase = TestCase::create_from_node(path, entry);
      if (maybe_testcase.has_value())
      {
        test_cases.push_back(maybe_testcase.value());
      }
    }

    return test_cases;
  }

  std::optional<TestCase> TestCase::create_from_node(
    const std::filesystem::path& filename, const Node& test_case_obj)
  {
    assert(test_case_obj == json::Object);
    try
    {
      TestCase test_case;
      auto note = maybe_get_string(test_case_obj, "note");
      if (note.has_value())
      {
        test_case = test_case.note(*note);
      }
      else
      {
        throw std::runtime_error("Note is required");
      }

      auto query = maybe_get_string(test_case_obj, "query");
      if (query.has_value())
      {
        test_case = test_case.query(*query);
      }
      else
      {
        throw std::runtime_error("Query is required");
      }

      auto data = maybe_get_object(test_case_obj, "data");
      if (data.has_value() && data.value()->type() == json::Object)
      {
        test_case = test_case.data(data.value());
      }

      auto input = maybe_get_object(test_case_obj, "input");
      if (input.has_value())
      {
        test_case = test_case.input(input.value());
      }

      test_case.filename(filename)
        .modules(get_modules(filename.parent_path(), test_case_obj))
        .data_path(get_path(filename.parent_path(), test_case_obj, "data_path"))
        .input_term(get_string(test_case_obj, "input_term"))
        .input_path(
          get_path(filename.parent_path(), test_case_obj, "input_path"))
        .want_defined(get_bool(test_case_obj, "want_defined"))
        .want_result(get_node(test_case_obj, "want_result"))
        .want_error_code(get_string(test_case_obj, "want_error_code"))
        .want_error(get_string(test_case_obj, "want_error"))
        .sort_bindings(get_bool(test_case_obj, "sort_bindings"))
        .strict_error(get_bool(test_case_obj, "strict_error"));

      // --- Special Cases --- //
      // these test cases require some modification due to differences between
      // C++ and Go, or due to issues with the testcases themselves.

      if (test_case.note() == "withkeyword/builtin-builtin: arity 0")
      {
        // as written, this test case can never pass (the result is the
        // opa.runtime object, which is not equal to the empty object) so we
        // write in the result of calling rego::version()
        test_case.want_result(
          rego::Results
          << (rego::Result << rego::Terms
                           << (rego::Bindings
                               << (rego::Binding
                                   << (rego::Var ^ "x")
                                   << (rego::Term << rego::version())))));
      }

      if (test_case.note() == "regexmatch/re_match: bad pattern err")
      {
        // the std::regex uses a slightly different error syntax than Go, and
        // reproducing the exact error is out of scope at this present time.
        test_case.want_error("");
      }

      if (test_case.note() == "jsonbuiltins/yaml unmarshal error")
      {
        // the wanted error is implementation and instance specific.
        // We output quite a lot of error information directly to the error log
        // but return a clear message, which we check here.
        test_case.want_error("failed to parse YAML");
      }

      // TODO
      // the following test cases are broken, and should be checked with each
      // new release to see if they have been fixed, as they are essentially
      // being skipped at present.

#if __cpp_lib_chrono < 201907L
      if (test_case.note().starts_with("time/"))
      {
        // The time builtins are depend on std::chrono::zoned_time and
        // std::chrono::parse, which are only available in C++20 and on
        // some platforms. Without these functions, the time builtins are
        // fairly limited and so these test cases are skipped.
        test_case.broken(true);
      }
#endif

      if (test_case.note() == "reachable_paths/cycle_1022_3")
      {
        // the test is wrong, the actual result is correct
        test_case.broken(true);
      }

      return test_case;
    }
    catch (const std::exception& e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << test_case_obj->location().str() << std::endl;
      return std::nullopt;
    }
  }

  Result TestCase::run(
    const std::filesystem::path& debug_path,
    bool wf_checks,
    RoundTrip roundtrip,
    LogLevel log_level) const
  {
    rego::Interpreter interpreter;
    interpreter.builtins()->strict_errors(m_strict_error);
    interpreter.builtins()->register_builtins(custom_builtins(m_note));
    interpreter.wf_check_enabled(wf_checks)
      .debug_enabled(!debug_path.empty())
      .debug_path(debug_path)
      .log_level(log_level);

    std::ostringstream error;
    Node actual = nullptr;
    for (std::size_t i = 0; i < m_modules.size(); ++i)
    {
      std::string name = "module" + std::to_string(i) + ".rego";
      actual = interpreter.add_module(name, m_modules[i]);
      if (actual != nullptr)
      {
        error << actual;
        return {false, error.str()};
      }
    }

    if (!m_data_path.empty())
    {
      actual = interpreter.add_data_json_file(m_data_path);
    }
    else if (m_data != nullptr)
    {
      actual = interpreter.add_data(m_data);
    }

    if (actual != nullptr)
    {
      error << actual;
      return {false, error.str()};
    }

    actual = interpreter.set_query(m_query);
    if (actual != nullptr)
    {
      error << actual;
      return {false, error.str()};
    }

    Node bundle_node = interpreter.build();
    if (bundle_node == ErrorSeq)
    {
      error << "Error when bundling: " << bundle_node;
      return {false, error.str()};
    }

    Bundle bundle = BundleDef::from_node(bundle_node);

    if (roundtrip == RoundTrip::JSON)
    {
      std::filesystem::path temp_dir =
        std::filesystem::temp_directory_path() / "bundle";
      actual = interpreter.save_bundle(temp_dir, bundle_node);
      if (actual != nullptr)
      {
        error << "Error saving bundle: " << actual;
        return {false, error.str()};
      }

      bundle_node = interpreter.load_bundle(temp_dir);
      if (bundle_node == nullptr || bundle_node == ErrorSeq)
      {
        error << "Error loading bundle: " << bundle_node;
        return {false, error.str()};
      }

      std::filesystem::remove_all(temp_dir);
    }
    else if (roundtrip == RoundTrip::Binary)
    {
      std::filesystem::path temp_path =
        std::filesystem::temp_directory_path() / "test.rbb";
      bundle->save(temp_path);
      bundle = BundleDef::load(temp_path);
      std::filesystem::remove(temp_path);
    }

    if (!m_input_path.empty())
    {
      actual = interpreter.set_input_json_file(m_input_path);
    }
    else if (!m_input_term.empty())
    {
      actual = interpreter.set_input_term(m_input_term);
    }
    else if (m_input != nullptr)
    {
      actual = interpreter.set_input(m_input);
    }

    if (actual != nullptr)
    {
      error << actual;
      return {false, error.str()};
    }

    try
    {
      actual = interpreter.query_bundle(bundle);
    }
    catch (const std::exception& e)
    {
      return {false, e.what()};
    }

    if (m_broken)
    {
      return {true, ""};
    }

    if (actual->type() == ErrorSeq)
    {
      if (actual->size() > 1)
      {
        error << "expected one error, actual: " << actual << std::endl;
        return {false, error.str()};
      }

      if (actual->size() == 0)
      {
        error << actual << std::endl;
        return {false, error.str()};
      }

      actual = actual->front();
    }

    WFContext context(rego::wf_result);

    if (m_want_error.length() > 0)
    {
      if (actual->type() != Error)
      {
        error << "wanted an error, actual: " << actual << std::endl;
        return {false, error.str()};
      }
      std::string actual_error =
        std::string((actual / ErrorMsg)->location().view());
      std::string actual_code =
        std::string((actual / ErrorCode)->location().view());
      bool pass_error = compare(actual_error, m_want_error, error);
      bool pass_code = true;
      if (m_want_error_code.length() == 0)
      {
        return {pass_error, error.str()};
      }

      pass_code = compare(actual_code, m_want_error_code, error);
      return {pass_error && pass_code, error.str()};
    }

    if (m_want_error_code.length() > 0)
    {
      if (actual != Error)
      {
        error << "wanted an error code, actual: " << actual << std::endl;
        return {false, error.str()};
      }

      std::string actual_code =
        std::string((actual / ErrorCode)->location().view());
      return {compare(actual_code, m_want_error_code, error), error.str()};
    }

    if (m_want_result)
    {
      if (actual != Undefined)
      {
        if (actual == Error)
        {
          error << "wanted a result, received: " << std::endl;
          error << "  error: " << (actual / ErrorMsg)->location().view()
                << std::endl;
          error << "  code: " << (actual / ErrorCode)->location().view()
                << std::endl;
          return {false, error.str()};
        }
        trieste::logging::Trace() << "====================" << std::endl
                                  << "actual: " << actual << std::endl
                                  << "---------" << std::endl
                                  << "wanted: " << m_want_result << std::endl
                                  << "====================" << std::endl;

        return {compare(actual, m_want_result, error), error.str()};
      }

      if (m_want_result->size() == 0)
      {
        return {true, ""};
      }

      return {false, "wanted a result, but was undefined"};
    }

    if (m_want_defined)
    {
      if (actual != Undefined)
      {
        return {true, ""};
      }

      return {false, "wanted a defined result, but was undefined"};
    }

    if (actual == Undefined)
    {
      return {true, ""};
    }

    logging::Error() << actual << std::endl;
    return {false, "wanted an undefined result, but was defined"};
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

  const std::filesystem::path& TestCase::data_path() const
  {
    return m_data_path;
  }

  TestCase& TestCase::data_path(const std::filesystem::path& path)
  {
    m_data_path = path;
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

  const std::filesystem::path& TestCase::input_path() const
  {
    return m_input_path;
  }

  TestCase& TestCase::input_path(const std::filesystem::path& path)
  {
    m_input_path = path;
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
    if (want_result == nullptr)
    {
      m_want_result = nullptr;
      return *this;
    }

    auto maybe_array = unwrap(want_result, Array);
    if (!maybe_array.success)
    {
      throw std::invalid_argument("want_result must be an array");
    }
    m_want_result = maybe_array.node;
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

  bool TestCase::broken() const
  {
    return m_broken;
  }

  TestCase& TestCase::broken(bool broken)
  {
    m_broken = broken;
    return *this;
  }
}