#include "rego_test.h"

namespace rego_test
{
  const std::string alpha =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string numeric = "0123456789";
  const std::string alphanumeric = alpha + numeric;
  const std::size_t max_string_length = 10;

  template <typename T>
  std::string rand_string(T& rnd)
  {
    // TODO add valid non-alphanum characters
    std::ostringstream buf;
    std::size_t length = rnd() % max_string_length;
    for (std::size_t i = 0; i < length; ++i)
    {
      buf << alphanumeric[rnd() % alphanumeric.size()];
    }
    return buf.str();
  }

  struct Block
  {
    std::size_t indent;
  };

  enum class Quote
  {
    None,
    Single,
    Double
  };

  enum class Collection
  {
    None,
    Brace,
    Square
  };

  Parse parser()
  {
    Parse p(depth::file);
    auto indents = std::make_shared<std::vector<std::size_t>>();
    indents->push_back(0);
    auto indent = std::make_shared<std::size_t>(0);
    auto string_indent = std::make_shared<std::size_t>(0);
    auto quote = std::make_shared<Quote>(Quote::None);
    auto stack = std::make_shared<std::vector<Collection>>();
    stack->push_back(Collection::None);

    p("start",
      {
        "---" >>
          [](auto&) {
            // TODO: add support for YAML front matter
          },

        // Line comment.
        "#[^\n]*" >> [](auto&) {},

        // end of file terminates
        "\r*\n$" >>
          [indents](auto& m) {
            while (!indents->empty())
            {
              m.term({Block});
              indents->pop_back();
            }
          },

        "\r*\n" >>
          [indent](auto& m) {
            m.term();
            *indent = 0;
            m.mode("indent");
          },

        // Brace.
        R"((\{)[[:blank:]]*)" >>
          [stack](auto& m) {
            m.push(Brace, 1);
            stack->push_back(Collection::Brace);
          },

        R"(\})" >>
          [stack](auto& m) {
            if (stack->back() == Collection::Brace)
            {
              m.term();
              m.pop(Brace);
              stack->pop_back();
            }
            else
            {
              m.extend(String);
            }
          },

        // Square.
        R"((\[)[[:blank:]]*)" >>
          [stack](auto& m) {
            m.push(Square, 1);
            stack->push_back(Collection::Square);
          },

        R"(\])" >>
          [stack](auto& m) {
            if (stack->back() == Collection::Square)
            {
              m.term();
              m.pop(Square);
              stack->pop_back();
            }
            else
            {
              m.extend(String);
            }
          },

        // Comma.
        ",[[:blank:]]*" >>
          [stack](auto& m) {
            if (stack->back() == Collection::None)
            {
              m.extend(String);
            }
            else
            {
              m.term();
            }
          },

        // Double quote string
        "\"" >>
          [quote](auto& m) {
            if (m.previous(Colon) || m.in(Block) || m.in(Square) || m.in(Brace))
            {
              m.push(DoubleQuoteString, 1);
              m.mode("multiline-string");
              *quote = Quote::Double;
            }
            else
            {
              m.extend(String);
            }
          },

        // Single quote string
        "'" >>
          [quote](auto& m) {
            if (m.previous(Colon) || m.in(Square) || m.in(Brace) || m.in(Block))
            {
              m.push(SingleQuoteString, 1);
              m.mode("multiline-string");
              *quote = Quote::Single;
            }
            else
            {
              m.extend(String);
            }
          },

        // KeyValue.
        ":[ ]*" >> [](auto& m) { m.add(Colon); },

        // Literal string
        "\\|" >> [](auto& m) { m.push(LiteralString); },

        // Folded string
        ">" >> [](auto& m) { m.push(FoldedString); },

        // Entry.
        "-[ \n][\r\n]*" >>
          [indent, indents](auto& m) {
            *indent += 2;
            m.add(Hyphen);
            m.push(Block);
            indents->push_back(*indent);
          },

        // True.
        "true\\b" >> [](auto& m) { m.add(True); },

        // False.
        "false\\b" >> [](auto& m) { m.add(False); },

        // Null.
        "null\\b" >> [](auto& m) { m.add(Null); },

        // Not a number
        R"((?:(?:[[:digit:]]+)\.\b){2})" >>
          [](auto& m) { m.add(String); m.mode("string"); },

        // String
        R"([[:digit:]\.]*[^[:digit:]^\.,\-\[\]\{\}[:blank:]\r\n])" >>
          [](auto& m) {
            m.extend(String);
            m.mode("string");
          },

        // Float.
        R"(\-?[[:digit:]]+\.[[:digit:]]+(?:e[+-]?[[:digit:]]+)?\b)" >>
          [](auto& m) { m.add(Float); },

        // Int.
        R"(\-?[[:digit:]]+\b)" >> [](auto& m) { m.add(Integer); },

        // Space.
        " " >> [](auto&) {},
      });

    p("indent",
      {R"(\\ )" >> [](auto&) {},

       // end of file terminates
       "\r*\n$" >>
         [indents](auto& m) {
           while (!indents->empty())
           {
             m.term({Block});
             indents->pop_back();
           }
         },

       "\r*\n" >>
         [indent, string_indent](auto& m) {
           if (*indent == *string_indent)
           {
             m.add(Blank);
             m.term();
           }
           *indent = 0;
         },

       // Space
       " " >>
         [indent, string_indent](auto& m) {
           if (*string_indent > 0 && *indent == *string_indent)
           {
             m.mode("multiline-string");
             m.add(String);
           }
           else
           {
             *indent = *indent + 1;
           }
         },

       // Hyphen
       "-[ \n][\r\n]*" >>
         [indent, string_indent, indents](auto& m) {
           m.mode("start");
           if (m.in(LiteralString) || m.in(FoldedString))
           {
             *string_indent = 0;
             if (m.in(LiteralString))
             {
               m.pop(LiteralString);
             }
             else
             {
               m.pop(FoldedString);
             }
           }

           while (*indent < indents->back())
           {
             m.term({Block});
             indents->pop_back();
           }

           *indent += 2;
           m.add(Hyphen);
           m.push(Block);
           indents->push_back(*indent);
         },

       // Double quote string
       "\"" >>
         [quote, indent, indents](auto& m) {
           while (*indent < indents->back())
           {
             m.term({Block});
             indents->pop_back();
           }
           m.term();
           m.push(DoubleQuoteString, 1);
           m.mode("multiline-string");
           *quote = Quote::Double;
         },

       // Single quote string
       "'" >>
         [quote, indent, indents](auto& m) {
           while (*indent < indents->back())
           {
             m.term({Block});
             indents->pop_back();
           }
           m.term();
           m.push(SingleQuoteString, 1);
           m.mode("multiline-string");
           *quote = Quote::Single;
         },

       "#" >>
         [indent, string_indent, indents](auto& m) {
           std::string mode = "comment";
           if (m.in(LiteralString) || m.in(FoldedString))
           {
             mode = "multiline-string";
             if (*string_indent == 0)
             {
               *string_indent = *indent;
               m.mode("multiline-string");
               m.add(String);
               return;
             }
             else if (*indent < *string_indent)
             {
               mode = "comment";
               *string_indent = 0;
               if (m.in(LiteralString))
               {
                 m.pop(LiteralString);
               }
               else if (m.in(FoldedString))
               {
                 m.pop(FoldedString);
               }
             }
             else
             {
               m.mode("multiline-string");
               m.add(String);
               return;
             }
           }
           else if (m.in(SingleQuoteString) || m.in(DoubleQuoteString))
           {
             if (*string_indent == 0)
             {
               *string_indent = *indent;
             }
             m.mode("multiline-string");
             m.add(String);
             return;
           }
           else
           {
             if (*indent > indents->back())
             {
               m.push(Block);
               indents->push_back(*indent);
             }
           }

           while (*indent < indents->back())
           {
             m.term({Block});
             indents->pop_back();
           }
           m.term();
           m.mode(mode);
         },

       // Character
       "." >>
         [indent, string_indent, indents](auto& m) {
           std::string mode = "string";
           if (m.in(LiteralString) || m.in(FoldedString))
           {
             mode = "multiline-string";
             if (*string_indent == 0)
             {
               *string_indent = *indent;
             }
             else if (*indent < *string_indent)
             {
               mode = "string";
               *string_indent = 0;
               if (m.in(LiteralString))
               {
                 m.pop(LiteralString);
               }
               else if (m.in(FoldedString))
               {
                 m.pop(FoldedString);
               }
             }
           }
           else if (m.in(SingleQuoteString) || m.in(DoubleQuoteString))
           {
             if (*string_indent == 0)
             {
               *string_indent = *indent;
             }
             m.mode("multiline-string");
             m.add(String);
             return;
           }
           else
           {
             if (*indent > indents->back())
             {
               m.push(Block);
               indents->push_back(*indent);
             }
           }

           while (*indent < indents->back())
           {
             m.term({Block});
             indents->pop_back();
           }
           m.term();
           m.mode(mode);
           m.add(String);
         }});

    p("string",
      {
        "\r*\n$" >>
          [indents](auto& m) {
            while (!indents->empty())
            {
              m.term({Block});
              indents->pop_back();
            }
          },

        "\r*\n" >>
          [indent](auto& m) {
            *indent = 0;
            m.term();
            m.mode("indent");
          },

        R"(\})" >>
          [stack](auto& m) {
            if (stack->back() == Collection::Brace)
            {
              m.term();
              m.pop(Brace);
              stack->pop_back();
              m.mode("start");
            }
            else
            {
              m.extend(String);
            }
          },

        R"(\])" >>
          [stack](auto& m) {
            if (stack->back() == Collection::Square)
            {
              m.term();
              m.pop(Square);
              stack->pop_back();
              m.mode("start");
            }
            else
            {
              m.extend(String);
            }
          },

        // Comma.
        ",[[:blank:]]*" >>
          [stack](auto& m) {
            if (stack->back() == Collection::None)
            {
              m.extend(String);
            }
            else
            {
              m.term();
              m.mode("start");
            }
          },

        ":" >>
          [](auto& m) {
            m.add(Colon);
            m.mode("start");
          },

        // Character
        "." >> [](auto& m) { m.extend(String); },
      });

    p("multiline-string",
      {R"(\\?\r*\n$)" >>
         [indents](auto& m) {
           while (!indents->empty())
           {
             m.term({Block});
             indents->pop_back();
           }
         },

       R"(\\?\r*\n)" >>
         [indent](auto& m) {
           *indent = 0;
           m.term();
           m.mode("indent");
         },

       R"(\\("))" >>
         [quote](auto& m) {
           if (*quote == Quote::Double)
           {
             m.add(String, 1);
           }
           else if (*quote == Quote::None)
           {
             m.extend(String);
           }
           else
           {
             m.invalid();
           }
         },

       R"(\\ )" >>
         [quote](auto& m) {
           if (*quote == Quote::Double)
           {
             m.add(String, 1);
           }
           else if (*quote == Quote::None)
           {
             m.extend(String);
           }
           else
           {
             m.invalid();
           }
         },

       R"(\\n)" >>
         [quote](auto& m) {
           if (*quote == Quote::Double)
           {
             m.term();
             m.add(NewLine);
           }
           else if (*quote == Quote::None)
           {
             m.extend(String);
           }
         },

       "\"" >>
         [string_indent, quote](auto& m) {
           if (*quote == Quote::Double)
           {
             m.term();
             *quote = Quote::None;
             *string_indent = 0;
             m.pop(DoubleQuoteString);
             m.mode("start");
           }
           else
           {
             m.extend(String);
           }
         },

       "'" >>
         [string_indent, quote](auto& m) {
           if (*quote == Quote::Single)
           {
             m.term();
             *quote = Quote::None;
             *string_indent = 0;
             m.pop(SingleQuoteString);
             m.mode("start");
           }
           else
           {
             m.extend(String);
           }
         },

       // Character
       "." >> [](auto& m) { m.extend(String); }});

    p("comment",
      {
        R"(\\?\r*\n$)" >>
          [indents](auto& m) {
            while (!indents->empty())
            {
              m.term({Block});
              indents->pop_back();
            }
          },

        R"(\\?\r*\n)" >>
          [indent](auto& m) {
            *indent = 0;
            m.term();
            m.mode("indent");
          },
        "." >> [](auto&) {},
      });

    p.done([indents](auto& m) {
      while (!indents->empty())
      {
        m.term({Block});
        indents->pop_back();
      }
    });

    p.gen({
      Integer >> [](auto& rnd) { return std::to_string(rnd() % 100); },
      Float >>
        [](auto& rnd) {
          std::uniform_real_distribution<> dist(-10.0, 10.0);
          return std::to_string(dist(rnd));
        },
      True >> [](auto&) { return "true"; },
      False >> [](auto&) { return "false"; },
      Null >> [](auto&) { return "null"; },
      String >> [](auto& rnd) { return rand_string(rnd); },
    });

    return p;
  }
}