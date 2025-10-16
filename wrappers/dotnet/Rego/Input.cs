namespace Rego;

#nullable enable

using System;
using System.Collections;
using System.Runtime.InteropServices;

/// <summary>
/// The Input class allows the creation of inputs to a policy without
/// requiring serialization to JSON. The interface is that of a stack,
/// in which values are pushed and then various operations are used to turn
/// terminal types into more complex ones like objects and arrays. When used,
/// the Input will provide the top of the stack to any downstream consumer (such as
/// <see cref="Interpreter.SetInput"/>). 
/// </summary>
/// <example>
/// <code>
/// Interpreter rego = new();
/// var input = Input.Create(new Dictionary&lt;string, object>{
///     { "a", 10 },
///     { "b", "20" },
///     { "c", 30.0 },
///     { "d", true }
/// });
///
/// rego.SetInput(input);
/// Console.WriteLine(rego.Query("input.a"));
/// // {"expressions":[10]}
/// </code>
/// </example>
public partial class Input
{
  [LibraryImport("rego_shared")]
  private static partial RegoInputHandle regoNewInput();

  [LibraryImport("rego_shared")]
  private static partial uint regoInputInt(RegoInputHandle ptr, long value);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputFloat(RegoInputHandle ptr, double value);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoInputString(RegoInputHandle ptr, string value);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputBoolean(RegoInputHandle ptr, byte value);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputNull(RegoInputHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputObjectItem(RegoInputHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputObject(RegoInputHandle ptr, uint size);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputArray(RegoInputHandle ptr, uint size);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputSet(RegoInputHandle ptr, uint size);

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoInputNode(RegoInputHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoInputSize(RegoInputHandle ptr);

  private readonly RegoInputHandle m_handle;

  /// <summary>
  /// Creates an empty input.
  /// </summary>
  public Input()
  {
    m_handle = regoNewInput();
  }

  /// <summary>
  /// The underlying pointer in the native library.
  /// </summary>
  public RegoInputHandle Handle
  {
    get
    {
      return m_handle;
    }
  }

  /// <summary>
  /// Node object representing the input.
  /// </summary>
  public Node Node
  {
    get
    {
      return new Node(regoInputNode(m_handle));
    }
  }

  /// <summary>
  /// Number of elements in the Input stack.
  /// </summary>
  public uint Size
  {
    get
    {
      return regoInputSize(m_handle);
    }
  }

  /// <summary>
  /// Push an integer onto the stack.
  /// </summary>
  /// <param name="value">value</param>
  /// <returns>A reference to this input</returns>
  public Input Int(long value)
  {
    var result = (RegoCode)regoInputInt(m_handle, value);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Push a float onto the stack.
  /// </summary>
  /// <param name="value">value</param>
  /// <returns>A reference to this input</returns>
  public Input Float(double value)
  {
    var result = (RegoCode)regoInputFloat(m_handle, value);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Push a string onto the stack.
  /// </summary>
  /// <param name="value">value</param>
  /// <returns>A reference to this input</returns>
  public Input String(string value)
  {
    var result = (RegoCode)regoInputString(m_handle, value);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Push a boolean onto the stack.
  /// </summary>
  /// <param name="value">value</param>
  /// <returns>A reference to this input</returns>
  public Input Boolean(bool value)
  {
    var result = (RegoCode)regoInputBoolean(m_handle, (byte)(value ? 1 : 0));
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Push a null onto the stack.
  /// </summary>
  /// <returns>A reference to this input</returns>
  public Input Null()
  {
    var result = (RegoCode)regoInputNull(m_handle);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Take the top two values on the stack and turn them into an object item.
  /// The penultimate value on the stack will be used as
  /// the key, and the top of the stack will be the value for that key.
  /// Objects are constructed from object items.
  /// </summary>
  /// <returns>A reference to this input</returns>
  public Input ObjectItem()
  {
    var result = (RegoCode)regoInputObjectItem(m_handle);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Take the top `size` values on the stack and turn them into an object.
  /// Note that all of these values must be object items in order for this to be valid.
  /// </summary>
  /// <param name="size">Number of object items to use</param>
  /// <returns>A reference to this input</returns>
  public Input Object(uint size)
  {
    var result = (RegoCode)regoInputObject(m_handle, size);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Take the top `size` values on the stack and turn them into an array.
  /// Stack order will be used.
  /// </summary>
  /// <param name="size">Number of values to use</param>
  /// <returns>A reference to this input</returns>
  public Input Array(uint size)
  {
    var result = (RegoCode)regoInputArray(m_handle, size);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  /// <summary>
  /// Take the top `size` values on the stack and turn them into an array.
  /// Identical items will be de-duplicated.
  /// </summary>
  /// <param name="size">Number of values to use</param>
  /// <returns>A reference to this input</returns>
  public Input Set(uint size)
  {
    var result = (RegoCode)regoInputSet(m_handle, size);
    if (result != RegoCode.REGO_OK)
    {
      throw RegoException.FromCode(result);
    }

    return this;
  }

  private static Input addDictionary(Input input, IDictionary dict)
  {
    foreach (System.Collections.DictionaryEntry e in dict)
    {
      add(input, e.Key);
      add(input, e.Value);
      input.ObjectItem();
    }

    return input.Object((uint)dict.Count);
  }

  private static Input addList(Input input, IList list)
  {
    foreach (object e in list)
    {
      add(input, e);
    }

    return input.Array((uint)list.Count);
  }

  private static Input add(Input input, object? value)
  {
    return value switch
    {
      null => input.Null(),
      int i32 => input.Int(i32),
      long i64 => input.Int(i64),
      float f32 => input.Float(f32),
      double f64 => input.Float(f64),
      string str => input.String(str),
      bool b => input.Boolean(b),
      IDictionary dict => addDictionary(input, dict),
      IList list => addList(input, list),
      _ => throw new ArgumentException("Invalid input object type")
    };
  }

  /// <summary>
  /// Factory function which creates Inputs from objects. Supported are any object
  /// that can map to JSON, including scalar types such as:
  /// <list type="bullet">
  ///   <item><term>int</term></item>
  ///   <item><term>long</term></item>
  ///   <item><term>float</term></item>
  ///   <item><term>double</term></item>
  ///   <item><term>string</term></item>
  ///   <item><term>bool</term></item>
  ///   <item><term>IDictionary</term></item>
  ///   <item><term>IList</term></item>
  /// </list>
  /// </summary>
  /// <param name="value">Object to encode as an input</param>
  /// <returns>A reference to the input</returns>
  public static Input Create(object value)
  {
    Input input = new();
    return add(input, value);
  }
}