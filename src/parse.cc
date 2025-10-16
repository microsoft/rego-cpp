#include "internal.hh"
#include "rego.hh"

namespace
{
  const std::string alpha =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string numeric = "0123456789";
  const std::string alphanumeric = alpha + numeric;
  const std::string whitespace = " \t";
  const std::string quoted = alphanumeric + whitespace + "\n";

  template <typename T>
  std::string rand_int(T& rnd, int min = -50, int max = 50)
  {
    int range = max - min;
    std::ostringstream buf;
    buf << rnd() % range + min;
    return buf.str();
  }

  template <typename T>
  std::string rand_float(T& rnd)
  {
    std::ostringstream buf;
    std::uniform_real_distribution<> dist(-10.0, 10.0);
    buf << dist(rnd);
    return buf.str();
  }

  template <typename T>
  std::string rand_string(
    T& rnd, std::size_t min_length = 0, std::size_t max_length = 10)
  {
    std::ostringstream buf;
    std::size_t length = rnd() % (max_length - min_length) + min_length;
    for (std::size_t i = 0; i < length; ++i)
    {
      buf << alphanumeric[rnd() % alphanumeric.size()];
    }
    return buf.str();
  }

  template <typename T>
  std::string rand_quoted(
    T& rnd, char quote, std::size_t min_length = 0, std::size_t max_length = 10)
  {
    std::ostringstream buf;
    std::size_t length = rnd() % (max_length - min_length) + min_length;
    buf << quote;
    for (std::size_t i = 0; i < length; ++i)
    {
      buf << quoted[rnd() % quoted.size()];
    }
    buf << quote;
    return buf.str();
  }

  enum class NewlineMode
  {
    Terminate,
    Ignore,
    Token,
  };
}

