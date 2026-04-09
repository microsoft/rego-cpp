#include "builtins.hh"
#include "trieste/json.h"

#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  // gobwas/glob metacharacters that need escaping in quote_meta
  bool is_glob_meta(char c)
  {
    return c == '*' || c == '?' || c == '\\' || c == '[' || c == ']' ||
      c == '{' || c == '}';
  }

  Node quote_meta_impl(const Nodes& args)
  {
    Node pattern_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("glob.quote_meta"));
    if (pattern_node->type() == Error)
    {
      return pattern_node;
    }

    std::string pattern = json::unescape(get_string(pattern_node));
    std::string result;
    result.reserve(pattern.size() * 2);
    for (char c : pattern)
    {
      if (is_glob_meta(c))
      {
        result.push_back('\\');
      }
      result.push_back(c);
    }
    return Resolver::scalar(json::escape(result));
  }

  // --- glob.match implementation ---

  // Lookup table for O(1) delimiter checks.
  struct DelimiterTable
  {
    bool table[256] = {};

    DelimiterTable() = default;

    explicit DelimiterTable(const std::string& delimiters)
    {
      for (char c : delimiters)
      {
        table[static_cast<unsigned char>(c)] = true;
      }
    }

    bool operator()(char c) const
    {
      return table[static_cast<unsigned char>(c)];
    }
  };

  // A frame for the iterative glob matcher. Either an exact position to try,
  // or a lazy wildcard range that yields one position per pop.
  struct GlobFrame
  {
    std::string_view pattern;
    std::size_t pi;
    std::size_t ti;
    // Lazy wildcard range: when limit > 0, this frame represents
    // ti..limit (exclusive). Pop yields ti and re-pushes with ti+1
    // while ti < limit.
    std::size_t limit;
  };

  // Arena that owns concatenated alternative pattern strings. Uses deque so
  // that push_back never invalidates references (and thus string_views).
  // Tracks total byte usage and rejects additions that exceed the budget.
  struct PatternArena
  {
    static constexpr std::size_t max_bytes = 10'000'000;

    // Try to add a string to the arena. Returns a view of the stored string
    // on success, or std::nullopt if the byte budget would be exceeded.
    std::optional<std::string_view> push(std::string s)
    {
      total_bytes += s.size();
      if (total_bytes > max_bytes)
      {
        return std::nullopt;
      }
      strings.push_back(std::move(s));
      return std::string_view(strings.back());
    }

  private:
    std::deque<std::string> strings;
    std::size_t total_bytes = 0;
  };

  static constexpr std::size_t glob_max_frames = 4096;
  static constexpr int glob_max_brace_depth = 64;

  // Iterative glob matcher using an explicit stack for backtracking.
  // Returns true/false for match result, or nullopt if pattern is too complex.
  std::optional<bool> glob_match_iterative(
    std::string_view pattern,
    std::size_t pi,
    const std::string& text,
    std::size_t ti,
    const DelimiterTable& delimiters,
    bool has_delimiters,
    std::string& error_msg);

  // Parse a character class at pattern[pi] (pi points to '[').
  // Returns the index past the closing ']', or std::string::npos on error.
  // Sets `match` to whether `c` matches the character class.
  //
  // Character classes operate on single bytes (ASCII), matching the
  // behaviour of Go's filepath.Match. For Unicode-aware character set
  // matching, use regex.globs_match instead.
  std::size_t match_char_class(
    std::string_view pattern, std::size_t pi, char c, bool& match)
  {
    // pi points to '['
    pi++; // skip '['
    if (pi >= pattern.size())
    {
      return std::string::npos;
    }

    bool negated = false;
    if (pattern[pi] == '!')
    {
      negated = true;
      pi++;
    }

    bool found = false;
    bool first = true;
    while (pi < pattern.size() && (first || pattern[pi] != ']'))
    {
      first = false;
      char lo = pattern[pi];
      if (lo == '\\' && pi + 1 < pattern.size())
      {
        pi++;
        lo = pattern[pi];
      }
      pi++;
      if (
        pi + 1 < pattern.size() && pattern[pi] == '-' && pattern[pi + 1] != ']')
      {
        // range: lo-hi
        pi++; // skip '-'
        char hi = pattern[pi];
        if (hi == '\\' && pi + 1 < pattern.size())
        {
          pi++;
          hi = pattern[pi];
        }
        pi++; // skip hi
        if (hi < lo)
        {
          return std::string::npos; // invalid range
        }
        if (c >= lo && c <= hi)
        {
          found = true;
        }
      }
      else
      {
        if (c == lo)
        {
          found = true;
        }
      }
    }

    if (pi >= pattern.size())
    {
      return std::string::npos; // unclosed '['
    }

    pi++; // skip ']'
    match = negated ? !found : found;
    return pi;
  }

  // Find alternatives in a {...} group. Returns list of subpatterns and
  // the index past the closing '}'.
  std::size_t parse_alternatives(
    std::string_view pattern, std::size_t pi, std::vector<std::string>& alts)
  {
    // pi points to '{'
    pi++; // skip '{'
    int depth = 1;
    std::string current;
    while (pi < pattern.size() && depth > 0)
    {
      char c = pattern[pi];
      if (c == '\\' && pi + 1 < pattern.size())
      {
        current.push_back(c);
        current.push_back(pattern[pi + 1]);
        pi += 2;
        continue;
      }
      if (c == '{')
      {
        depth++;
        if (depth > glob_max_brace_depth)
        {
          return std::string::npos;
        }
        current.push_back(c);
        pi++;
      }
      else if (c == '}')
      {
        depth--;
        if (depth == 0)
        {
          alts.push_back(current);
          pi++; // skip '}'
        }
        else
        {
          current.push_back(c);
          pi++;
        }
      }
      else if (c == ',' && depth == 1)
      {
        alts.push_back(current);
        if (alts.size() > glob_max_frames)
        {
          return std::string::npos;
        }
        current.clear();
        pi++;
      }
      else
      {
        current.push_back(c);
        pi++;
      }
    }
    if (depth > 0)
    {
      return std::string::npos;
    }
    return pi;
  }

  // Handle backslash escape in glob pattern. Returns true if matching
  // continues, false if this path failed. Advances pi and ti.
  bool match_escape(
    std::string_view pattern,
    std::size_t& pi,
    const std::string& text,
    std::size_t& ti)
  {
    pi++; // skip backslash
    if (pi >= pattern.size())
    {
      // Trailing backslash is a malformed pattern (matches Go filepath.Match
      // ErrBadPattern behavior). Always fails.
      return false;
    }
    if (ti >= text.size() || text[ti] != pattern[pi])
    {
      return false;
    }
    pi++;
    ti++;
    return true;
  }

  // Handle wildcard (* or **) by pushing a lazy range frame onto the stack.
  // Returns nullopt if too complex, otherwise true (always succeeds by
  // deferring work to the stack).
  std::optional<bool> push_wildcard(
    std::string_view pattern,
    std::size_t& pi,
    const std::string& text,
    std::size_t ti,
    const DelimiterTable& delimiters,
    bool has_delimiters,
    std::vector<GlobFrame>& stack)
  {
    bool super = (pi + 1 < pattern.size() && pattern[pi + 1] == '*');
    pi += super ? 2 : 1;

    // Compute the furthest position * can match to.
    // Single * stops at delimiters; ** crosses them.
    std::size_t wc_limit = text.size();
    if (!super && has_delimiters)
    {
      wc_limit = ti;
      while (wc_limit < text.size() && !delimiters(text[wc_limit]))
      {
        wc_limit++;
      }
    }

    if (ti <= wc_limit)
    {
      if (stack.size() >= glob_max_frames)
      {
        return std::nullopt;
      }
      stack.push_back({pattern, pi, ti, wc_limit});
    }
    return true;
  }

  // Handle brace alternatives {a,b,...} by pushing one frame per alternative.
  // Returns nullopt if too complex, true on success, false if braces malformed.
  std::optional<bool> push_alternatives(
    std::string_view pattern,
    std::size_t pi,
    std::size_t ti,
    std::vector<GlobFrame>& stack,
    PatternArena& arena)
  {
    std::vector<std::string> alts;
    std::size_t after_brace = parse_alternatives(pattern, pi, alts);
    if (after_brace == std::string::npos)
    {
      return false;
    }
    auto rest = pattern.substr(after_brace);
    for (auto it = alts.rbegin(); it != alts.rend(); ++it)
    {
      if (stack.size() >= glob_max_frames)
      {
        return std::nullopt;
      }
      std::string s = *it;
      s.append(rest);
      auto view = arena.push(std::move(s));
      if (!view.has_value())
      {
        return std::nullopt;
      }
      stack.push_back({view.value(), 0, ti, 0});
    }
    return true;
  }

  // Handle a character class [...] token. Returns nullopt if the class is
  // malformed, true if the character matched, false otherwise. Advances pi
  // past the closing ']' and ti past the matched character on success.
  std::optional<bool> match_bracket_token(
    std::string_view pattern,
    std::size_t& pi,
    const std::string& text,
    std::size_t& ti,
    std::string& error_msg)
  {
    if (ti >= text.size())
    {
      return false;
    }
    bool char_match = false;
    std::size_t next_pi = match_char_class(pattern, pi, text[ti], char_match);
    if (next_pi == std::string::npos)
    {
      error_msg = "syntax error in pattern";
      return std::nullopt;
    }
    if (!char_match)
    {
      return false;
    }
    pi = next_pi;
    ti++;
    return true;
  }

  // Process pattern tokens until a terminal condition is reached.
  // Returns nullopt if pattern is too complex, true if all pattern tokens
  // were consumed without mismatch, false if a mismatch was found.
  // Advances pi and ti through matched literal/? tokens. Wildcards and
  // brace alternatives push continuation frames onto the stack.
  std::optional<bool> match_pattern_step(
    std::string_view pattern,
    std::size_t& pi,
    const std::string& text,
    std::size_t& ti,
    const DelimiterTable& delimiters,
    bool has_delimiters,
    std::vector<GlobFrame>& stack,
    PatternArena& arena,
    std::string& error_msg)
  {
    while (pi < pattern.size())
    {
      char pc = pattern[pi];

      if (pc == '\\')
      {
        return match_escape(pattern, pi, text, ti);
      }

      if (pc == '*')
      {
        auto r = push_wildcard(
          pattern, pi, text, ti, delimiters, has_delimiters, stack);
        if (!r.has_value())
        {
          error_msg = "glob pattern too complex";
          return std::nullopt;
        }
        return false; // continuation is on the stack
      }

      if (pc == '?')
      {
        if (ti >= text.size() || (has_delimiters && delimiters(text[ti])))
        {
          return false;
        }
        pi++;
        ti++;
        continue;
      }

      if (pc == '[')
      {
        auto r = match_bracket_token(pattern, pi, text, ti, error_msg);
        if (!r.has_value() || !r.value())
        {
          return r;
        }
        continue;
      }

      if (pc == '{')
      {
        auto r = push_alternatives(pattern, pi, ti, stack, arena);
        if (!r.has_value())
        {
          error_msg = "glob pattern too complex";
          return std::nullopt;
        }
        return false; // alternatives are on the stack (or malformed)
      }

      // Literal character
      if (ti >= text.size() || text[ti] != pc)
      {
        return false;
      }
      pi++;
      ti++;
    }
    return true;
  }

  // Iterative glob matcher using an explicit stack for backtracking.
  // Wildcard frames use lazy expansion: a single frame stores the remaining
  // range (ti through limit) and peels off one candidate at a time.
  // Returns true/false for match result, or nullopt if pattern is too complex.
  std::optional<bool> glob_match_iterative(
    std::string_view initial_pattern,
    std::size_t initial_pi,
    const std::string& text,
    std::size_t initial_ti,
    const DelimiterTable& delimiters,
    bool has_delimiters,
    std::string& error_msg)
  {
    PatternArena arena;
    std::vector<GlobFrame> stack;
    stack.reserve(256);
    stack.push_back({initial_pattern, initial_pi, initial_ti, 0});

    while (!stack.empty())
    {
      auto top = stack.back();
      stack.pop_back();

      auto pattern = top.pattern;
      auto pi = top.pi;
      auto ti = top.ti;
      auto limit = top.limit;

      // If this was a lazy range frame, re-push the remainder
      if (limit > 0 && ti < limit)
      {
        stack.push_back({pattern, pi, ti + 1, limit});
      }

      auto step_result = match_pattern_step(
        pattern,
        pi,
        text,
        ti,
        delimiters,
        has_delimiters,
        stack,
        arena,
        error_msg);
      if (!step_result.has_value())
      {
        return std::nullopt;
      }

      if (step_result.value() && ti == text.size())
      {
        return true;
      }
    }

    return false;
  }

  // Result of parsing the delimiters argument for glob.match.
  struct DelimiterSpec
  {
    std::string chars;
    bool enabled;
    Node error;
  };

  // Parse the delimiters argument node into a DelimiterSpec.
  // On success, error is Undefined. On failure, error is an Error node.
  DelimiterSpec parse_delimiters(const Node& delimiters_node)
  {
    if (delimiters_node->type() == Null)
    {
      return {"", false, NodeDef::create(Undefined)};
    }

    if (delimiters_node->size() == 0)
    {
      // Empty array → default delimiter "."
      return {".", true, NodeDef::create(Undefined)};
    }

    std::string chars;
    for (auto& child : *delimiters_node)
    {
      auto maybe_item = unwrap(child, JSONString);
      if (!maybe_item.success)
      {
        return {
          "",
          false,
          err(
            delimiters_node,
            "operand 2 must be array of string",
            EvalBuiltInError)};
      }
      std::string s = json::unescape(get_string(maybe_item.node));
      if (s.size() == 1)
      {
        chars.push_back(s[0]);
      }
      else if (s.size() > 1)
      {
        return {
          "",
          false,
          err(
            delimiters_node,
            "operand 2 must be array of single-character strings",
            EvalBuiltInError)};
      }
    }
    return {std::move(chars), true, NodeDef::create(Undefined)};
  }

  Node match_impl(const Nodes& args)
  {
    Node pattern_node =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("glob.match"));
    if (pattern_node->type() == Error)
    {
      return pattern_node;
    }

    Node delimiters_node =
      unwrap_arg(args, UnwrapOpt(1).types({Null, Array}).func("glob.match"));
    if (delimiters_node->type() == Error)
    {
      return delimiters_node;
    }

    Node match_node =
      unwrap_arg(args, UnwrapOpt(2).type(JSONString).func("glob.match"));
    if (match_node->type() == Error)
    {
      return match_node;
    }

    std::string pattern = json::unescape(get_string(pattern_node));
    std::string text = json::unescape(get_string(match_node));

    auto delim_spec = parse_delimiters(delimiters_node);
    if (delim_spec.error->type() == Error)
    {
      return delim_spec.error;
    }

    DelimiterTable delimiters(delim_spec.chars);
    std::string error_msg;
    auto result = glob_match_iterative(
      pattern, 0, text, 0, delimiters, delim_spec.enabled, error_msg);
    if (!result.has_value())
    {
      return err(pattern_node, error_msg, EvalBuiltInError);
    }
    return Resolver::scalar(result.value());
  }

  BuiltIn match_factory()
  {
    const Node match_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg << (bi::Name ^ "pattern")
                      << (bi::Description ^ "glob pattern")
                      << (bi::Type << bi::String))
          << (bi::Arg << (bi::Name ^ "delimiters")
                      << (bi::Description ^
                          "glob pattern delimiters, e.g. `[\".\", \":\"]`, "
                          "defaults to `[\".\"]` if unset. If `delimiters` is "
                          "`null`, glob match without a delimiter.")
                      << (bi::Type
                          << (bi::TypeSeq
                              << (bi::Type << bi::Null)
                              << (bi::Type
                                  << (bi::DynamicArray
                                      << (bi::Type << bi::String))))))
          << (bi::Arg << (bi::Name ^ "match")
                      << (bi::Description ^ "string to match against `pattern`")
                      << (bi::Type << bi::String)))
      << (bi::Result << (bi::Name ^ "result")
                     << (bi::Description ^
                         "true if `match` can be found in `pattern` which is "
                         "separated by `delimiters`")
                     << (bi::Type << bi::Boolean));
    return BuiltInDef::create({"glob.match"}, match_decl, match_impl);
  }

  BuiltIn quote_meta_factory()
  {
    const Node quote_meta_decl =
      bi::Decl << (bi::ArgSeq
                   << (bi::Arg << (bi::Name ^ "pattern")
                               << (bi::Description ^ "glob pattern")
                               << (bi::Type << bi::String)))
               << (bi::Result
                   << (bi::Name ^ "output")
                   << (bi::Description ^ "the escaped string of `pattern`")
                   << (bi::Type << bi::String));
    return BuiltInDef::create(
      {"glob.quote_meta"}, quote_meta_decl, quote_meta_impl);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn glob(const Location& name)
    {
      assert(name.view().starts_with("glob."));
      std::string_view view = name.view().substr(5); // skip "glob."
      if (view == "match")
      {
        return match_factory();
      }
      if (view == "quote_meta")
      {
        return quote_meta_factory();
      }

      return nullptr;
    }
  }
}