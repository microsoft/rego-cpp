#include "builtins.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  using NodeIndex = std::map<std::string, Node>;
  using Graph = std::map<std::string_view, std::set<std::string_view>>;

  std::string_view lookup(NodeIndex& index, Node node)
  {
    std::string key = to_key(node);
    if (!index.contains(key))
    {
      index[key] = node;
    }

    return index.find(key)->first;
  }

  void index_nodes(NodeIndex& index, Node array)
  {
    for (auto& node : *array)
    {
      std::string key = to_key(node);
      index[key] = node;
    }
  }

  Node build_graph(Graph& graph, Node edges, NodeIndex& index)
  {
    for (auto& edge : *edges)
    {
      std::string_view src = lookup(index, edge / Key);

      auto maybe_dst = unwrap(edge / Val, {Set, Array, Null});
      if (!maybe_dst.success)
      {
        return err(maybe_dst.node, "graph.reachable: expected a set/array");
      }

      if (maybe_dst.node == Null)
      {
        graph[src] = {};
        continue;
      }

      std::set<std::string_view> dsts = {};
      for (auto& node : *maybe_dst.node)
      {
        dsts.insert(lookup(index, node));
      }

      graph[src] = dsts;
    }

    return nullptr;
  }

  Node reachable(const Nodes& args)
  {
    Node graph_object =
      unwrap_arg(args, UnwrapOpt(0).type(Object).func("graph.reachable"));
    if (graph_object->type() == Error)
    {
      return graph_object;
    }

    Node initial_nodes = unwrap_arg(
      args, UnwrapOpt(1).types({Set, Array}).func("graph.reachable"));
    if (initial_nodes->type() == Error)
    {
      return initial_nodes;
    }

    NodeIndex nodes;
    index_nodes(nodes, initial_nodes);

    std::vector<std::string_view> initial_set;
    for (auto& [key, _] : nodes)
    {
      initial_set.push_back(key);
    }

    Graph graph;
    Node err = build_graph(graph, graph_object, nodes);
    if (err != nullptr)
    {
      return err;
    }

    std::set<std::string_view> visited;
    std::vector<std::string_view> frontier;
    frontier.insert(frontier.end(), initial_set.begin(), initial_set.end());

    while (!frontier.empty())
    {
      std::string_view current = frontier.back();
      frontier.pop_back();
      if (visited.contains(current))
      {
        continue;
      }

      if (graph.find(current) == graph.end())
      {
        continue;
      }

      visited.insert(current);
      frontier.insert(
        frontier.end(), graph[current].begin(), graph[current].end());
    }

    Node argseq = NodeDef::create(Seq);
    for (auto& key : visited)
    {
      argseq->push_back(nodes[std::string(key)]);
    }

    return Resolver::set(argseq);
  }

  BuiltIn reachable_factory()
  {
    const Node reachable_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg
              << (bi::Name ^ "graph")
              << (bi::Description ^
                  "object containing a set or array of neighboring vertices")
              << (bi::Type
                  << (bi::DynamicObject
                      << (bi::Type << bi::Any)
                      << (bi::Type
                          << (bi::TypeSeq
                              << (bi::Type
                                  << (bi::DynamicArray
                                      << (bi::Type << bi::Any)))
                              << (bi::Type
                                  << (bi::Set << (bi::Type << bi::Any))))))))
          << (bi::Arg << (bi::Name ^ "initial")
                      << (bi::Description ^ "set or array of root vertices")
                      << (bi::Type
                          << (bi::TypeSeq
                              << (bi::Type
                                  << (bi::DynamicArray
                                      << (bi::Type << bi::Any)))
                              << (bi::Type
                                  << (bi::Set << (bi::Type << bi::Any)))))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "set of vertices from the `initial` vertices in the "
                         "directed `graph`")
                     << (bi::Type << (bi::Set << (bi::Type << bi::Any))));
    return BuiltInDef::create({"graph.reachable"}, reachable_decl, reachable);
  }

  class Path
  {
  private:
    std::vector<std::string_view> m_order;
    std::set<std::string_view> m_visited;

  public:
    Path(const Path& path) :
      m_order(path.m_order.begin(), path.m_order.end()),
      m_visited(path.m_visited.begin(), path.m_visited.end())
    {}

    Path(Path&& other) :
      m_order(std::move(other.m_order)), m_visited(std::move(other.m_visited))
    {}

    Path(std::string_view key)
    {
      m_order.push_back(key);
      m_visited.insert(key);
    }

    Path operator+(std::string_view key)
    {
      Path new_path = *this;
      new_path.m_order.push_back(key);
      new_path.m_visited.insert(key);
      return new_path;
    }

    Path& operator=(const Path& path)
    {
      m_order =
        std::vector<std::string_view>(path.m_order.begin(), path.m_order.end());
      m_visited = std::set<std::string_view>(
        path.m_visited.begin(), path.m_visited.end());
      return *this;
    }

    Path& operator=(Path&& other)
    {
      m_order = std::move(other.m_order);
      m_visited = std::move(other.m_visited);
      return *this;
    }

    std::string_view back() const
    {
      return m_order.back();
    }

    const std::set<std::string_view>& visited() const
    {
      return m_visited;
    }

    Node to_node(const NodeIndex& index) const
    {
      Node argseq = NodeDef::create(Seq);
      for (auto& key : m_order)
      {
        std::string key_str(key);
        argseq->push_back(index.at(key_str));
      }

      return Resolver::array(argseq);
    }
  };

  Node reachable_paths(const Nodes& args)
  {
    Node graph_object =
      unwrap_arg(args, UnwrapOpt(0).type(Object).func("graph.reachable"));
    if (graph_object->type() == Error)
    {
      return graph_object;
    }

    Node initial_nodes = unwrap_arg(
      args, UnwrapOpt(1).types({Set, Array}).func("graph.reachable"));
    if (initial_nodes->type() == Error)
    {
      return initial_nodes;
    }

    NodeIndex nodes;
    index_nodes(nodes, initial_nodes);

    std::vector<std::string_view> initial_set;
    for (auto& [key, _] : nodes)
    {
      initial_set.push_back(key);
    }

    Graph graph;
    Node err = build_graph(graph, graph_object, nodes);
    if (err != nullptr)
    {
      return err;
    }

    std::vector<Path> paths;
    std::vector<Path> frontier;
    std::transform(
      initial_set.begin(),
      initial_set.end(),
      std::back_inserter(frontier),
      [](std::string_view key) { return Path{key}; });

    while (!frontier.empty())
    {
      Path path = std::move(frontier.back());
      frontier.pop_back();

      auto current = path.back();

      if (graph.find(current) == graph.end())
      {
        continue;
      }

      std::vector<std::string_view> unvisited;
      std::set_difference(
        graph[current].begin(),
        graph[current].end(),
        path.visited().begin(),
        path.visited().end(),
        std::back_inserter(unvisited));

      std::vector<std::string_view> neighbors;
      std::copy_if(
        unvisited.begin(),
        unvisited.end(),
        std::back_inserter(neighbors),
        [&graph](std::string_view key) { return graph.contains(key); });

      if (neighbors.empty())
      {
        paths.push_back(std::move(path));
      }
      else
      {
        std::transform(
          neighbors.begin(),
          neighbors.end(),
          std::back_inserter(frontier),
          [&path](std::string_view key) { return path + key; });
      }
    }

    Node argseq = NodeDef::create(Seq);
    for (auto& path : paths)
    {
      argseq->push_back(path.to_node(nodes));
    }

    return Resolver::set(argseq);
  }

  BuiltIn reachable_paths_factory()
  {
    const Node reachable_paths_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg
              << (bi::Name ^ "graph")
              << (bi::Description ^
                  "object containing a set or array of neighboring vertices")
              << (bi::Type
                  << (bi::DynamicObject
                      << (bi::Type << bi::Any)
                      << (bi::Type
                          << (bi::TypeSeq
                              << (bi::Type
                                  << (bi::DynamicArray
                                      << (bi::Type << bi::Any)))
                              << (bi::Type
                                  << (bi::Set << (bi::Type << bi::Any))))))))
          << (bi::Arg << (bi::Name ^ "initial")
                      << (bi::Description ^ "set or array of root vertices")
                      << (bi::Type
                          << (bi::TypeSeq
                              << (bi::Type
                                  << (bi::DynamicArray
                                      << (bi::Type << bi::Any)))
                              << (bi::Type
                                  << (bi::Set << (bi::Type << bi::Any)))))))
      << (bi::Result << (bi::Name ^ "output")
                     << (bi::Description ^
                         "set of vertices from the `initial` vertices in the "
                         "directed `graph`")
                     << (bi::Type
                         << (bi::Set
                             << (bi::Type
                                 << (bi::DynamicArray
                                     << (bi::Type << bi::Any))))));
    return BuiltInDef::create(
      {"graph.reachable_paths"}, reachable_paths_decl, reachable_paths);
  }
}

namespace rego
{
  namespace builtins
  {
    BuiltIn graph(const Location& name)
    {
      assert(name.view().starts_with("graph."));
      std::string_view view = name.view().substr(6); // skip "graph."
      if (view == "reachable")
      {
        return reachable_factory();
      }
      if (view == "reachable_paths")
      {
        return reachable_paths_factory();
      }

      return nullptr;
    }
  }
}