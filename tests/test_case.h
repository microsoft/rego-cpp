#pragma once

#include "rego.h"

#include <string>
#include <vector>

namespace rego_test
{
  using namespace rego;

  using BindingMap = std::map<std::string, std::string>;

  struct Result
  {
    bool passed;
    std::string error;
  };

  class TestCase
  {
  public:
    /** Loads a test case from a YAML file */
    static std::vector<TestCase> load(
      const std::filesystem::path& path,
      const std::filesystem::path& debug_path = "");

    Result run(const std::filesystem::path& executable_path, const std::filesystem::path& debug_path) const;

    /** name of the test case category. */
    const std::string& category() const;

    /** name of file that case was loaded from */
    const std::filesystem::path& filename() const;
    TestCase& filename(const std::filesystem::path& filename);

    /** globally unique identifier for this test case */
    const std::string& note() const;
    TestCase& note(const std::string& note);

    /** policy query to execute */
    const std::string& query() const;
    TestCase& query(const std::string& query);

    /** policies to test against */
    const std::vector<std::string>& modules() const;
    TestCase& modules(const std::vector<std::string>& modules);

    /** data to tests against */
    const Node& data() const;
    TestCase& data(const Node& data);

    /** parsed input data to use */
    const Node& input() const;
    TestCase& input(const Node& input);

    /** raw input data (serialized to a string, overrides input) */
    const std::string& input_term() const;
    TestCase& input_term(const std::string& input_term);

    /** expect query result to be defined (or not) */
    bool want_defined() const;
    TestCase& want_defined(bool want_defined);

    /** expect query result (overrides defined) */
    const Node& want_result() const;
    TestCase& want_result(const Node& want_result);

    /** expect query error code (overrides result) */
    const std::string& want_error_code() const;
    TestCase& want_error_code(const std::string& want_error_code);

    /** expect query error message (overrides error code) */
    const std::string& want_error() const;
    TestCase& want_error(const std::string& want_error);

    /** indicates that binding values should be treated as sets */
    bool sort_bindings() const;
    TestCase& sort_bindings(bool sort_bindings);

    /** indicates that the error message depends on strict builtin error mode */
    bool strict_error() const;
    TestCase& strict_error(bool strict_error);

  private:
    BindingMap to_binding_map(const Node& node) const;
    bool compare(const Node& actual, const Node& wanted, std::ostream& os) const;
    bool compare(const std::string& actual, const std::string& wanted, std::ostream& os) const;

    static void write_ast(const std::filesystem::path& debug_path, std::size_t index, const std::string& pass, const Node& ast);
    static std::optional<Node> maybe_get_file(const Node& mapping, const std::string& name);
    static std::optional<std::string> maybe_get_string(const Node& mapping, const std::string& name);
    static std::string get_string(const Node& mapping, const std::string& name);
    static Node get_node(const Node& mapping, const std::string& name);
    static bool get_bool(const Node& mapping, const std::string& name);
    static std::vector<std::string> get_modules(const Node& mapping);
    static void diff(const std::string& actual, const std::string& wanted, std::ostream& os);
    static std::optional<TestCase> create_from_node(const Node& test_case_map);

    std::filesystem::path m_filename;
    std::string m_note;
    std::string m_category;
    std::string m_query;
    std::vector<std::string> m_modules;
    Node m_data;
    Node m_input;
    std::string m_input_term;
    bool m_want_defined;
    Node m_want_result;
    std::string m_want_error_code;
    std::string m_want_error;
    bool m_sort_bindings;
    bool m_strict_error;
  };

}
