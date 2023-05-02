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
        "\r*\n$" >>
          [](auto& m) {
            m.term({Assign});
            m.pop(RuleSeq);
          },

        // A newline sometimes terminates.
        "\r*\n([[:blank:]]*)" >>
          [](auto& m) {
            if (m.in(List))
            {
              return;
            }

            m.term({Assign});
            if (m.in(Package))
            {
              m.term();
              m.pop(Package);
              m.push(RuleSeq);
            }
          },

        // Whitespace between tokens.
        "[[:blank:]]+" >> [](auto&) {},

        // terminator
        ";" >> [](auto& m) { m.term({Assign}); },

        // Assign.
        ":=" >> [](auto& m) { m.seq(Assign); },

        // Key/Value
        ":" >> [](auto& m) { m.seq(KeyValue); },

        // List
        "," >> [](auto& m) { m.term({KeyValue}); },

        // Brace.
        R"((\{)[[:blank:]]*)" >>
          [](auto& m) {
            m.term();
            m.push(Brace, 1);
          },

        R"(\})" >>
          [](auto& m) {
            m.term({KeyValue});
            m.pop(Brace);
          },

        // Square.
        R"((\[)[[:blank:]]*)" >> [](auto& m) { m.push(Square, 1); },

        R"(\])" >>
          [](auto& m) {
            m.term();
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
          [](auto& m) { m.add(Float); },

        // String.
        R"("[^"]*")" >> [](auto& m) { m.add(String); },

        // Int.
        R"([[:digit:]]+\b)" >> [](auto& m) { m.add(Int); },

        // Bool.
        "(?:true|false)\\b" >> [](auto& m) { m.add(Bool); },

        // Null.
        "null\\b" >> [](auto& m) { m.add(Null); },

        // Dot.
        R"(\.)" >> [](auto& m) { m.add(Dot); },

        // Line comment.
        "#[^\n]*" >> [](auto&) {},

        // Print.
        R"(print\b)" >> [](auto& m) { m.add(Print); },

        // Package.
        R"(package\b)" >>
          [](auto& m) {
            m.term();
            m.push(Package);
          },

        // Query.
        R"(query\b)" >> [](auto& m) { m.add(Query); },

        // Identifier.
        R"([_[:alpha:]][_[:alnum:]]*\b)" >> [](auto& m) { m.add(Ident); },

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
      Int >> [](auto& rnd) { return std::to_string(rnd() % 100); },
      Float >>
        [](auto& rnd) {
          std::uniform_real_distribution<> dist(-10.0, 10.0);
          return std::to_string(dist(rnd));
        },
      Bool >> [](auto& rnd) { return rnd() % 2 ? "true" : "false"; },
      Null >> [](auto&) { return "null"; },
    });

    return p;
  }
}