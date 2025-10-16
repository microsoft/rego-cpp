namespace Rego;

#nullable enable

using System;
using System.Runtime.InteropServices;

public partial class Output
{
  [LibraryImport("rego_shared")]
  private static partial byte regoOutputOk(RegoOutputHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoOutputSize(RegoOutputHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoOutputJSONSize(RegoOutputHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoOutputJSON(RegoOutputHandle ptr, IntPtr buffer, uint size);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial IntPtr regoOutputBindingAtIndex(RegoOutputHandle ptr, uint index, string name);

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoOutputExpressionsAtIndex(RegoOutputHandle ptr, uint index);

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoOutputNode(RegoOutputHandle ptr);

  private readonly RegoOutputHandle m_handle;
  private readonly bool m_ok;
  private readonly int m_size;
  private readonly Node m_node;
  private string? m_string;

  internal Output(RegoOutputHandle handle)
  {
    m_handle = handle;
    m_ok = regoOutputOk(handle) != 0;
    m_size = (int)regoOutputSize(handle);
    m_string = null;
    m_node = new Node(regoOutputNode(handle));
  }

  /// <summary>
  /// The underlying Rego output handle.
  /// </summary>
  public RegoOutputHandle Handle
  {
    get
    {
      return m_handle;
    }
  }

  /// <summary>
  /// Indicates whether the output is valid.
  /// </summary>
  public bool Ok
  {
    get
    {
      return m_ok;
    }
  }

  /// <summary>
  /// The number of results in the output.
  /// </summary>
  public int Count
  {
    get
    {
      return m_size;
    }
  }

  /// <summary>
  /// Returns the binding with the specified name at the specified index.
  /// If the index is out of range, an <see cref="ArgumentOutOfRangeException"/> is thrown.
  /// If the name does not exist, an empty node is returned.
  /// </summary>
  /// <param name="index">Index into the result set</param>
  /// <param name="name">Name of the binding</param>
  /// <returns>The binding node, or an empty node if not found</returns>
  /// <exception cref="ArgumentOutOfRangeException"></exception>
  public Node Binding(int index, string name)
  {
    if (index < 0 || index >= m_size)
    {
      throw new ArgumentOutOfRangeException(nameof(index));
    }

    IntPtr ptr = regoOutputBindingAtIndex(m_handle, (uint)index, name);
    return new Node(ptr);
  }

  /// <summary>
  /// Returns the binding with the specified name at index 0.
  /// If the name does not exist, an empty node is returned.
  /// </summary>
  /// <param name="name">Name of the binding</param>
  /// <returns>The binding node, or an empty node if not found</returns>
  public Node Binding(string name)
  {
    return Binding(0, name);
  }

  /// <summary>
  /// Returns the expressions at the specified index.
  /// If the index is out of range, an <see cref="ArgumentOutOfRangeException"/> is thrown.
  /// The expressions are returned as a node.
  /// If there are no expressions, an empty node is returned.
  /// </summary>
  /// <param name="index">The index into the result set</param>
  /// <returns>The expressions node, or an empty node if not found</returns>
  /// <exception cref="ArgumentOutOfRangeException"></exception>
  public Node Expressions(int index)
  {
    if (index < 0 || index >= m_size)
    {
      throw new ArgumentOutOfRangeException(nameof(index));
    }

    IntPtr ptr = regoOutputExpressionsAtIndex(m_handle, (uint)index);
    return new Node(ptr);
  }

  /// <summary>
  /// Returns the expressions at index 0.
  /// If there are no expressions, an empty node is returned.
  /// </summary>
  /// <returns>The expressions node, or an empty node if not found</returns>
  public Node Expressions()
  {
    return Expressions(0);
  }

  /// <summary>
  /// The root node of the output.
  /// </summary>
  public Node Node
  {
    get
    {
      return m_node;
    }
  }

  /// <summary>
  /// Returns a JSON representation of the output.
  /// </summary>
  /// <returns>The JSON string representation of the output</returns>
  /// <exception cref="RegoException"></exception>
  public override string ToString()
  {
    if (m_string != null)
    {
      return m_string;
    }

    m_string = Interop.GetString(() => regoOutputJSONSize(m_handle), (IntPtr buffer, uint size) => regoOutputJSON(m_handle, buffer, size)) ?? throw new RegoException("Failed to get output JSON");
    return m_string;
  }
}