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

  class Indent
  {
  public:
    Indent() : m_current(0), m_string_start(0)
    {
      m_indents.push_back(0);
    }

    void newline()
    {
      m_current = 0;
    }

    bool at_string_start()
    {
      return m_string_start > 0 && m_string_start == m_current;
    }

    void end_string()
    {
      m_string_start = 0;
    }

    void inc(std::size_t n = 1)
    {
      m_current += n;
    }

    bool push()
    {
      if (m_current > m_indents.back())
      {
        m_indents.push_back(m_current);
        return true;
      }

      return false;
    }

    template <typename M>
    void pop_blocks(M& m)
    {
      while (m_current < m_indents.back())
      {
        m.term({Block});
        m_indents.pop_back();
      }
    }

    template <typename M>
    void cleanup(M& m)
    {
      while (!m_indents.empty())
      {
        m.term({Block});
        m_indents.pop_back();
      }
    }

    template <typename M>
    bool complete(M& m, std::string mode)
    {
      if (m.in(LiteralString) || m.in(FoldedString))
      {
        if (m_string_start == 0)
        {
          m_string_start = m_current;
          m.mode("multiline-string");
          m.add(String);
          return false;
        }

        if (m_current < m_string_start)
        {
          m_string_start = 0;
          if (m.in(LiteralString))
          {
            m.pop(LiteralString);
          }
          else if (m.in(FoldedString))
          {
            m.pop(FoldedString);
          }
          pop_blocks(m);
          m.term();
          m.mode(mode);
          return true;
        }

        m.mode("multiline-string");
        m.add(String);
        return false;
      }

      if (m.in(SingleQuoteString) || m.in(DoubleQuoteString))
      {
        if (m_string_start == 0)
        {
          m_string_start = m_current;
        }
        m.mode("multiline-string");
        m.add(String);
        return false;
      }

      if (push())
      {
        m.push(Block);
      }
      else
      {
        pop_blocks(m);
      }

      m.term();
      m.mode(mode);
      return true;
    }

  private:
    std::size_t m_current;
    std::size_t m_string_start;
    std::vector<std::size_t> m_indents;
  };

  enum class Quote
  {
    None,
    Single,
    Double,
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
    auto indent = std::make_shared<Indent>();
    auto quote = std::make_shared<Quote>(Quote::None);
    auto stack = std::make_shared<std::vector<Collection>>();
    auto in_value = std::make_shared<bool>(false);
    stack->push_back(Collection::None);

    p("start",
      {
        "---" >>
          [](auto&) {
            // TODO: add support for YAML front matter
          },

        // Line comment.
        "#[^\n]*" >> [](auto&) {},

        "\r*\n" >>
          [indent, in_value](auto& m) {
            *in_value = false;
            indent->newline();
            m.term();
            m.mode("indent");
          },

        // Brace.
        R"(\{)" >>
          [stack, in_value](auto& m) {
            *in_value = false;
            m.term();
            m.push(Brace);
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
        R"(\[)" >>
          [stack](auto& m) {
            m.term();
            m.push(Square);
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
          [stack, in_value](auto& m) {
            if (stack->back() == Collection::None)
            {
              m.extend(String);
            }
            else
            {
              *in_value = false;
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
        ":[ ]*" >>
          [in_value](auto& m) {
            m.add(Colon);
            *in_value = true;
          },

        // Literal string
        "\\|" >> [](auto& m) { m.push(LiteralString); },

        // Folded string
        ">" >> [](auto& m) { m.push(FoldedString); },

        // Entry.
        "-[ \n][\r\n]*" >>
          [indent](auto& m) {
            indent->inc(2);
            indent->push();
            m.add(Hyphen);
            m.push(Block);
          },

        // True.
        "true\\b" >> [](auto& m) { m.add(True); },

        // False.
        "false\\b" >> [](auto& m) { m.add(False); },

        // Null.
        "null\\b" >> [](auto& m) { m.add(Null); },

        // Not a number
        R"((?:(?:[[:digit:]]+)\.\b){2})" >>
          [](auto& m) {
            m.add(String);
            m.mode("string");
          },

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

       "\r*\n" >>
         [indent](auto& m) {
           if (indent->at_string_start())
           {
             m.add(Blank);
             m.term();
           }
           indent->newline();
         },

       // Space
       " " >>
         [indent](auto& m) {
           if (indent->at_string_start())
           {
             m.mode("multiline-string");
             m.add(String);
           }
           else
           {
             indent->inc();
           }
         },

       // Hyphen
       "-[ \n][\r\n]*" >>
         [indent](auto& m) {
           m.mode("start");
           if (m.in(LiteralString) || m.in(FoldedString))
           {
             indent->end_string();
             if (m.in(LiteralString))
             {
               m.pop(LiteralString);
             }
             else
             {
               m.pop(FoldedString);
             }
           }

           indent->pop_blocks(m);
           indent->inc(2);
           indent->push();
           m.add(Hyphen);
           m.push(Block);
         },

       // Double quote string
       "\"" >>
         [quote, indent](auto& m) {
           if (indent->push())
           {
             m.push(Block);
           }
           else
           {
             indent->pop_blocks(m);
             m.term();
           }
           m.push(DoubleQuoteString, 1);
           m.mode("multiline-string");
           *quote = Quote::Double;
         },

       // Single quote string
       "'" >>
         [quote, indent](auto& m) {
           if (indent->push())
           {
             m.push(Block);
           }
           else
           {
             indent->pop_blocks(m);
             m.term();
           }

           m.push(SingleQuoteString, 1);
           m.mode("multiline-string");
           *quote = Quote::Single;
         },

       "#" >> [indent](auto& m) { indent->complete(m, "comment"); },

       R"(\{)" >>
         [indent, stack, in_value](auto& m) {
           if (indent->complete(m, "start"))
           {
             *in_value = false;
             stack->push_back(Collection::Brace);
             m.term();
             m.push(Brace);
           }
         },

       R"(\})" >>
         [indent, stack](auto& m) {
           if (stack->back() == Collection::Brace)
           {
             m.term();
             m.pop(Brace);
             stack->pop_back();
             m.mode("start");
           }
           else if (indent->complete(m, "string"))
           {
             m.add(String);
           }
         },

       R"(\[)" >>
         [indent, stack](auto& m) {
           if (indent->complete(m, "start"))
           {
             stack->push_back(Collection::Square);
             m.term();
             m.push(Square);
           }
         },

       R"(\])" >>
         [indent, stack](auto& m) {
           if (stack->back() == Collection::Square)
           {
             m.term();
             m.pop(Square);
             stack->pop_back();
             m.mode("start");
           }
           else if (indent->complete(m, "string"))
           {
             m.add(String);
           }
         },

       // Character
       "." >>
         [indent](auto& m) {
           if (indent->complete(m, "string"))
           {
             m.add(String);
           }
         }});

    p("string",
      {
        "\r*\n" >>
          [indent, in_value](auto& m) {
            indent->newline();
            *in_value = false;
            m.term();
            m.mode("indent");
          },

        R"(\})" >>
          [stack, quote](auto& m) {
            if (*quote == Quote::None && stack->back() == Collection::Brace)
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
          [stack, quote](auto& m) {
            if (*quote == Quote::None && stack->back() == Collection::Square)
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
          [stack, in_value](auto& m) {
            if (stack->back() == Collection::None)
            {
              m.extend(String);
            }
            else
            {
              *in_value = false;
              m.term();
              m.mode("start");
            }
          },

        ":" >>
          [in_value](auto& m) {
            if (*in_value)
            {
              m.extend(String);
            }
            else
            {
              *in_value = true;
              m.add(Colon);
              m.mode("start");
            }
          },

        // Character
        "." >> [](auto& m) { m.extend(String); },
      });

    p("multiline-string",
      {R"(\\?\r*\n)" >>
         [indent](auto& m) {
           indent->newline();
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
         [indent, quote](auto& m) {
           if (*quote == Quote::Double)
           {
             m.term();
             *quote = Quote::None;
             indent->end_string();
             m.pop(DoubleQuoteString);
             m.mode("start");
           }
           else
           {
             m.extend(String);
           }
         },

       "'" >>
         [indent, quote](auto& m) {
           if (*quote == Quote::Single)
           {
             m.term();
             *quote = Quote::None;
             indent->end_string();
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
        R"(\\?\r*\n)" >>
          [indent](auto& m) {
            indent->newline();
            m.term();
            m.mode("indent");
          },
        "." >> [](auto&) {},
      });

    p.done([indent, stack](auto& m) {
      if (stack->size() > 1)
      {
        m.error("Unclosed braces");
      }
      indent->cleanup(m);
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