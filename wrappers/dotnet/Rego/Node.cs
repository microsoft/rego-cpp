namespace Rego;

#nullable enable

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

/// <summary>
/// Node types for Rego AST nodes
/// </summary>
public enum NodeType
{
  /// <summary>
  /// A binding of a variable to a term
  /// </summary>
  Binding = 1000,

  /// <summary>
  /// A variable
  /// </summary>
  Var = 1001,

  /// <summary>
  /// A term, which can be a scalar, array, set, or object
  /// </summary>
  Term = 1002,

  /// <summary>
  /// A scalar value (int, float, string, true, false, null)
  /// </summary>
  Scalar = 1003,

  /// <summary>,
  /// An array of terms
  /// </summary>
  Array = 1004,

  /// <summary>
  /// A set of terms. Will not contain duplicates.
  /// </summary>
  Set = 1005,

  /// <summary>
  /// An object, which is a collection of ObjectItem nodes
  /// </summary>
  Object = 1006,

  /// <summary>
  /// An item in an object, which is a key-value pair. The first child is the key, the second child is the value.
  /// </summary>
  ObjectItem = 1007,

  /// <summary>
  /// An integer value
  /// </summary>
  Int = 1008,

  /// <summary>
  /// A floating point value
  /// </summary>
  Float = 1009,

  /// <summary>
  /// A string value
  /// </summary>
  String = 1010,
  /// <summary>
  /// A boolean value representing true
  /// </summary>
  True = 1011,
  /// <summary>
  /// A boolean value representing false
  /// </summary>
  False = 1012,
  /// <summary>
  /// A null value
  /// </summary>
  Null = 1013,
  /// <summary>
  /// An undefined value
  /// </summary>
  Undefined = 1014,
  /// <summary>
  /// A list of terms
  /// </summary>
  Terms = 1015,
  /// <summary>
  /// A list of bindings
  /// </summary>
  Bindings = 1016,
  /// <summary>
  /// A list of results
  /// </summary>
  Results = 1017,
  /// <summary>
  /// A result, consiting of a list of bindings and a list of terms
  /// </summary>
  Result = 1018,
  /// <summary>
  /// An error node, containing an error message, an error AST, and an error code
  /// </summary>
  Error = 1800,
  /// <summary>
  /// An error message
  /// </summary>
  ErrorMessage = 1801,
  /// <summary>
  /// An error AST showing where in the code tree the error occurred
  /// </summary>
  ErrorAst = 1802,
  /// <summary>
  /// An error code
  /// </summary>
  ErrorCode = 1803,
  /// <summary>
  /// An error sequence
  /// </summary>
  ErrorSeq = 1804,
  /// <summary>
  /// An internal node type, not exposed to users
  /// </summary>
  Internal = 1999
}

/// <summary>
/// Represents a null value in Rego
/// </summary>
public record RegoNull { }

/// <summary>
/// Interface for a Rego Node.
/// 
/// Rego Nodes are the basic building blocks of a Rego result. They
/// exist in a tree structure. Each node has a kind, which is one of
/// the variants of <seealso cref="NodeType"/>. Each node also has zero or more
/// children, which are also nodes.
/// </summary>
/// <example>
/// <code>
/// Interpreter rego = new();
/// var output = rego.Query("""x={"a": 10, "b": "20", "c": [30.5, 60], "d": true, "e": null}""");
/// var x = output.Binding("x");
/// Console.WriteLine(x);
/// // {"a":10, "b":"20", "c":[30.5,60], "d":true, "e":null}
///
/// Console.WriteLine(x["a"]);
/// // 10
///
/// Console.WriteLine(x["b"]);
/// // "20"
///
/// Console.WriteLine(x["c"][0]);
/// // 30.5
///
/// Console.WriteLine(x["c"][1]);
/// // 60
///
/// Console.WriteLine(x["d"]);
/// // true
///
/// Console.WriteLine(x["e"]);
/// // null
/// </code>
/// </example>
public partial class Node : IList<Node>
{
  private readonly IntPtr m_ptr;
  private readonly NodeType m_type;
  private object? m_value;
  private string? m_json;

  private readonly Node?[] m_children;

  private readonly Dictionary<string, Node> m_lookup;

