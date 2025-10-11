namespace Rego;

#nullable enable

using System;
using System.Runtime.InteropServices;

public enum BundleFormat
{
  JSON,
  Binary
}

/// <summary>
/// Encapsulates a Rego bundle, which contains compiled virtual documents and execution plans,
/// the base documents merged into a single JSON hierarchy, and assorted metadata, including the
/// original module files.
/// </summary>
public partial class Bundle
{
  [LibraryImport("rego_shared")]
  private static partial byte regoBundleOk(RegoBundleHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoBundleNode(RegoBundleHandle ptr);

  private readonly RegoBundleHandle m_handle;
  private readonly bool m_ok;
  private readonly Node? m_node;

  internal Bundle(RegoBundleHandle handle)
  {
    m_handle = handle;
    m_ok = regoBundleOk(handle) != 0;
    var node_ptr = regoBundleNode(handle);
    if (node_ptr != IntPtr.Zero)
    {
      m_node = new Node(node_ptr);
    }
  }

  /// <summary>
  /// The underlying pointer in the native library.
  /// </summary>
  public RegoBundleHandle Handle
  {
    get
    {
      return m_handle;
    }
  }

  /// <summary>
  /// Whether the bundle is in a valid state.
  /// </summary>
  public bool Ok
  {
    get
    {
      return m_ok;
    }
  }

  /// <summary>
  /// Node representing the bundle. This is null if
  /// the bundle was loaded from a binary file.
  /// </summary>
  public Node? Node
  {
    get
    {
      return m_node;
    }
  }
}