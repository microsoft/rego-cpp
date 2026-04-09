#include "builtins.hh"
#include "rego.hh"
#include "trieste/json.h"
#include "trieste/utf8.h"

#include <algorithm>
#include <optional>
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

    // Pattern is JSON-unescaped because it is a regex specification whose
    // escape sequences (e.g. \d) must be interpreted literally. Value is
    // the match target — its escapes are handled by the regex engine.
    std::string pattern = json::unescape(get_string(pattern_node));
    std::string value = get_string(value_node);

    try
    {
      std::regex re(pattern);
      bool match = std::regex_search(value, re);
      return Resolver::scalar(match);
    }
    catch (std::regex_error& e)
    {
      return error(pattern_node, e.code());
    }
  }

  BuiltIn match_factory()
  {
    const Node match_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "pattern")
                                 << (bi::Description ^ "regular expression")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "value")
                                 << (bi::Description ^
                                     "value to match against `pattern`")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^ "true of `value` matches `pattern`")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"regex.match"}, match_decl, match);
  }

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

  BuiltIn is_valid_factory()
  {
    const Node is_valid_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "pattern")
                               << (bi::Description ^ "regular expression")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "result")
                   << (bi::Description ^
                       "`true` if `pattern` is a valid regular expression")
                   << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"regex.is_valid"}, is_valid_decl, is_valid);
  }

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

  BuiltIn replace_factory()
  {
    const Node replace_decl = bi::Decl
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
    return BuiltInDef::create({"regex.replace"}, replace_decl, replace);
  }

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

    // Pattern is JSON-unescaped (regex spec); value is the literal match
    // target whose escapes are handled by the regex engine.
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
    auto it = std::sregex_iterator(value.begin(), value.end(), re);
    auto end = std::sregex_iterator();
    for (std::size_t i = 0; i < number && it != end; ++i, ++it)
    {
      array->push_back(Resolver::scalar(it->str()));
    }

    return array;
  }

  BuiltIn find_factory()
  {
    const Node find_n_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "pattern")
                      << (bi::Description ^ "regular expression")
                      << (bi::Type << bi::String))
          << (bi::Arg << (bi::Name ^ "value")
                      << (bi::Description ^ "string to match")
                      << (bi::Type << bi::String))
          << (bi::Arg << (bi::Name ^ "number")
                      << (bi::Description ^
                          "number of matches to return; `-1` means all matches")
                      << (bi::Type << bi::Number)))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^ "collected matches")
                     << (bi::Type
                         << (bi::DynamicArray << (bi::Type << bi::String))));
    return BuiltInDef::create({"regex.find_n"}, find_n_decl, find_n);
  }

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

    // Pattern is JSON-unescaped (regex spec); value is the literal match
    // target whose escapes are handled by the regex engine.
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

      // After a zero-length match, advance by one Unicode codepoint to avoid
      // re-matching at the same position (matching Go's regexp behavior).
      if (match[0].length() == 0)
      {
        auto [r, consumed] = utf8::utf8_to_rune(value, false);
        if (value.size() <= consumed.size())
        {
          break;
        }
        value = value.substr(consumed.size());
      }
      else
      {
        value = match.suffix();
      }
    }

    return array;
  }

  BuiltIn find_all_string_submatch_n_factory()
  {
    const Node find_all_string_submatch_n_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "pattern")
                      << (bi::Description ^ "regular expression")
                      << (bi::Type << bi::String))
          << (bi::Arg << (bi::Name ^ "value")
                      << (bi::Description ^ "string to match")
                      << (bi::Type << bi::String))
          << (bi::Arg << (bi::Name ^ "number")
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
    return BuiltInDef::create(
      {"regex.find_all_string_submatch_n"},
      find_all_string_submatch_n_decl,
      find_all_string_submatch_n);
  }

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

    // Pattern is JSON-unescaped (regex spec); value is the literal match
    // target whose escapes are handled by the regex engine.
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

  BuiltIn split_factory()
  {
    const Node split_decl = bi::Decl
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
    return BuiltInDef::create({"regex.split"}, split_decl, split);
  }

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

  BuiltIn template_match_factory()
  {
    const Node template_match_decl = bi::Decl
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
          << (bi::Arg
              << (bi::Name ^ "delimiter_end")
              << (bi::Description ^
                  "end delimiter of the regular expression in `template`")
              << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true of `value` matches the `template`")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create(
      {"regex.template_match"}, template_match_decl, template_match);
  }

  // --- regex.globs_match implementation ---
  // Port of yashtewari/glob-intersection algorithm.
  // Glob syntax: '.' = any char, '[...]' = char set, '+' = one or more,
  //              '*' = zero or more, '\' = escape
  //
  // Unlike glob.match (which operates on single bytes matching Go's
  // filepath.Match), this implementation uses full Unicode codepoints
  // (char32_t) for pattern tokenization and character set matching.

  // Token type ordering encodes specificity: Character is most specific
  // (matches exactly one codepoint), Dot is universal (matches any single
  // codepoint), and Set is in between (matches a defined subset).
  // gmatch() relies on this ordering to normalise its arguments so that
  // the more-specific type is always checked first. Do not reorder.
  enum class GTokenType
  {
    Character,
    Dot,
    Set
  };

  enum class GFlag
  {
    None,
    Plus,
    Star
  };

  struct GToken
  {
    GTokenType type;
    GFlag flag;
    char32_t ch; // for Character
    std::vector<char32_t> runes; // for Set (sorted, deduplicated)

    GToken() : type(GTokenType::Dot), flag(GFlag::None), ch(0) {}

    static GToken character(char32_t c)
    {
      GToken t;
      t.type = GTokenType::Character;
      t.ch = c;
      return t;
    }

    static GToken dot()
    {
      GToken t;
      t.type = GTokenType::Dot;
      return t;
    }

    static GToken set(std::vector<char32_t> rs)
    {
      GToken t;
      t.type = GTokenType::Set;
      t.runes = std::move(rs);
      return t;
    }

    bool equal(const GToken& other) const
    {
      if (type != other.type)
      {
        return false;
      }
      switch (type)
      {
        case GTokenType::Character:
          return ch == other.ch;
        case GTokenType::Dot:
          return true;
        case GTokenType::Set:
          return runes == other.runes;
      }
      return false;
    }
  };

  using GGlob = std::vector<GToken>;

  // Match two tokens ignoring flags
  bool gmatch(const GToken& t1, const GToken& t2)
  {
    // Normalize: ensure t1.type <= t2.type
    const GToken& a = (t1.type <= t2.type) ? t1 : t2;
    const GToken& b = (t1.type <= t2.type) ? t2 : t1;

    switch (a.type)
    {
      case GTokenType::Character:
        switch (b.type)
        {
          case GTokenType::Character:
            return a.ch == b.ch;
          case GTokenType::Dot:
            return true;
          case GTokenType::Set:
            return std::binary_search(b.runes.begin(), b.runes.end(), a.ch);
        }
        break;
      case GTokenType::Dot:
        // Dot matches Dot or Set
        return true;
      case GTokenType::Set: {
        // Set vs Set: check if sorted vectors share any element
        auto it1 = a.runes.begin();
        auto it2 = b.runes.begin();
        while (it1 != a.runes.end() && it2 != b.runes.end())
        {
          if (*it1 == *it2)
          {
            return true;
          }
          if (*it1 < *it2)
          {
            ++it1;
          }
          else
          {
            ++it2;
          }
        }
        return false;
      }
    }
    return false;
  }

  std::string globs_error_msg(
    const std::string& input, std::size_t rune_pos, const std::string& detail)
  {
    return "input:" + input + ", pos:" + std::to_string(rune_pos) + ", " +
      detail + ": the input provided is invalid";
  }

  // Apply a trailing + or * flag to a token, advancing the rune index.
  void gapply_flag(GToken& tok, const std::u32string& runes, std::size_t& i)
  {
    if (i < runes.size() && (runes[i] == '+' || runes[i] == '*'))
    {
      tok.flag = (runes[i] == '+') ? GFlag::Plus : GFlag::Star;
      i++;
    }
  }

  // Maximum total character set runes across all sets in a single pattern.
  // Individual ranges are capped at 65536 characters (2^16) to prevent
  // allocating unreasonable memory for patterns like [a-\U0010FFFF].
  // The aggregate cap across all sets is 2^20 runes (~4 MB of char32_t).
  static constexpr std::size_t max_total_set_runes = 1048576;

  // Parse a character range (e.g., a-z) inside a set. The prev character
  // has already been recorded. i points to the '-'. On success, appends
  // the range to set_runes and advances i past the range end.
  bool gtokenize_set_range(
    const std::string& input,
    const std::u32string& runes,
    std::size_t& i,
    char32_t prev,
    std::vector<char32_t>& set_runes,
    std::size_t& total_set_runes,
    std::string& error_msg)
  {
    if (i >= runes.size() - 1)
    {
      error_msg = globs_error_msg(
        input,
        i + 1,
        "range character '-' must be followed by a Unicode character");
      return false;
    }
    i++;
    char32_t range_end = runes[i];
    bool range_escaped = false;
    if (range_end == '\\')
    {
      if (i < runes.size() - 1)
      {
        i++;
        range_end = runes[i];
        range_escaped = true;
      }
    }
    if (!range_escaped && (range_end == ']' || range_end == '-'))
    {
      error_msg = globs_error_msg(
        input,
        i + 1,
        "range character '-' cannot be followed by a special symbol");
      return false;
    }
    if (range_end < prev)
    {
      std::ostringstream os;
      os << "range is out of order: '" << utf8::rune(range_end)
         << "' comes before '" << utf8::rune(prev) << "' in Unicode";
      error_msg = globs_error_msg(input, i + 1, os.str());
      return false;
    }
    if (range_end - prev > 65536)
    {
      error_msg = globs_error_msg(input, i + 1, "character range too large");
      return false;
    }
    std::size_t range_size = static_cast<std::size_t>(range_end - prev) + 1;
    if (total_set_runes + range_size > max_total_set_runes)
    {
      error_msg = globs_error_msg(input, i + 1, "character range too large");
      return false;
    }
    set_runes.reserve(set_runes.size() + range_size);
    for (char32_t x = prev; x <= range_end; x++)
    {
      set_runes.push_back(x);
    }
    total_set_runes += range_size;
    i++;
    return true;
  }

  // Parse a character set [...] starting after the '['. Populates set_runes
  // and advances i past the closing ']'. Returns true on success.
  bool gtokenize_set(
    const std::string& input,
    const std::u32string& runes,
    std::size_t& i,
    std::vector<char32_t>& set_runes,
    std::size_t& total_set_runes,
    std::string& error_msg)
  {
    bool prev_exists = false;
    char32_t prev = 0;

    while (i < runes.size())
    {
      char32_t r = runes[i];

      bool escaped = false;
      if (r == '\\')
      {
        if (i < runes.size() - 1)
        {
          i++;
          r = runes[i];
          escaped = true;
        }
        else
        {
          error_msg = globs_error_msg(
            input, i, "input ends with a \\\\ (escape) character");
          return false;
        }
      }

      if (!escaped)
      {
        if (r == '-')
        {
          if (!prev_exists)
          {
            error_msg = globs_error_msg(
              input,
              i + 1,
              "range character '-' must be preceded by a Unicode character");
            return false;
          }
          if (!gtokenize_set_range(
                input, runes, i, prev, set_runes, total_set_runes, error_msg))
          {
            return false;
          }
          prev_exists = false;
          continue;
        }

        if (r == ']')
        {
          i++; // skip ']'
          return true;
        }
      }

      set_runes.push_back(r);
      total_set_runes++;
      prev = r;
      prev_exists = true;
      i++;
    }

    error_msg = globs_error_msg(input, i, "found [ without matching ]");
    return false;
  }

  // Tokenize a glob pattern (yashtewari syntax)
  // Returns empty vector and sets error_msg on failure
  std::vector<GToken> gtokenize(
    const std::string& input, std::string& error_msg)
  {
    auto runes = utf8::utf8_to_runestring(input);
    std::vector<GToken> tokens;
    tokens.reserve(runes.size());
    std::size_t i = 0;
    std::size_t total_set_runes = 0;

    while (i < runes.size())
    {
      char32_t r = runes[i];

      if (r == '\\')
      {
        if (i == runes.size() - 1)
        {
          error_msg = globs_error_msg(
            input, i, "input ends with a \\\\ (escape) character");
          return {};
        }
        i++;
        GToken tok = GToken::character(runes[i]);
        i++;
        gapply_flag(tok, runes, i);
        tokens.push_back(tok);
        continue;
      }

      if (r == '.')
      {
        GToken tok = GToken::dot();
        i++;
        gapply_flag(tok, runes, i);
        tokens.push_back(tok);
        continue;
      }

      if (r == ']')
      {
        error_msg =
          globs_error_msg(input, i + 1, "set-close ']' with no preceding '['");
        return {};
      }

      if (r == '[')
      {
        i++; // skip '['
        std::vector<char32_t> set_runes;
        if (!gtokenize_set(
              input, runes, i, set_runes, total_set_runes, error_msg))
        {
          return {};
        }
        std::sort(set_runes.begin(), set_runes.end());
        set_runes.erase(
          std::unique(set_runes.begin(), set_runes.end()), set_runes.end());
        GToken tok = GToken::set(std::move(set_runes));
        gapply_flag(tok, runes, i);
        tokens.push_back(std::move(tok));
        continue;
      }

      if (r == '+' || r == '*')
      {
        std::string flag_str(1, static_cast<char>(r));
        error_msg = globs_error_msg(
          input,
          i + 1,
          "flag '" + flag_str + "' must be preceded by a non-flag");
        return {};
      }

      // Regular character
      GToken tok = GToken::character(r);
      i++;
      gapply_flag(tok, runes, i);
      tokens.push_back(tok);
    }

    return tokens;
  }

  // Simplify: merge adjacent flagged tokens of same kind
  GGlob gsimplify(GGlob tokens)
  {
    if (tokens.empty())
    {
      return tokens;
    }

    // Check if any merging is needed before allocating a new vector.
    bool needs_merge = false;
    for (std::size_t i = 1; i < tokens.size(); i++)
    {
      if (
        tokens[i].flag != GFlag::None && tokens[i - 1].flag != GFlag::None &&
        tokens[i].equal(tokens[i - 1]))
      {
        needs_merge = true;
        break;
      }
    }
    if (!needs_merge)
    {
      return tokens;
    }

    GGlob result;
    result.reserve(tokens.size());
    result.push_back(std::move(tokens[0]));

    for (std::size_t i = 1; i < tokens.size(); i++)
    {
      bool handled = false;
      if (tokens[i].flag != GFlag::None && result.back().flag != GFlag::None)
      {
        if (tokens[i].equal(result.back()))
        {
          // Plus takes precedence over Star
          GFlag flag = (tokens[i].flag == GFlag::Plus ||
                        result.back().flag == GFlag::Plus) ?
            GFlag::Plus :
            GFlag::Star;
          result.back().flag = flag;
          handled = true;
        }
      }
      if (!handled)
      {
        result.push_back(std::move(tokens[i]));
      }
    }
    return result;
  }

  // Iterative glob intersection test using an explicit stack.
  // Each stack frame represents a gintersect_normal(g1, i, g2, j) call.
  // When a flagged token is encountered, the star/plus expansion logic
  // computes all candidate continuation points and pushes them onto the
  // stack instead of recursing.
  // A GSpan is a non-owning view into a GGlob (pointer + offset range).
  struct GSpan
  {
    const GGlob* glob;
    std::size_t begin;
    std::size_t end;

    std::size_t size() const
    {
      return end - begin;
    }
    const GToken& operator[](std::size_t idx) const
    {
      return (*glob)[begin + idx];
    }
  };

  struct GIFrame
  {
    GSpan g1;
    std::size_t i;
    GSpan g2;
    std::size_t j;
  };

  static constexpr std::size_t gi_max_frames = 4096;

  // Advance through unflagged matching tokens in both spans.
  // Returns true if all unflagged tokens matched so far (may have stopped
  // at a flagged token or end-of-span), false if a mismatch was found.
  bool gintersect_advance(
    const GSpan& g1, std::size_t& i, const GSpan& g2, std::size_t& j)
  {
    while (i < g1.size() && j < g2.size())
    {
      if (g1[i].flag != GFlag::None || g2[j].flag != GFlag::None)
      {
        return true; // stopped at flagged token, not a failure
      }
      if (!gmatch(g1[i], g2[j]))
      {
        return false;
      }
      i++;
      j++;
    }
    return true;
  }

  // Expand a star-flagged (or plus-after-first-match) token against the
  // other span, pushing candidate continuation frames onto the stack.
  // Returns nullopt if too complex, true if star consumed everything with
  // no remaining pattern (immediate success), false otherwise.
  std::optional<bool> gintersect_expand_star(
    const GSpan& flagged_span,
    std::size_t fi,
    const GSpan& other_span,
    std::size_t oi,
    std::vector<GIFrame>& stack)
  {
    const GToken& flag_token = flagged_span[fi];
    bool has_next = (fi + 1 < flagged_span.size());

    std::vector<std::size_t> candidates;
    bool loop_completed = true;

    for (std::size_t k = oi; k < other_span.size(); k++)
    {
      if (has_next && gmatch(other_span[k], flagged_span[fi + 1]))
      {
        candidates.push_back(k);
      }
      if (!gmatch(other_span[k], flag_token))
      {
        loop_completed = false;
        break;
      }
    }

    if (loop_completed && !has_next)
    {
      return true;
    }

    for (auto it = candidates.rbegin(); it != candidates.rend(); ++it)
    {
      if (stack.size() >= gi_max_frames)
      {
        return std::nullopt;
      }
      stack.push_back({flagged_span, fi + 1, other_span, *it});
    }
    return false;
  }

  // Iterative glob intersection test using an explicit stack.
  std::optional<bool> gintersect(GSpan s1, GSpan s2)
  {
    std::vector<GIFrame> stack;
    stack.reserve(256);
    stack.push_back({s1, 0, s2, 0});

    while (!stack.empty())
    {
      auto [g1, i, g2, j] = stack.back();
      stack.pop_back();

      if (!gintersect_advance(g1, i, g2, j))
      {
        continue;
      }

      if (i == g1.size() && j == g2.size())
      {
        return true;
      }

      if (i >= g1.size() || j >= g2.size())
      {
        continue;
      }

      // Orient so flagged_span has the flagged token
      GSpan flagged_span = (g1[i].flag != GFlag::None) ? g1 : g2;
      std::size_t fi = (g1[i].flag != GFlag::None) ? i : j;
      GSpan other_span = (g1[i].flag != GFlag::None) ? g2 : g1;
      std::size_t oi = (g1[i].flag != GFlag::None) ? j : i;

      const GToken& flag_token = flagged_span[fi];

      if (flag_token.flag == GFlag::Plus)
      {
        if (oi >= other_span.size() || !gmatch(flag_token, other_span[oi]))
        {
          continue;
        }
        if (other_span[oi].flag != GFlag::None)
        {
          if (stack.size() >= gi_max_frames)
          {
            return std::nullopt;
          }
          stack.push_back({flagged_span, fi + 1, other_span, oi});
        }
        oi++;
      }

      auto star_result =
        gintersect_expand_star(flagged_span, fi, other_span, oi, stack);
      if (!star_result.has_value())
      {
        return std::nullopt;
      }
      if (star_result.value())
      {
        return true;
      }
    }

    return false;
  }

  // Trim matching unflagged prefix and suffix tokens from two globs.
  // Returns false if a mismatch is found (meaning the globs definitely
  // don't intersect). On success, sets l, r1, r2 so that [l, r1) and
  // [l, r2) are the untrimmed core spans to pass to gintersect.
  //
  // The l-- after the forward scan keeps one "buffer" element from the
  // matched prefix as an anchor for the intersection algorithm.
  // Similarly, the r1++/r2++ after the backward scan preserves one
  // element from the matched suffix. This ensures the core span always
  // includes at least one token from each boundary.
  bool trim_matching_affixes(
    const GGlob& g1,
    const GGlob& g2,
    std::size_t& l,
    std::size_t& r1,
    std::size_t& r2)
  {
    l = 0;
    while (l < g1.size() && l < g2.size() && g1[l].flag == GFlag::None &&
           g2[l].flag == GFlag::None)
    {
      if (!gmatch(g1[l], g2[l]))
      {
        return false;
      }
      l++;
    }
    if (l > 0)
    {
      l--;
    }

    r1 = g1.size();
    r2 = g2.size();
    while (r1 > l + 1 && r2 > l + 1 && g1[r1 - 1].flag == GFlag::None &&
           g2[r2 - 1].flag == GFlag::None)
    {
      if (!gmatch(g1[r1 - 1], g2[r2 - 1]))
      {
        return false;
      }
      r1--;
      r2--;
    }
    if (r1 < g1.size())
    {
      r1++;
      r2++;
    }

    assert(l <= r1 && l <= r2);
    return true;
  }

  Node globs_match(const Nodes& args)
  {
    Node glob1_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("regex.globs_match"));
    if (glob1_node->type() == Error)
    {
      return glob1_node;
    }

    Node glob2_node =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("regex.globs_match"));
    if (glob2_node->type() == Error)
    {
      return glob2_node;
    }

    std::string glob1 = json::unescape(get_string(glob1_node));
    std::string glob2 = json::unescape(get_string(glob2_node));

    std::string error_msg;
    auto tokens1 = gtokenize(glob1, error_msg);
    if (!error_msg.empty())
    {
      return err(glob1_node, error_msg, EvalBuiltInError);
    }

    error_msg.clear();
    auto tokens2 = gtokenize(glob2, error_msg);
    if (!error_msg.empty())
    {
      return err(glob2_node, error_msg, EvalBuiltInError);
    }

    GGlob g1 = gsimplify(std::move(tokens1));
    GGlob g2 = gsimplify(std::move(tokens2));

    std::size_t l, r1, r2;
    if (!trim_matching_affixes(g1, g2, l, r1, r2))
    {
      return Resolver::scalar(false);
    }

    GSpan s1{&g1, l, r1};
    GSpan s2{&g2, l, r2};

    auto result = gintersect(s1, s2);
    if (!result.has_value())
    {
      return err(glob1_node, "glob pattern too complex", EvalBuiltInError);
    }
    return Resolver::scalar(result.value());
  }

  BuiltIn globs_match_factory()
  {
    const Node globs_match_decl = bi::Decl
      << (bi::ArgSeq << (bi::Arg << (bi::Name ^ "glob1")
                                 << (bi::Description ^
                                     "first glob-style regular expression")
                                 << (bi::Type << bi::String))
                     << (bi::Arg << (bi::Name ^ "glob2")
                                 << (bi::Description ^
                                     "second glob-style regular expression")
                                 << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "`true` if the intersection of `glob1` and `glob2` "
                         "matches a non-empty set of non-empty strings.")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create(
      {"regex.globs_match"}, globs_match_decl, globs_match);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn regex(const Location& name)
    {
      assert(name.view().starts_with("regex."));
      std::string_view view = name.view().substr(6); // skip "regex."
      if (view == "find_all_string_submatch_n")
      {
        return find_all_string_submatch_n_factory();
      }
      if (view == "find_n")
      {
        return find_factory();
      }
      if (view == "is_valid")
      {
        return is_valid_factory();
      }
      if (view == "match")
      {
        return match_factory();
      }
      if (view == "replace")
      {
        return replace_factory();
      }
      if (view == "split")
      {
        return split_factory();
      }
      if (view == "template_match")
      {
        return template_match_factory();
      }
      if (view == "globs_match")
      {
        return globs_match_factory();
      }

      return nullptr;
    }
  }
}