namespace rego
{
  Parse parser()
  {
    Parse p(depth::file, wf_parser);

    std::shared_ptr<NewlineMode> newline_mode =
      std::make_shared<NewlineMode>(NewlineMode::Ignore);

    // Our starting path tries to determine if the input is a module or a query
    p("start",
      {
        R"(\s+)" >>
          [](auto&) {
            // ignore leading whitespace
          },

        R"(#[^\r\n]*\r?\n)" >>
          [](auto&) {
            // ignore leading comments
          },

        R"(package)" >>
          [newline_mode](auto& m) {
            m.push(Module);
            m.add(Package);
            m.mode("main");
            *newline_mode = NewlineMode::Terminate;
          },

        "^" >>
          [](auto& m) {
            m.push(Query);
            m.mode("main");
          },
      });

    p("main",
      {
        R"(import\b)" >>
          [newline_mode](auto& m) {
            m.add(Import);
            *newline_mode = NewlineMode::Terminate;
          },

        R"(default\b)" >> [](auto& m) { m.add(Default); },

        R"(not\b)" >> [](auto& m) { m.add(Not); },

        R"(with\b)" >> [](auto& m) { m.add(With); },

        R"(else\b)" >> [](auto& m) { m.add(Else); },

        R"(some\b)" >> [](auto& m) { m.add(Some); },

        R"(true\b)" >> [](auto& m) { m.add(True); },

        R"(false\b)" >> [](auto& m) { m.add(False); },

        R"(null\b)" >> [](auto& m) { m.add(Null); },

        R"(as\b)" >> [](auto& m) { m.add(As); },

        R"(if\b)" >> [](auto& m) { m.add(If); },

        R"(in\b)" >> [](auto& m) { m.add(IsIn); },

        R"(contains\b)" >> [](auto& m) { m.add(Contains); },

        R"(every\b)" >> [](auto& m) { m.add(Every); },

        R"(set\(\))" >> [](auto& m) { m.add(EmptySet); },

        "==" >> [](auto& m) { m.add(Equals); },

        "!=" >> [](auto& m) { m.add(NotEquals); },

        "<=" >> [](auto& m) { m.add(LessThanOrEquals); },

        "<" >> [](auto& m) { m.add(LessThan); },

        ">=" >> [](auto& m) { m.add(GreaterThanOrEquals); },

        ">" >> [](auto& m) { m.add(GreaterThan); },

        R"(\+)" >> [](auto& m) { m.add(Add); },

        R"(\*)" >> [](auto& m) { m.add(Multiply); },

        "/" >> [](auto& m) { m.add(Divide); },

        "%" >> [](auto& m) { m.add(Modulo); },

        "&" >> [](auto& m) { m.add(And); },

        R"(\|)" >> [](auto& m) { m.add(Or); },

        ":=" >> [](auto& m) { m.add(Assign); },

        "=" >> [](auto& m) { m.add(Unify); },

        "," >> [](auto& m) { m.add(Comma); },

        ":" >> [](auto& m) { m.add(Colon); },

        R"(\.)" >> [](auto& m) { m.add(Dot); },

        R"(\[)" >> [](auto& m) { m.push(Square); },

        "]" >>
          [](auto& m) {
            m.term();
            m.pop(Square);
          },

        R"({(?:\r?\n)?)" >> [](auto& m) { m.push(Brace); },

        "}" >>
          [](auto& m) {
            m.term();
            m.pop(Brace);
          },

        R"(\()" >> [](auto& m) { m.push(Paren); },

        R"(\))" >>
          [](auto& m) {
            m.term();
            m.pop(Paren);
          },

        // RE for a JSON number (float):
        // -? : optional minus sign
        // (?:0|[1-9][0-9]*) : either a single 0, or 1-9 followed by any digits
        // (?:\.[0-9]+)? : a single period followed by one or more digits
        // (?:[eE][-+]?[0-9]+)? : optionally, an exponent. This can start with
        // e or E, have +/-/nothing, and then 1 or more digits
        R"(-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)(?:[eE][-+]?[0-9]+)?)" >>
          [](auto& m) { m.add(Float); },

        // RE for an integer with an exponent (float):
        // -? : optional minus sign
        // (?:0|[1-9][0-9]*) : either a single 0, or 1-9 followed by any digits
        // (?:[eE][-+]?[0-9]+)? : an exponent. This can start with
        // e or E, have +/-/nothing, and then 1 or more digits
        R"(-?(?:0|[1-9][0-9]*)[eE][-+]?[0-9]+)" >>
          [](auto& m) { m.add(Float); },

        // RE for a JSON number (int):
        // -? : optional minus sign
        // (?:0|[1-9][0-9]*) : either a single 0, or 1-9 followed by any digits
        R"(-?(?:0|[1-9][0-9]*))" >> [](auto& m) { m.add(Int); },

        "-" >> [](auto& m) { m.add(Subtract); },

        // RE for a JSON string:
        // " : a double quote followed by either:
        // 1. [^"\\\x00-\x1F]+ : one or more characters that are not a double
        // quote, backslash, or a control character from 00-1f
        // 2. \\["\\\/bfnrt] : a backslash followed by one of the characters ",
        // \, /, b, f, n, r, or t
        // 3. \\u[[:xdigit:]]{4} : a backslash followed by u, followed by 4 hex
        // digits zero or more times and then " : a double quote
        R"("(?:[^"\\\x00-\x1F]+|\\["\\\/bfnrt]|\\u[[:xdigit:]]{4})*")" >>
          [](auto& m) { m.add(JSONString); },

        R"(\`[^\`]*\`)" >> [](auto& m) { m.add(RawString); },

        R"((?:[[:alpha:]]|_)(?:[[:alnum:]]|_)*\b)" >>
          [](auto& m) { m.add(Var); },

        ";" >> [](auto& m) { m.add(NewLine); },

        R"([ \t]*\r?\n)" >>
          [newline_mode](auto& m) {
            switch (*newline_mode)
            {
              case NewlineMode::Terminate:
                m.term();
                break;
              case NewlineMode::Ignore:
                break;
              case NewlineMode::Token:
                m.add(NewLine);
                break;
            }
            *newline_mode = NewlineMode::Ignore;
          },

        R"(#[^\r\n]*\r?\n)" >> [](auto&) {},

        R"(\s+)" >> [](auto&) {},
      });

    p.done([](auto& m) { m.term({Module, Query}); });

    p.gen({
      Int >> [](auto& rnd) { return rand_int(rnd); },
      Int32 >> [](auto& rnd) { return rand_int(rnd); },
      Int64 >> [](auto& rnd) { return rand_int(rnd); },
      LocalIndex >> [](auto& rnd) { return rand_int(rnd, 0, 100); },
      StringIndex >> [](auto& rnd) { return rand_int(rnd, 0, 40); },
      Float >> [](auto& rnd) { return rand_float(rnd); },
      True >> [](auto&) { return "true"; },
      False >> [](auto&) { return "false"; },
      Null >> [](auto&) { return "null"; },
      Boolean >> [](auto& rnd) { return rnd() % 2 ? "true" : "false"; },
      builtins::Name >> [](auto& rnd) { return rand_string(rnd); },
      builtins::Description >> [](auto& rnd) { return rand_string(rnd); },
      IRString >> [](auto& rnd) { return rand_string(rnd); },
      JSONString >> [](auto& rnd) { return rand_quoted(rnd, '"'); },
      RawString >> [](auto& rnd) { return rand_quoted(rnd, '`'); },
      Version >>
        [](auto&) {
          return DefaultVersion;
        }, // TODO ensure this returns all supported versions in the future
    });

    return p;
  }
}