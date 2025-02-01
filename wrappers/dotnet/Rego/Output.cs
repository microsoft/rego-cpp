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

  public RegoOutputHandle Handle
  {
    get
    {
      return m_handle;
    }
  }

  public bool Ok
  {
    get
    {
      return m_ok;
    }
  }

  public int Count
  {
    get
    {
      return m_size;
    }
  }

  public Node Binding(int index, string name)
  {
    if (index < 0 || index >= m_size)
    {
      throw new ArgumentOutOfRangeException(nameof(index));
    }

    IntPtr ptr = regoOutputBindingAtIndex(m_handle, (uint)index, name);
    return new Node(ptr);
  }

  public Node Binding(string name)
  {
    return Binding(0, name);
  }

  public Node Expressions(int index)
  {
    if (index < 0 || index >= m_size)
    {
      throw new ArgumentOutOfRangeException(nameof(index));
    }

    IntPtr ptr = regoOutputExpressionsAtIndex(m_handle, (uint)index);
    return new Node(ptr);
  }

  public Node Expressions()
  {
    return Expressions(0);
  }

  public Node Node
  {
    get
    {
      return m_node;
    }
  }

  public override string ToString()
  {
    if (m_string != null)
    {
      return m_string;
    }

    uint size = regoOutputJSONSize(m_handle);
    if (size == 0)
    {
      m_string = "";
      return m_string;
    }

    IntPtr buffer = Marshal.AllocHGlobal((int)size);
    try
    {
      RegoCode code = (RegoCode)regoOutputJSON(m_handle, buffer, size);
      if (code != RegoCode.REGO_OK)
      {
        throw new Exception($"Failed to get output JSON: {code}");
      }

      m_string = Marshal.PtrToStringUTF8(buffer, (int)size - 1);
      return m_string;
    }
    finally
    {
      Marshal.FreeHGlobal(buffer);
    }
  }
}