  [LibraryImport("rego_shared")]
  private static partial uint regoNodeType(IntPtr ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoNodeValueSize(IntPtr ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoNodeValue(IntPtr ptr, IntPtr buffer, uint size);

  [LibraryImport("rego_shared")]
  private static partial uint regoNodeJSONSize(IntPtr ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoNodeJSON(IntPtr ptr, IntPtr buffer, uint size);


  [LibraryImport("rego_shared")]
  private static partial uint regoNodeSize(IntPtr ptr);

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoNodeGet(IntPtr ptr, uint index);

  internal Node(IntPtr ptr)
  {
    m_ptr = ptr;
    m_type = (NodeType)regoNodeType(ptr);
    uint num_children = regoNodeSize(ptr);
    if (m_type == NodeType.Object)
    {
      m_children = new Node?[num_children];
      m_lookup = new Dictionary<string, Node>();
      for (uint i = 0; i < num_children; i++)
      {
        var child_ptr = regoNodeGet(ptr, i);
        if (child_ptr == IntPtr.Zero)
        {
          throw new RegoException("Failed to get child node");
        }

        Node child = new(child_ptr);
        m_children[i] = child;
        Node key = child.At(0);
        Node value = child.At(1);
        m_lookup[key.ToJson()] = value;
      }
    }
    else if (m_type == NodeType.Set)
    {
      m_children = new Node?[num_children];
      m_lookup = new Dictionary<string, Node>();
      for (uint i = 0; i < num_children; i++)
      {
        var child_ptr = regoNodeGet(ptr, i);
        if (child_ptr == IntPtr.Zero)
        {
          throw new RegoException("Failed to get child node");
        }

        Node child = new(child_ptr);
        m_children[i] = child;
        m_lookup[child.ToJson()] = child;
      }
    }
    else
    {
      m_children = new Node?[num_children];
      m_lookup = [];
    }

    m_value = null;
    m_json = null;
  }

  /// <summary>
  /// Pointer to the underlying native node.
  /// </summary>
  public IntPtr Pointer
  {
    get
    {
      return m_ptr;
    }
  }

  /// <summary>
  /// The type of this node.
  /// </summary>
  public NodeType Type
  {
    get
    {
      return m_type;
    }
  }

  /// <summary>
  /// The value of this node, if it is a scalar type (int, float, string, true, false, null).
  /// If the node is a Term or Scalar, the value of the first child is returned.
  /// For other node types, an exception is thrown.
  /// The value is cached after the first call.
  /// </summary>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// Interpreter rego = new();
  /// var output = rego.Query("""x=10; y="20"; z=true""");
  /// Console.WriteLine(output.Binding("x").Value);
  /// // 10
  ///
  /// Console.WriteLine(output.Binding("y").Value);
  /// // "20"
  ///
  /// Console.WriteLine(output.Binding("z").Value);
  /// // True
  /// </code>
  /// </example>
  public object Value
  {
    get
    {
      if (m_type == NodeType.Term || m_type == NodeType.Scalar)
      {
        Node? first = At(0) ?? throw new RegoException("Term or Scalar node has no children");
        return first.Value;
      }

      if (m_value != null)
      {
        return m_value;
      }

      string value = Interop.GetString(() => regoNodeValueSize(m_ptr), (IntPtr buffer, uint size) => regoNodeValue(m_ptr, buffer, size)) ?? throw new RegoException("Failed to get node value");
      m_value = m_type switch
      {
        NodeType.Int => long.Parse(value),
        NodeType.Float => double.Parse(value),
        NodeType.String => value,
        NodeType.True => true,
        NodeType.False => false,
        NodeType.Null => new RegoNull(),
        _ => value,
      };

      return m_value;
    }
  }

  /// <summary>
  /// Returns a JSON representation of this node.
  /// The JSON is cached after the first call.
  /// </summary>
  /// <returns>a JSON string representation of this node</returns>
  /// <exception cref="RegoException"></exception>
  public string ToJson()
  {
    if (m_json != null)
    {
      return m_json;
    }

    uint size = regoNodeJSONSize(m_ptr);
    if (size == 0)
    {
      m_json = "";
      return m_json;
    }

    m_json = Interop.GetString(() => regoNodeJSONSize(m_ptr), (IntPtr buffer, uint size) => regoNodeJSON(m_ptr, buffer, size)) ?? throw new RegoException("Failed to get node JSON");
    return m_json;
  }

  /// <summary>
  /// Returns the child node at the specified index.
  /// If the node is a Term, the first child is returned.
  /// The child node is cached after the first call.
  /// </summary>
  /// <param name="index">Index of the child node</param>
  /// <returns>The child node at the specified index</returns>
  /// <exception cref="IndexOutOfRangeException"></exception>
  /// <exception cref="RegoException"></exception>
  public Node At(int index)
  {
    Node? child = m_children[index];
    if (child != null)
    {
      return child;
    }

    if (index < 0 || index >= m_children.Length)
    {
      throw new IndexOutOfRangeException();
    }

    var ptr = regoNodeGet(m_ptr, (uint)index);
    if (ptr == IntPtr.Zero)
    {
      throw new RegoException("Failed to get child node");
    }

    child = new Node(ptr);
    m_children[index] = child;
    return child;
  }

  /// <summary>
  /// Returns the child node at the specified index.
  /// If the node is a Term, the first child is used as the base node.
  /// If the node is an Array, Terms, or Results, the child at the specified index is returned.
  /// For other node types, an exception is thrown.
  /// </summary>
  /// <param name="index">Index of the child node</param>
  /// <returns>The child node at the specified index</returns>
  /// <exception cref="NotSupportedException"></exception>
  /// <exception cref="IndexOutOfRangeException"></exception>
  /// <exception cref="RegoException"></exception>
  public Node Index(int index)
  {
    return m_type switch
    {
      NodeType.Term => At(0).Index(index),
      NodeType.Array => At(index),
      NodeType.Terms => At(index),
      NodeType.Results => At(index),
      _ => throw new NotSupportedException("Indexing not supported for this node type")
    };
  }

  /// <summary>
  /// Looks up a child node by key.
  /// If the node is a Term, the first child is used as the base node.
  /// If the node is an Object or Set, the child with the specified key is returned.
  /// For other node types, an exception is thrown.
  /// </summary>
  /// <param name="key">The key to look up.</param>
  /// <returns>The child node with the specified key</returns>
  /// <exception cref="NotSupportedException"></exception>
  /// <exception cref="KeyNotFoundException"></exception>
  /// <exception cref="RegoException"></exception>
  public Node Lookup(object key)
  {
    if (m_type == NodeType.Term)
    {
      return At(0).Lookup(key);
    }

    string json = JsonSerializer.Serialize(key);
    return m_type switch
    {
      NodeType.Object => m_lookup[json],
      NodeType.Set => m_lookup[json],
      _ => throw new NotSupportedException("Lookup not supported for this node type")
    };
  }

  /// <summary>
  /// Adds a child node. This is not supported and will always throw an exception.
  /// </summary>
  /// <param name="value"></param>
  /// <exception cref="NotSupportedException"></exception>
  public void Add(Node value)
  {
    throw new NotSupportedException();
  }

  /// <summary>
  /// Clears all child nodes. This is not supported and will always throw an exception.
  /// </summary>
  /// <exception cref="NotSupportedException"></exception>
  public void Clear()
  {
    throw new NotSupportedException();
  }

  /// <summary>
  /// Checks if the specified node is a child of this node.
  /// If the node is a Term, the first child is used as the base node.
  /// For other node types, the child nodes are checked.
  /// The comparison is done by comparing the JSON representation of the nodes.
  /// </summary>
  /// <param name="value">the <see cref="Node"/> to check</param>
  /// <returns>whether the node is a child of this node</returns>
  public bool Contains(Node value)
  {
    string json = value.ToJson();
    for (int i = 0; i < Count; i++)
    {
      if (At(i).ToJson() == json)
      {
        return true;
      }
    }

    return false;
  }

  /// <summary>
  /// Returns the index of the specified child node.
  /// If the node is a Term, the first child is used as the base node.
  /// For other node types, the child nodes are checked.
  /// The comparison is done by comparing the JSON representation of the nodes.
  /// If the node is not found, -1 is returned.
  /// </summary>
  /// <param name="value">the <see cref="Node"/> to find</param>
  /// <returns>the index of the child node, or -1 if not found</returns>
  public int IndexOf(Node value)
  {
    string json = value.ToJson();
    for (int i = 0; i < Count; i++)
    {
      if (At(i).ToJson() == json)
      {
        return i;
      }
    }

    return -1;
  }

  /// <summary>
  /// Inserts a child node at the specified index. This is not supported and will always throw an exception.
  /// </summary>
  /// <param name="index"></param>
  /// <param name="value"></param>
  /// <exception cref="NotSupportedException"></exception>
  public void Insert(int index, Node value)
  {
    throw new NotSupportedException();
  }

  /// <summary>
  /// Indicates whether the collection is read-only. This is always true.
  /// </summary>
  public bool IsReadOnly
  {
    get
    {
      return true;
    }
  }

  /// <summary>
  /// Removes the specified child node. This is not supported and will always throw an exception.
  /// </summary>
  /// <param name="value"></param>
  /// <returns></returns>
  /// <exception cref="NotSupportedException"></exception>
  public bool Remove(Node value)
  {
    throw new NotSupportedException();
  }

  /// <summary>
  /// Removes the child node at the specified index. This is not supported and will always throw an exception.
  /// </summary>
  /// <param name="index"></param>
  /// <exception cref="NotSupportedException"></exception>
  public void RemoveAt(int index)
  {
    throw new NotSupportedException();
  }

  /// <summary>
  /// Returns a child node by key or index.
  /// If the node is a Term, the first child is used as the base node.
  /// If the node is an Object or Set, the child with the specified key is returned.
  /// If the node is an Array, Terms, or Results, the child at the specified index is returned.
  /// For other node types, an exception is thrown.
  /// </summary>
  /// <param name="key">the key or index of the child node</param>
  /// <returns>the child node</returns>
  /// <exception cref="ArgumentException"></exception>
  /// <exception cref="NotSupportedException"></exception>
  public Node this[object key]
  {
    get
    {
      NodeType type = m_type;
      if (type == NodeType.Term)
        type = At(0).m_type;

      if (type == NodeType.Set || type == NodeType.Object)
        return Lookup(key);

      if (key is int index)
        return Index(index);

      throw new ArgumentException("Invalid key type");
    }
    set
    {
      throw new NotSupportedException();
    }
  }

  /// <summary>
  /// Returns the child node at the specified index.
  /// If the node is a Term, the first child is used as the base node.
  /// If the node is an Array, Terms, or Results, the child at the specified index is returned.
  /// For other node types, an exception is thrown.
  /// This is read-only and will always throw an exception if you try to set a value.
  /// </summary>
  /// <param name="index">the index of the child node</param>
  /// <returns>the child node</returns>
  /// <exception cref="NotSupportedException"></exception>
  Node IList<Node>.this[int index]
  {
    get
    {
      return Index(index);
    }
    set
    {
      throw new NotSupportedException();
    }
  }

  /// <summary>
  /// Copies the child nodes to the specified array, starting at the specified index.
  /// If the node is a Term, the first child is used as the base node.
  /// For other node types, the child nodes are copied.
  /// </summary>
  /// <param name="array">The array to copy the child nodes to.</param>
  /// <param name="index">The zero-based index at which copying begins.</param>
  public void CopyTo(Node[] array, int index)
  {
    for (int i = 0; i < Count; i++, index++)
    {
      array[index] = At(i);
    }
  }

  /// <summary>
  /// The number of child nodes.
  /// </summary>
  public int Count
  {
    get
    {
      if (m_type == NodeType.Term)
      {
        return At(0).Count;
      }

      return m_children.Length;
    }
  }

  /// <summary>
  /// Returns an enumerator that iterates through the child nodes.
  /// If the node is a Term, the first child is used as the base node.
  /// For other node types, the child nodes are iterated.
  /// </summary>
  /// <returns>the enumerator</returns>
  IEnumerator IEnumerable.GetEnumerator()
  {
    for (int i = 0; i < Count; i++)
    {
      yield return At(i);
    }
  }

  /// <summary>
  /// Returns an enumerator that iterates through the child nodes.
  /// If the node is a Term, the first child is used as the base node.
  /// For other node types, the child nodes are iterated.
  /// </summary>
  /// <returns>the enumerator</returns>
  public IEnumerator<Node> GetEnumerator()
  {
    // Refer to the IEnumerator documentation for an example of
    // implementing an enumerator.
    for (int i = 0; i < Count; i++)
    {
      yield return At(i);
    }
  }

  /// <summary>
  /// The keys of the child nodes, if this node is an Object or Set.
  /// If the node is a Term, the first child is used as the base node.
  /// For other node types, an empty collection is returned.
  /// The keys are the JSON representation of the child nodes.
  /// </summary>
  public ICollection<string> Keys
  {
    get
    {
      return m_lookup.Keys;
    }
  }

  /// <summary>
  /// Checks if the specified key exists in the child nodes.
  /// If the node is a Term, the first child is used as the base node.
  /// If the node is an Object or Set, the keys are checked.
  /// For other node types, an exception is thrown.
  /// The comparison is done by comparing the JSON representation of the keys.
  /// </summary>
  /// <param name="key">the key to check</param>
  /// <returns>true if the key exists; otherwise, false</returns>
  public bool ContainsKey(object key)
  {
    var json = JsonSerializer.Serialize(key);
    return m_lookup.ContainsKey(json);
  }

  /// <summary>
  /// Tries to get the child node with the specified key.
  /// If the node is a Term, the first child is used as the base node.
  /// If the node is an Object or Set, the keys are checked.
  /// For other node types, an exception is thrown.
  /// The comparison is done by comparing the JSON representation of the keys.
  /// </summary>
  /// <param name="key">the key to look up</param>
  /// <param name="value">when this method returns, contains the child node associated with the specified key, if the key is found; otherwise, null</param>
  /// <returns>true if the key was found; otherwise, false</returns>
  public bool TryGetValue(object key, out Node? value)
  {
    var json = JsonSerializer.Serialize(key);
    return m_lookup.TryGetValue(json, out value);
  }

  /// <summary>
  /// Returns a JSON representation of this node.
  /// This is the same as calling <see cref="ToJson()"/>.
  /// </summary>
  /// <returns>the JSON representation of this node</returns>
  public override string ToString()
  {
    return ToJson();
  }
}