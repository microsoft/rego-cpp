#include "rego.hh"
#include "trieste/wf.h"

#include <stdexcept>

namespace rego
{
  Output::Output(Node node) : m_node(node)
  {
    if (node == nullptr || node == ErrorSeq)
    {
      m_json = "<error>";
    }
    else
    {
      assert(node == Results);
      std::ostringstream buf;
      WFContext context(rego::wf_result);
      buf << rego::to_key(m_node, true);
      m_json = buf.str();
    }
  }

  bool Output::ok() const
  {
    return !(m_node == nullptr || m_node == ErrorSeq);
  }

  Node Output::node() const
  {
    return m_node;
  }

  size_t Output::size() const
  {
    return m_node->size();
  }

  Node Output::expressions_at(size_t index) const
  {
    if (!ok())
    {
      throw std::logic_error("An invalid output has no expressions");
    }

    if (index >= m_node->size())
    {
      throw std::out_of_range("Index out of range");
    }

    WFContext context(rego::wf_result);
    Node result = m_node->at(index);
    return result / rego::Terms;
  }

  Node Output::expressions() const
  {
    return expressions_at(0);
  }

  Node Output::binding_at(size_t index, const std::string& name) const
  {
    if (!ok())
    {
      throw std::logic_error("An invalid output has no expressions");
    }

    if (index >= m_node->size())
    {
      throw std::out_of_range("Index out of range");
    }

    WFContext context(rego::wf_result);
    auto result = m_node->at(index);
    auto defs = result->lookdown({name});
    if (defs.empty())
    {
      throw std::invalid_argument("Name is not a valid binding");
    }

    Node val = defs[0] / rego::Val;
    assert(val == Term);
    val = val->front();
    return val;
  }

  Node Output::binding(const std::string& name) const
  {
    return binding_at(0, name);
  }

  const std::string& Output::json() const
  {
    return m_json;
  }

  std::vector<std::string> Output::errors() const
  {
    if (ok())
    {
      return {};
    }

    std::vector<std::string> result;
    for (Node error : *m_node)
    {
      std::ostringstream output_buf;
      Node error_msg = error->at(0);
      Node error_ast = error->at(1);
      output_buf << "error: " << error_msg->location().view() << std::endl;
      auto error_code = error->find_first(ErrorCode, error->begin());
      if (error_code != error->end())
      {
        output_buf << "code: " << (*error_code)->location().view() << std::endl;
      }
      output_buf << error_ast;
      result.push_back(output_buf.str());
    }

    return result;
  }
}