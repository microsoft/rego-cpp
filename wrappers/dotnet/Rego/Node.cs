namespace Rego;

#nullable enable

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

public enum NodeType
{
  Binding = 1000,
  Var = 1001,
  Term = 1002,
  Scalar = 1003,
  Array = 1004,
  Set = 1005,
  Object = 1006,
  ObjectItem = 1007,
  Int = 1008,
  Float = 1009,
  String = 1010,
  True = 1011,
  False = 1012,
  Null = 1013,
  Undefined = 1014,
  Terms = 1015,
  Bindings = 1016,
  Results = 1017,
  Result = 1018,
  Error = 1800,
  ErrorMessage = 1801,
  ErrorAst = 1802,
  ErrorCode = 1803,
  ErrorSeq = 1804,
  Internal = 1999
}

public record RegoNull { }

public partial class Node : IList<Node>
{
  private readonly IntPtr m_ptr;
  private readonly NodeType m_type;
  private object? m_value;
  private string? m_json;

  private readonly Node?[] m_children;

  private readonly Dictionary<string, Node> m_lookup;

  [LibraryImport("rego_shared")]
  private static partial int regoNodeType(IntPtr ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoNodeValueSize(IntPtr ptr);

  [LibraryImport("rego_shared")]
  private static partial int regoNodeValue(IntPtr ptr, IntPtr buffer, uint size);

  [LibraryImport("rego_shared")]
  private static partial uint regoNodeJSONSize(IntPtr ptr);

  [LibraryImport("rego_shared")]
  private static partial int regoNodeJSON(IntPtr ptr, IntPtr buffer, uint size);


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
        Node child = new(regoNodeGet(ptr, i));
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
        Node child = new(regoNodeGet(ptr, i));
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

  public IntPtr Pointer
  {
    get
    {
      return m_ptr;
    }
  }

  public NodeType Type
  {
    get
    {
      return m_type;
    }
  }

  public object Value
  {
    get
    {
      if (m_type == NodeType.Term || m_type == NodeType.Scalar)
      {
        return At(0).Value;
      }

      if (m_value != null)
      {
        return m_value;
      }

      uint size = regoNodeValueSize(m_ptr);
      IntPtr buffer = Marshal.AllocHGlobal((int)size);
      try
      {
        RegoCode result = (RegoCode)regoNodeValue(m_ptr, buffer, size);
        if (result != RegoCode.REGO_OK)
        {
          throw new Exception($"Failed to get node value: {result}");
        }

        string value = Marshal.PtrToStringUTF8(buffer, (int)size - 1);
        m_value = m_type switch
        {
          NodeType.Int => int.Parse(value),
          NodeType.Float => double.Parse(value),
          NodeType.String => value,
          NodeType.True => true,
          NodeType.False => false,
          NodeType.Null => new RegoNull(),
          _ => value,
        };

        return m_value;
      }
      finally
      {
        Marshal.FreeHGlobal(buffer);
      }
    }
  }

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

    IntPtr buffer = Marshal.AllocHGlobal((int)size);
    try
    {
      RegoCode result = (RegoCode)regoNodeJSON(m_ptr, buffer, size);
      if (result != RegoCode.REGO_OK)
      {
        throw new Exception($"Failed to get node JSON: {result}");
      }

      m_json = Marshal.PtrToStringUTF8(buffer, (int)size - 1); ;
      return m_json;
    }
    finally
    {
      Marshal.FreeHGlobal(buffer);
    }
  }

  public Node At(int index)
  {
    Node? child = m_children[index];
    if (child != null)
    {
      return child;
    }

    child = new Node(regoNodeGet(m_ptr, (uint)index));
    m_children[index] = child;
    return child;
  }

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

  // IList Members
  public void Add(Node value)
  {
    throw new NotSupportedException();
  }

  public void Clear()
  {
    throw new NotSupportedException();
  }

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

  public void Insert(int index, Node value)
  {
    throw new NotSupportedException();
  }

  public bool IsReadOnly
  {
    get
    {
      return true;
    }
  }

  public bool Remove(Node value)
  {
    throw new NotSupportedException();
  }

  public void RemoveAt(int index)
  {
    throw new NotSupportedException();
  }

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


  // ICollection members.

  public void CopyTo(Node[] array, int index)
  {
    for (int i = 0; i < Count; i++, index++)
    {
      array[index] = At(i);
    }
  }

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

  // IEnumerable Members

  IEnumerator IEnumerable.GetEnumerator()
  {
    // Refer to the IEnumerator documentation for an example of
    // implementing an enumerator.
    for (int i = 0; i < Count; i++)
    {
      yield return At(i);
    }
  }

  public IEnumerator<Node> GetEnumerator()
  {
    // Refer to the IEnumerator documentation for an example of
    // implementing an enumerator.
    for (int i = 0; i < Count; i++)
    {
      yield return At(i);
    }
  }

  public ICollection<string> Keys
  {
    get
    {
      return m_lookup.Keys;
    }
  }

  public bool ContainsKey(object key)
  {
    var json = JsonSerializer.Serialize(key);
    return m_lookup.ContainsKey(json);
  }

  public bool TryGetValue(object key, out Node? value)
  {
    var json = JsonSerializer.Serialize(key);
    return m_lookup.TryGetValue(json, out value);
  }

  public override string ToString()
  {
    return ToJson();
  }
}