#include "builtins.h"
#include "rego.hh"
#include "trieste/json.h"

#include <regex>

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node error(
    const Node& pattern_node,
    const std::regex_constants::error_type& error_code)
  {
    switch (error_code)
    {
      case std::regex_constants::error_collate:
        // the expression contains an invalid collating element name
        return err(
          pattern_node,
          "error parsing regexp: invalid collating element name",
          EvalBuiltInError);

      case std::regex_constants::error_ctype:
        // the expression contains an invalid character class name
        return err(
          pattern_node,
          "error parsing regexp: invalid character class name",
          EvalBuiltInError);

      case std::regex_constants::error_escape:
        // the expression contains an invalid escaped character or a trailing
        // escape
        return err(
          pattern_node,
          "error parsing regexp: invalid escaped character or a trailing "
          "escape",
          EvalBuiltInError);

      case std::regex_constants::error_backref:
        // the expression contains an invalid back reference
        return err(
          pattern_node,
          "error parsing regexp: invalid back reference",
          EvalBuiltInError);

      case std::regex_constants::error_brack:
        // the expression contains mismatched square brackets ('[' and ']')
        return err(
          pattern_node,
          "error parsing regexp: missing closing ]",
          EvalBuiltInError);

      case std::regex_constants::error_paren:
        // the expression contains mismatched parentheses ('(' and ')')
        return err(
          pattern_node,
          "error parsing regexp: missing closing )",
          EvalBuiltInError);

      case std::regex_constants::error_brace:
        // the expression contains mismatched curly braces ('{' and '}')
        return err(
          pattern_node,
          "error parsing regexp: missing closing }",
          EvalBuiltInError);

      case std::regex_constants::error_badbrace:
        // the expression contains an invalid range in a {} expression
        return err(
          pattern_node,
          "error parsing regexp: invalid range in a {} expression",
          EvalBuiltInError);

      case std::regex_constants::error_range:
        // the expression contains an invalid character range (e.g. [b-a])
        return err(
          pattern_node,
          "error parsing regexp: invalid character range",
          EvalBuiltInError);

      case std::regex_constants::error_space:
        // there was not enough memory to convert the expression into a finite
        // state machine
        return err(
          pattern_node,
          "error parsing regexp: not enough memory",
          EvalBuiltInError);

      case std::regex_constants::error_badrepeat:
        // '*', '?', '+' or '{' was not preceded by a valid regular expression
        return err(
          pattern_node,
          "error parsing regexp: *, ?, + or { was not preceded by a valid "
          "regular expression",
          EvalBuiltInError);

      case std::regex_constants::error_complexity:
        // the complexity of an attempted match exceeded a predefined level
        return err(
          pattern_node,
          "error parsing regexp: the complexity of an attempted match "
          "exceeded a predefined level",
          EvalBuiltInError);

      case std::regex_constants::error_stack:
        // there was not enough memory to perform a match
        return err(
          pattern_node,
          "error parsing regexp: not enough memory",
          EvalBuiltInError);

      default:
        return err(pattern_node, "error parsing regexp", EvalBuiltInError);
    }
  }

  Node match(const Nodes& args)
  {
    Node pattern_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("regex.match"));
    if (pattern_node->type() == Error)
    {
      return pattern_node;
    }

    Node value_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("regex.match"));
    if (value_node->type() == Error)
    {
      return value_node;
    }

    std::string pattern = json::unescape(get_string(pattern_node));
    std::string value = get_string(value_node);

    try
    {
      std::regex re(pattern);
      bool match = std::regex_match(value, re);
      return Resolver::scalar(match);
    }
    catch (std::regex_error& e)
    {
      return error(pattern_node, e.code());
    }
  }

  Node match_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "pattern")
                               << (bi::Description ^ "regular expression")
                               << (bi::Type << bi::String))
                   << (bi::Arg
                       << (bi::Name ^ "value")
                       << (bi::Description ^ "value to match against `pattern`")
                       << (bi::Type << bi::String)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^ "true of `value` matches `pattern`")
                   << (bi::Type << bi::Boolean));

  Node is_valid(const Nodes& args)
  {
    Node pattern_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("regex.isvalid"));
    if (pattern_node->type() == Error)
    {
      return Resolver::scalar(false);
    }

    std::string pattern = json::unescape(get_string(pattern_node));

    try
    {
      std::regex re(pattern);
      return Resolver::scalar(true);
    }
    catch (std::regex_error&)
    {
      return Resolver::scalar(false);
    }
  }

  Node is_valid_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "pattern")
                             << (bi::Description ^ "regular expression")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "result")
                 << (bi::Description ^
                     "`true` if `pattern` is a valid regular expression")
                 << (bi::Type << bi::Boolean));

  Node replace(const Nodes& args)
  {
    Node s_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("regex.replace"));
    if (s_node->type() == Error)
    {
      return s_node;
    }

    Node pattern_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("regex.replace"));
    if (pattern_node->type() == Error)
    {
      return pattern_node;
    }

    Node value_node =
      unwrap_arg(args, UnwrapOpt(2).type(JSONString).func("regex.replace"));
    if (value_node->type() == Error)
    {
      return value_node;
    }

    std::string s = get_string(s_node);
    std::string pattern = json::unescape(get_string(pattern_node));
    std::string value = json::unescape(get_string(value_node));

    std::ostringstream os;

    try
    {
      std::regex re(pattern);
      os << std::regex_replace(s, re, value);
      return Resolver::scalar(os.str());
    }
    catch (std::regex_error& e)
    {
      return error(pattern_node, e.code());
    }
  }

  Node replace_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "s")
                               << (bi::Description ^ "string being processed")
                               << (bi::Type << bi::String))
                   << (bi::Arg
                       << (bi::Name ^ "pattern")
                       << (bi::Description ^ "regex pattern to be applied")
                       << (bi::Type << bi::String))
                   << (bi::Arg << (bi::Name ^ "value")
                               << (bi::Description ^ "regex value")
                               << (bi::Type << bi::String)))
    << (bi::Result << (bi::Name ^ "output")
                   << (bi::Description ^ "string with replaced substrings")
                   << (bi::Type << bi::String));

  Node find_n(const Nodes& args)
  {
    Node pattern_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("regex.find_n"));
    if (pattern_node->type() == Error)
    {
      return pattern_node;
    }

    Node value_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("regex.find_n"));
    if (value_node->type() == Error)
    {
      return value_node;
    }

    Node number_node =
      unwrap_arg(args, UnwrapOpt(2).type({Int}).func("regex.find_n"));
    if (number_node->type() == Error)
    {
      return number_node;
    }

    std::string pattern = json::unescape(get_string(pattern_node));
    std::string value = get_string(value_node);

    auto maybe_number = get_int(number_node).to_size();
    if (!maybe_number.has_value())
    {
      return err(number_node, "not a valid integer", EvalBuiltInError);
    }
    std::size_t number = maybe_number.value();

    std::regex re;
    try
    {
      re = std::regex(pattern);
    }
    catch (std::regex_error& e)
    {
      return error(pattern_node, e.code());
    }

    Node array = NodeDef::create(Array);
    std::smatch match;
    for (std::size_t i = 0; i < number && !value.empty(); ++i)
    {
      std::regex_search(value, match, re);
      if (match.empty())
      {
        break;
      }

      array->push_back(Resolver::scalar(match.str()));
      value = match.suffix();
    }

    return array;
  }

  Node find_n_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "pattern")
                             << (bi::Description ^ "regular expression")
                             << (bi::Type << bi::String))
                 << (bi::Arg << (bi::Name ^ "value")
                             << (bi::Description ^ "string to match")
                             << (bi::Type << bi::String))
                 << (bi::Arg
                     << (bi::Name ^ "number")
                     << (bi::Description ^
                         "number of matches to return; `-1` means all matches")
                     << (bi::Type << bi::Number)))
             << (bi::Result
                 << (bi::Name ^ "output")
                 << (bi::Description ^ "collected matches")
                 << (bi::Type
                     << (bi::DynamicArray << (bi::Type << bi::String))));

  Node find_all_string_submatch_n(const Nodes& args)
  {
    Node pattern_node = unwrap_arg(
      args,
      UnwrapOpt(0).type(JSONString).func("regex.find_all_string_submatch_n"));
    if (pattern_node->type() == Error)
    {
      return pattern_node;
    }

    Node value_node = unwrap_arg(
      args,
      UnwrapOpt(1).type(JSONString).func("regex.find_all_string_submatch_n"));
    if (value_node->type() == Error)
    {
      return value_node;
    }

    Node number_node = unwrap_arg(
      args, UnwrapOpt(2).type({Int}).func("regex.find_all_string_submatch_n"));
    if (number_node->type() == Error)
    {
      return number_node;
    }

    std::string pattern = json::unescape(get_string(pattern_node));
    std::string value = get_string(value_node);

    auto maybe_number = get_int(number_node).to_size();
    if (!maybe_number.has_value())
    {
      return err(number_node, "not a valid integer", EvalBuiltInError);
    }
    std::size_t number = maybe_number.value();

    std::regex re;
    try
    {
      re = std::regex(pattern);
    }
    catch (std::regex_error& e)
    {
      return error(pattern_node, e.code());
    }

    Node array = NodeDef::create(Array);
    std::smatch match;
    for (std::size_t i = 0; i < number && !value.empty(); ++i)
    {
      std::regex_search(value, match, re);
      if (match.empty())
      {
        break;
      }

      Node submatch_array = NodeDef::create(Array);
      for (std::size_t j = 0; j < match.size(); ++j)
      {
        submatch_array->push_back(Resolver::scalar(match[j].str()));
      }

      array->push_back(Term << submatch_array);
      value = match.suffix();
    }

    return array;
  }

  Node find_all_string_submatch_n_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg << (bi::Name ^ "pattern")
                             << (bi::Description ^ "regular expression")
                             << (bi::Type << bi::String))
                 << (bi::Arg << (bi::Name ^ "value")
                             << (bi::Description ^ "string to match")
                             << (bi::Type << bi::String))
                 << (bi::Arg
                     << (bi::Name ^ "number")
                     << (bi::Description ^
                         "number of matches to return; `-1` means all matches")
                     << (bi::Type << bi::Number)))
             << (bi::Result << (bi::Name ^ "output")
                            << (bi::Description ^ "array of all matches")
                            << (bi::Type
                                << (bi::DynamicArray
                                    << (bi::Type
                                        << (bi::DynamicArray
                                            << (bi::Type << bi::String))))));

  Node split(const Nodes& args)
  {
    Node pattern_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("regex.split"));
    if (pattern_node->type() == Error)
    {
      return pattern_node;
    }

    Node value_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("regex.split"));
    if (value_node->type() == Error)
    {
      return value_node;
    }

    std::string pattern = json::unescape(get_string(pattern_node));
    std::string value = get_string(value_node);

    std::regex re;
    try
    {
      re = std::regex(pattern);
    }
    catch (std::regex_error& e)
    {
      return error(pattern_node, e.code());
    }

    Node array = NodeDef::create(Array);
    std::smatch match;
    while (true)
    {
      std::regex_search(value, match, re);
      if (match.empty() || value.empty())
      {
        array->push_back(Resolver::scalar(value));
        break;
      }

      array->push_back(Resolver::scalar(match.prefix()));
      value = match.suffix();
    }

    return array;
  }

  Node split_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "pattern")
                               << (bi::Description ^ "regular expression")
                               << (bi::Type << bi::String))
                   << (bi::Arg << (bi::Name ^ "value")
                               << (bi::Description ^ "string to match")
                               << (bi::Type << bi::String)))
    << (bi::Result << (bi::Name ^ "output")
                   << (bi::Description ^
                       "the parts obtained by splitting `value`")
                   << (bi::Type
                       << (bi::DynamicArray << (bi::Type << bi::String))));

  enum class SectionType
  {
    Literal,
    Regex
  };

  struct Section
  {
    std::string pattern;
    SectionType type;
  };

  std::vector<Section> parse_template(
    const std::string& template_,
    const std::string& delim_start,
    const std::string& delim_end)
  {
    std::vector<Section> sections;
    std::size_t start = 0;
    std::size_t regex_start = template_.find(delim_start);
    while (regex_start < template_.npos)
    {
      regex_start += delim_start.size();
      std::size_t regex_end = template_.find(delim_end, regex_start);
      if (regex_end != template_.npos)
      {
        if (regex_start > start)
        {
          sections.push_back(
            {template_.substr(start, regex_start - start - delim_start.size()),
             SectionType::Literal});
        }
        sections.push_back(
          {template_.substr(regex_start, regex_end - regex_start),
           SectionType::Regex});
        start = regex_end + delim_end.size();
        regex_start = template_.find(delim_start, start);
      }
      else
      {
        break;
      }
    }

    if (start < template_.size())
    {
      sections.push_back({template_.substr(start), SectionType::Literal});
    }

    return sections;
  }

  std::string compile_template(
    const std::string& template_,
    const std::string& delim_start,
    const std::string& delim_end)
  {
    std::vector<Section> sections =
      parse_template(template_, delim_start, delim_end);
    std::ostringstream os;
    for (const auto& section : sections)
    {
      if (section.type == SectionType::Literal)
      {
        os << section.pattern;
      }
      else
      {
        os << "(?:" << section.pattern << ")";
      }
    }
    return os.str();
  }

  Node template_match(const Nodes& args)
  {
    Node template_node = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("regex.template_match"));
    if (template_node->type() == Error)
    {
      return template_node;
    }

    Node value_node = unwrap_arg(
      args, UnwrapOpt(1).type(JSONString).func("regex.template_match"));
    if (value_node->type() == Error)
    {
      return value_node;
    }

    Node delimiter_start_node = unwrap_arg(
      args, UnwrapOpt(2).type(JSONString).func("regex.template_match"));
    if (delimiter_start_node->type() == Error)
    {
      return delimiter_start_node;
    }

    Node delimiter_end_node = unwrap_arg(
      args, UnwrapOpt(3).type(JSONString).func("regex.template_match"));
    if (delimiter_end_node->type() == Error)
    {
      return delimiter_end_node;
    }

    std::string template_ = json::unescape(get_string(template_node));
    std::string value = get_string(value_node);
    std::string delimiter_start = get_string(delimiter_start_node);
    std::string delimiter_end = get_string(delimiter_end_node);

    std::string pattern =
      compile_template(template_, delimiter_start, delimiter_end);
    try
    {
      std::regex re(pattern);
      bool match = std::regex_match(value, re);
      return Resolver::scalar(match);
    }
    catch (std::regex_error& e)
    {
      return error(template_node, e.code());
    }
  }

  Node template_match_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg
            << (bi::Name ^ "template")
            << (bi::Description ^
                "template expression containing `0..n` regular expressions")
            << (bi::Type << bi::String))
        << (bi::Arg << (bi::Name ^ "value")
                    << (bi::Description ^ "string to match")
                    << (bi::Type << bi::String))
        << (bi::Arg
            << (bi::Name ^ "delimiter_start")
            << (bi::Description ^
                "start delimiter of the regular expression in `template`")
            << (bi::Type << bi::String))
        << (bi::Arg << (bi::Name ^ "delimiter_end")
                    << (bi::Description ^
                        "end delimiter of the regular expression in `template`")
                    << (bi::Type << bi::String)))
    << (bi::Result << (bi::Name ^ "result")
                   << (bi::Description ^
                       "true of `value` matches the `template`")
                   << (bi::Type << bi::Boolean));
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> regex()
    {
      return {
        BuiltInDef::create(
          Location("regex.find_all_string_submatch_n"),
          find_all_string_submatch_n_decl,
          find_all_string_submatch_n),
        BuiltInDef::create(Location("regex.find_n"), find_n_decl, find_n),
        BuiltInDef::create(Location("regex.is_valid"), is_valid_decl, is_valid),
        BuiltInDef::create(Location("regex.match"), match_decl, match),
        BuiltInDef::create(Location("regex.replace"), replace_decl, replace),
        BuiltInDef::create(Location("regex.split"), split_decl, split),
        BuiltInDef::create(
          Location("regex.template_match"),
          template_match_decl,
          template_match),
      };
    }
  }
}