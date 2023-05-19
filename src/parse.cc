#include "lang.h"

namespace rego
{
  const std::string alpha =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string numeric = "0123456789";
  const std::string alphanumeric = alpha + numeric;

  Parse parser()
  {
    Parse p(depth::file);

    p("start",
      {
        // end of file terminates
        "\r*\n$" >> [](auto& m) { m.term(); },

        // A newline sometimes terminates.
        "\r*\n([[:blank:]]*)" >>
          [](auto& m) {
            if (m.in(List))
            {
              return;
            }

            m.term();
          },

        // Whitespace between tokens.
        "[[:blank:]]+" >> [](auto&) {},

        // terminator
        ";" >> [](auto& m) { m.term(); },

        // Assign.
        ":=" >> [](auto& m) { m.add(Assign); },

        // Key/Value
        ":" >> [](auto& m) { m.add(Colon); },

        // List
        "," >> [](auto& m) { m.seq(List); },

        // Brace.
        R"((\{)[[:blank:]]*)" >> [](auto& m) { m.push(Brace, 1); },

        R"(\})" >>
          [](auto& m) {
            m.term({List});
            m.pop(Brace);
          },

        // Square.
        R"((\[)[[:blank:]]*)" >> [](auto& m) { m.push(Square, 1); },

        R"(\])" >>
          [](auto& m) {
            m.term({List});
            m.pop(Square);
          },

        // Parens.
        R"((\()[[:blank:]]*)" >> [](auto& m) { m.push(Paren, 1); },

        R"(\))" >>
          [](auto& m) {
            m.term();
            m.pop(Paren);
          },

        // Float.
        R"([[:digit:]]+\.[[:digit:]]+(?:e[+-]?[[:digit:]]+)?\b)" >>
          [](auto& m) { m.add(JSONFloat); },

        // String.
        R"("[^"]*")" >> [](auto& m) { m.add(JSONString); },

        // Int.
        R"([[:digit:]]+\b)" >> [](auto& m) { m.add(JSONInt); },

        // True.
        "true\\b" >> [](auto& m) { m.add(JSONTrue); },

        // False.
        "false\\b" >> [](auto& m) { m.add(JSONFalse); },

        // Null.
        "null\\b" >> [](auto& m) { m.add(JSONNull); },

        // Empty set.
        R"(set\(\))" >> [](auto& m) { m.add(EmptySet); },

        // Dot.
        R"(\.)" >> [](auto& m) { m.add(Dot); },

        // Line comment.
        "#[^\n]*" >> [](auto&) {},

        // Package.
        R"(package\b)" >> [](auto& m) { m.add(Package); },

        // Query.
        R"(query\b)" >> [](auto& m) { m.add(Query); },

        // Identifier.
        R"([_[:alpha:]][_[:alnum:]]*\b)" >> [](auto& m) { m.add(Var); },

        // Add ('+' is a reserved RegEx character)
        R"(\+)" >> [](auto& m) { m.add(Add); },

        // Subtract
        "-" >> [](auto& m) { m.add(Subtract); },

        // Multiply ('*' is a reserved RegEx character)
        R"(\*)" >> [](auto& m) { m.add(Multiply); },

        // Divide
        "/" >> [](auto& m) { m.add(Divide); },

        // Equals
        "==" >> [](auto& m) { m.add(Equals); },

        // NotEquals
        "!=" >> [](auto& m) { m.add(NotEquals); },

        // LessThanOrEquals
        "<=" >> [](auto& m) { m.add(LessThanOrEquals); },

        // Less than
        "<" >> [](auto& m) { m.add(LessThan); },

        // GreaterThanOrEquals
        ">=" >> [](auto& m) { m.add(GreaterThanOrEquals); },

        // GreaterThan
        ">" >> [](auto& m) { m.add(GreaterThan); },

      });

    p.gen({
      JSONInt >> [](auto& rnd) { return std::to_string(rnd() % 100); },
      JSONFloat >>
        [](auto& rnd) {
          std::uniform_real_distribution<> dist(-10.0, 10.0);
          return std::to_string(dist(rnd));
        },
      JSONTrue >> [](auto&) { return "true"; },
      JSONFalse >> [](auto&) { return "false"; },
      JSONNull >> [](auto&) { return "null"; },
    });

    return p;
  }
}