namespace Rego;

using System;
using System.IO.Pipes;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;

/// <summary>
/// Rego codes returned from the native library
/// </summary>
internal enum RegoCode
{
  REGO_OK = 0,
  REGO_ERROR = 1,
  REGO_ERROR_BUFFER_TOO_SMALL = 2,
  REGO_ERROR_INVALID_LOG_LEVEL = 3,
  // REGO_ERROR_MANUAL_TZDATA_NOT_SUPPORTED 4 deprecated
  REGO_ERROR_INPUT_NULL = 5,
  REGO_ERROR_INPUT_MISSING_ARGUMENTS = 6,
  REGO_ERROR_INPUT_OBJECT_ITEM = 7
}

/// <summary>
/// Exception thrown for errors returned from the Rego library
/// </summary>
public class RegoException(string message) : Exception(message)
{
  internal static RegoException FromCode(RegoCode code)
  {
    return code switch
    {
      RegoCode.REGO_ERROR_BUFFER_TOO_SMALL => new RegoException("Provided buffer is not large enough for the data."),
      RegoCode.REGO_ERROR_INVALID_LOG_LEVEL => new RegoException("Log level provided is not valid"),
      RegoCode.REGO_ERROR_INPUT_NULL => new RegoException("Input object has not been initialized"),
      RegoCode.REGO_ERROR_INPUT_MISSING_ARGUMENTS => new RegoException("Input stack does not contain enough arguments for the command"),
      RegoCode.REGO_ERROR_INPUT_OBJECT_ITEM => new RegoException("Objects must be built from object items"),
      _ => throw new ArgumentException("Cannot produce automatic message from error code: " + code),
    };
  }
}

/// <summary>
/// Handle wrapping the pointer to the native Rego interpreter object
/// </summary>
public sealed partial class RegoHandle : SafeHandle
{
  [LibraryImport("rego_shared")]
  private static partial void regoFree(IntPtr ptr);


  /// <summary>
  /// Creates a new invalid handle
  /// </summary>
  public RegoHandle() : base(IntPtr.Zero, true)
  {
  }

  /// <summary>
  /// Indicates whether the handle is invalid
  /// </summary>
  public override bool IsInvalid
  {
    get
    {
      return handle == IntPtr.Zero;
    }
  }

  protected override bool ReleaseHandle()
  {
    regoFree(handle);
    return true;
  }
}

/// <summary>
/// Handle wrapping the pointer to the native Rego output object
/// </summary>
public sealed partial class RegoOutputHandle : SafeHandle
{
  [LibraryImport("rego_shared")]
  private static partial void regoFreeOutput(IntPtr ptr);

  /// <summary>
  /// Creates a new invalid handle
  /// </summary>
  public RegoOutputHandle() : base(IntPtr.Zero, true)
  {
  }

  /// <summary>
  /// Indicates whether the handle is invalid
  /// </summary>
  public override bool IsInvalid
  {
    get
    {
      return handle == IntPtr.Zero;
    }
  }

  protected override bool ReleaseHandle()
  {
    regoFreeOutput(handle);
    return true;
  }
}

/// <summary>
/// Handle wrapping the pointer to the native Rego input object
/// </summary>
public sealed partial class RegoInputHandle : SafeHandle
{
  [LibraryImport("rego_shared")]
  private static partial void regoFreeInput(IntPtr ptr);

  /// <summary>
  /// Creates a new invalid handle
  /// </summary>
  public RegoInputHandle() : base(IntPtr.Zero, true)
  {
  }

  /// <summary>
  /// Indicates whether the handle is invalid
  /// </summary>
  public override bool IsInvalid
  {
    get
    {
      return handle == IntPtr.Zero;
    }
  }

  protected override bool ReleaseHandle()
  {
    regoFreeInput(handle);
    return true;
  }
}

/// <summary>
/// Handle wrapping the pointer to the native Rego bundle object
/// </summary>
public sealed partial class RegoBundleHandle : SafeHandle
{
  [LibraryImport("rego_shared")]
  private static partial void regoFreeBundle(IntPtr ptr);

  /// <summary>
  /// Creates a new invalid handle
  /// </summary>
  public RegoBundleHandle() : base(IntPtr.Zero, true)
  {
  }

  /// <summary>
  /// Indicates whether the handle is invalid
  /// </summary>
  public override bool IsInvalid
  {
    get
    {
      return handle == IntPtr.Zero;
    }
  }

  protected override bool ReleaseHandle()
  {
    regoFreeBundle(handle);
    return true;
  }
}

/// <summary>
/// Rego interpreter instance. This class provides methods to load modules, data, and input,
/// execute queries, and manage bundles.
/// </summary>
/// <example>
/// You can use a query alone, without input or data:
/// <code>
/// using Rego;
/// Interpreter rego = new();
/// Console.WriteLine(rego.Query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5"));
/// // {"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}
/// </code>
/// </example>
/// <example>
/// More typically, a policy will involve base documents (static data) and virtual documents (Rego modules):
/// <code>
/// using System.Collections;
/// using Rego;
/// Interpreter rego = new();
/// var data0 = new Dictionary&lt;string, object>{
///    {"one", new Dictionary&lt;string, object>{
///        {"bar", "Foo"},
///        {"baz", 5},
///        {"be", true},
///        {"bop", 23.4}}},
///    {"two", new Dictionary&lt;string, object>{
///        {"bar", "Bar"},
///        {"baz", 12.3},
///        {"be", false},
///        {"bop", 42}}}};
/// rego.AddData(data0);
///
/// rego.AddDataJson("""
///    {
///        "three": {
///            "bar": "Baz",
///            "baz": 15,
///            "be": true,
///            "bop": 4.23
///        }
///    }
///    """);
///
/// var objectsSource = """
///    package objects
///
///    rect := {"width": 2, "height": 4}
///    cube := {"width": 3, "height": 4, "depth": 5}
///    a := 42
///    b := false
///    c := null
///    d := {"a": a, "x": [b, c]}
///    index := 1
///    shapes := [rect, cube]
///    names := ["prod", "smoke1", "dev"]
///    sites := [{"name": "prod"}, {"name": names[index]}, {"name": "dev"}]
///    e := {
///        a: "foo",
///        "three": c,
///        names[2]: b,
///        "four": d,
///    }
///    f := e["dev"]
///    """;
/// rego.AddModule("objects.rego", objectsSource);
///
/// rego.SetInputTerm("""
///    {
///        "a": 10,
///        "b": "20",
///        "c": 30.0,
///        "d": true
///    }
///    """);
///
/// Console.WriteLine(rego.Query("[data.one, input.b, data.objects.sites[1]] = x"));
/// // {"bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]}}
/// </code>
/// </example>
/// <example>
/// When the same policy is going to be queried multiple times with different inputs, 
/// it can be more efficient to build a bundle.
/// <code>
/// using System.Collections;
/// using Rego;
/// Interpreter rego_build = new();
/// rego_build.AddDataJson("""
/// {"a": 7,
/// "b": 13}
/// """);
///
/// rego_build.AddModule("example.rego", """
///     package example
///
///    foo := data.a * input.x + data.b * input.y
///    bar := data.b * input.x + data.a * input.y
/// """);
///
/// var bundle = rego_build.Build("x=data.example.foo + data.example.bar", ["example/foo", "example/bar"]);
///
/// rego_build.SaveBundle("bundle", bundle);
/// Interpreter rego_run = new();
/// rego_run.LoadBundle("bundle");
///
/// var input = Input.Create(new Dictionary&lt;string, int>
/// {
///     {"x", 104 },
///     {"y", 119 }
/// });
///
/// rego_run.SetInput(input);
///
/// Console.WriteLine(rego_run.QueryBundle(bundle));
/// // {"expressions":[true], "bindings":{"x":4460}}
/// Console.WriteLine(rego_run.QueryBundle(bundle, "example/foo"));
/// // {"expressions":[2275]}
/// </code>
/// </example>
public partial class Interpreter
{
  [LibraryImport("rego_shared")]
  private static partial RegoHandle regoNew();

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial RegoOutputHandle regoQuery(RegoHandle ptr, string query);

  [LibraryImport("rego_shared")]
  private static partial uint regoErrorSize(RegoHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoError(RegoHandle ptr, IntPtr buffer, uint size);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoAddModule(RegoHandle ptr, string name, string source);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoAddDataJSON(RegoHandle ptr, string json);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoSetInputTerm(RegoHandle ptr, string term);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoSetInput(RegoHandle ptr, RegoInputHandle input_ptr);

  [LibraryImport("rego_shared")]
  private static partial uint regoGetDebugEnabled(RegoHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial void regoSetDebugEnabled(RegoHandle ptr, byte enabled);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoSetDebugPath(RegoHandle ptr, string path);

  [LibraryImport("rego_shared")]
  private static partial void regoSetWellFormedChecksEnabled(RegoHandle ptr, byte enabled);

  [LibraryImport("rego_shared")]
  private static partial uint regoGetWellFormedChecksEnabled(RegoHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial void regoSetStrictBuiltInErrors(RegoHandle ptr, byte enabled);

  [LibraryImport("rego_shared")]
  private static partial uint regoGetStrictBuiltInErrors(RegoHandle ptr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial byte regoIsAvailableBuiltIn(RegoHandle ptr, string name);

  [LibraryImport("rego_shared")]
  private static partial uint regoVersionSize();

  [LibraryImport("rego_shared")]
  private static partial uint regoVersion(IntPtr buffer, uint size);

  [LibraryImport("rego_shared")]
  private static partial uint regoBuildInfoSize();

  [LibraryImport("rego_shared")]
  private static partial uint regoBuildInfo(IntPtr buffer, uint size);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoSetQuery(RegoHandle ptr, string query_expr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoAddEntrypoint(RegoHandle ptr, string entrypoint);

  [LibraryImport("rego_shared")]
  private static partial RegoBundleHandle regoBuild(RegoHandle ptr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial RegoBundleHandle regoBundleLoad(RegoHandle ptr, string dir);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial RegoBundleHandle regoBundleLoadBinary(RegoHandle ptr, string path);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoBundleSave(RegoHandle ptr, string dir, RegoBundleHandle bundle_ptr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoBundleSaveBinary(RegoHandle ptr, string path, RegoBundleHandle bundle_ptr);

  [LibraryImport("rego_shared")]
  private static partial RegoOutputHandle regoBundleQuery(RegoHandle ptr, RegoBundleHandle bundle_ptr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial RegoOutputHandle regoBundleQueryEntrypoint(RegoHandle ptr, RegoBundleHandle bundle_ptr, string endpoint);

  [LibraryImport("rego_shared")]
  private static partial uint regoSetLogLevel(RegoHandle ptr, uint level);

  [LibraryImport("rego_shared")]
  private static partial uint regoGetLogLevel(RegoHandle ptr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoLogLevelFromString(string level);

  private readonly RegoHandle m_handle;

  /// <summary>
  /// Creates a new Rego interpreter instance
  /// </summary>
  public Interpreter()
  {
    m_handle = regoNew();
  }

  private string getError()
  {
    return Interop.GetString(() => regoErrorSize(m_handle), (IntPtr buffer, uint size) => regoError(m_handle, buffer, size)) ?? throw new RegoException("Unknown error obtaining error message");
  }

  /// <summary>
  /// Returns the version of the underlying Rego library
  /// </summary>
  public static string Version
  {
    get
    {
      string version = Interop.GetString(regoVersionSize, regoVersion) ?? throw new RegoException("Unable to obtain version");
      return version;
    }
  }

  /// <summary>
  /// Returns build information about the underlying Rego library
  /// </summary>
  public static string BuildInfo
  {
    get
    {
      string buildInfo = Interop.GetString(regoBuildInfoSize, regoBuildInfo) ?? throw new RegoException("Unable to obtain build info");
      return buildInfo;
    }
  }

  /// <summary>
  /// Returns the handle to the underlying Rego interpreter instance
  /// </summary>
  public RegoHandle Handle
  {
    get
    {
      return m_handle;
    }
  }

  /// <summary>
  /// Adds a new Rego module to the interpreter
  /// </summary>
  /// <param name="name">The name of the module</param>
  /// <param name="source">The source code of the module</param>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// using Rego;
  /// Interpreter rego = new();
  /// var source = """
  ///    package scalars
  /// 
  ///    greeting := "Hello"
  ///    max_height := 42
  ///    pi := 3.14159
  ///    allowed := true
  ///    location := null
  /// """;
  /// rego.AddModule("scalars.rego", source);
  /// Console.WriteLine(rego.Query("data.scalars.greeting"));
  /// // {"expressions":["Hello"]}
  /// </code>
  /// </example>
  public void AddModule(string name, string source)
  {
    var result = (RegoCode)regoAddModule(m_handle, name, source);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  /// <summary>
  /// Adds a base document in JSON format.
  /// </summary>
  /// <param name="json">JSON representation of the document</param>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// Interpreter rego = new();
  /// var data = """
  ///   {
  ///     "one": {
  ///       "bar": "Foo",
  ///       "baz": 5,
  ///       "be": true,
  ///       "bop": 23.4
  ///     },
  ///     "two": {
  ///       "bar": "Bar",
  ///       "baz": 12.3,
  ///       "be": false,
  ///       "bop": 42
  ///     }
  ///   }
  /// """;
  /// rego.AddDataJson(data);
  /// Console.WriteLine(rego.Query("data.one.bar"));
  /// // {"expressions":["Foo"]}
  /// </code>
  /// </example>
  public void AddDataJson(string json)
  {
    var result = (RegoCode)regoAddDataJSON(m_handle, json);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  /// <summary>
  /// Adds a base document by serializing the provided object to JSON.
  /// </summary>
  /// <typeparam name="T"></typeparam>
  /// <param name="data">The data to serialize</param>
  /// <exception cref="RegoException"></exception>
  public void AddData<T>(T data)
  {
    AddDataJson(JsonSerializer.Serialize(data));
  }

  /// <summary>
  /// Sets the input document from a Rego term.
  /// </summary>
  /// <param name="term">The Rego term</param>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// Interpreter rego = new();
  /// var input = """
  ///     {
  ///         "a": 10,
  ///         "b": "20",
  ///         "c": 30.0,
  ///         "d": true
  ///     }
  ///     """;
  /// rego.SetInputTerm(input);
  /// Console.WriteLine(rego.Query("input.a"));
  /// // {"expressions":[10]}
  /// </code>
  /// </example>
  public void SetInputTerm(string term)
  {
    var result = (RegoCode)regoSetInputTerm(m_handle, term);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  /// <summary>
  /// Sets the input document by serializing the provided object to JSON.
  /// </summary>
  /// <typeparam name="T"></typeparam>
  /// <param name="data">The data to serialize</param>
  /// <exception cref="RegoException"></exception>
  public void SetInput<T>(T data)
  {
    SetInputTerm(JsonSerializer.Serialize(data));
  }

  /// <summary>
  /// Sets the input document from an Input object.
  /// </summary>
  /// <param name="input">The input object</param>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// Interpreter rego = new();
  /// var input = Input.Create(new Dictionary&lt;string, int>
  /// {
  ///     {"x", 104 },
  ///     {"y", 119 }
  /// });
  /// rego.SetInput(input);
  /// Console.WriteLine(rego.Query("x=input.x"));
  /// // {"expressions":[true], "bindings":{"x":104}}
  /// </code>
  /// </example>
  public void SetInput(Input input)
  {
    var result = (RegoCode)regoSetInput(m_handle, input.Handle);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  /// <summary>
  /// Executes a query against the loaded modules, data, and input.
  /// </summary>
  /// <param name="query">Rego query string</param>
  /// <returns>The query output</returns>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// Interpreter rego = new();
  /// var input0 = new Dictionary&lt;string, int> { { "a", 10 } };
  /// var input1 = new Dictionary&lt;string, int> { { "a", 4 } };
  /// var input2 = new Dictionary&lt;string, int> { { "a", 7 } };
  /// var multi = """
  ///     package multi
  ///
  ///     default a := 0
  ///
  ///     a := val {
  ///         input.a > 0
  ///         input.a &lt; 10
  ///         input.a % 2 == 1
  ///         val := input.a * 10
  ///     } {
  ///         input.a > 0
  ///         input.a &lt; 10
  ///         input.a % 2 == 0
  ///         val := input.a * 10 + 1
  ///     }
  ///
  ///     a := input.a / 10 {
  ///         input.a >= 10
  ///     }
  ///     """;
  /// rego.AddModule("multi.rego", multi);
  /// 
  /// rego.SetInput(input0);
  /// Console.WriteLine(rego.Query("data.multi.a"));
  /// // {"expressions":[1]}
  /// 
  /// rego.SetInput(input1);
  /// Console.WriteLine(rego.Query("data.multi.a"));
  /// // {"expressions":[41]}
  /// 
  /// rego.SetInput(input2);
  /// Console.WriteLine(rego.Query("data.multi.a"));
  /// // {"expressions":[70]}
  /// </code>
  /// </example>
  public Output Query(string query)
  {
    var ptr = regoQuery(m_handle, query);
    if (ptr.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Output(ptr);
  }

  /// <summary>
  /// Gets or sets whether debug logging of intermediate ASTs is enabled.
  /// </summary>
  public bool DebugEnabled
  {
    get
    {
      return regoGetDebugEnabled(m_handle) != 0;
    }
    set
    {
      regoSetDebugEnabled(m_handle, (byte)(value ? 1 : 0));
    }
  }

  /// <summary>
  /// Sets the root of the directory tree where debug ASTs will be written.
  /// It will be created if it does not exist.
  /// </summary>
  /// <param name="path">Path to a directory</param>
  /// <exception cref="RegoException"></exception>
  public void SetDebugPath(string path)
  {
    var result = (RegoCode)regoSetDebugPath(m_handle, path);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  /// <summary>
  /// Gets or sets whether well-formedness checks are enabled.
  /// </summary>
  public bool WellFormedChecksEnabled
  {
    get
    {
      return regoGetWellFormedChecksEnabled(m_handle) != 0;
    }
    set
    {
      regoSetWellFormedChecksEnabled(m_handle, (byte)(value ? 1 : 0));
    }
  }

  /// <summary>
  /// Gets or sets whether built-in functions will raise an error on invalid input.
  /// </summary>
  public bool StrictBuiltInErrors
  {
    get
    {
      return regoGetStrictBuiltInErrors(m_handle) != 0;
    }
    set
    {
      regoSetStrictBuiltInErrors(m_handle, (byte)(value ? 1 : 0));
    }
  }

  /// <summary>
  /// Gets or sets the log level for the interpreter.
  /// </summary>
  public LogLevel LogLevel
  {
    get
    {
      return (LogLevel)regoGetLogLevel(m_handle);
    }

    set
    {
      regoSetLogLevel(m_handle, (byte)value);
    }
  }

  /// <summary>
  /// Returns whether the specified string corresponds to a known built-in function.
  /// </summary>
  /// <param name="name">The name of the built-in function</param>
  /// <returns>True if the function is built-in, false otherwise</returns>
  public bool IsAvailableBuiltIn(string name)
  {
    return regoIsAvailableBuiltIn(m_handle, name) != 0;
  }

  /// <summary>
  /// Builds a bundle from the loaded modules and data, setting the specified query as the bundle's entrypoint.
  /// </summary>
  /// <param name="query">The Rego query string</param>
  /// <returns>The created bundle</returns>
  /// <exception cref="RegoException"></exception>
  public Bundle Build(string query)
  {
    var result = (RegoCode)regoSetQuery(m_handle, query);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }

    var bundle = regoBuild(m_handle);
    if (bundle.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Bundle(bundle);
  }

  /// <summary>
  /// Builds a bundle from the loaded modules and data, setting the specified entrypoints. Entrypoints
  /// are of the format [path/to/rule].
  /// </summary>
  /// <param name="entrypoints">The Rego entrypoints</param>
  /// <returns>The created bundle</returns>
  /// <exception cref="RegoException"></exception>
  public Bundle Build(string[] entrypoints)
  {
    foreach (var entrypoint in entrypoints)
    {
      var result = (RegoCode)regoAddEntrypoint(m_handle, entrypoint);
      if (result != RegoCode.REGO_OK)
      {
        throw new RegoException(getError());
      }
    }

    var bundle = regoBuild(m_handle);
    if (bundle.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Bundle(bundle);
  }

  /// <summary>
  /// Builds a bundle from the loaded modules and data, setting the specified query and entrypoints
  /// as the bundle's entrypoints. Entrypoints are of the format [path/to/rule].
  /// </summary>
  /// <param> name="query">The Rego query string</param>
  /// <param name="entrypoints">The Rego entrypoints</param>
  /// <returns>The created bundle</returns>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// var input = new Dictionary&lt;string, int>{
  ///     {"x", 104 },
  ///     {"y", 119 }};
  /// var data = new Dictionary&lt;string, int>{
  ///     {"a", 7},
  ///     {"b", 13}};
  /// var module = """
  ///     package example
  ///     foo := data.a * input.x + data.b * input.y
  ///     bar := data.b * input.x + data.a * input.y
  /// """;
  ///
  /// Interpreter rego_build = new();
  /// rego_build.AddData(data);
  /// rego_build.AddModule("example.rego", module);
  /// var bundle = rego_build.Build("x=data.example.foo + data.example.bar", ["example/foo", "example/bar"]);
  ///
  /// Interpreter rego_run = new();
  /// rego_run.SetInput(input);
  /// Console.WriteLine(rego_run.QueryBundle(bundle));
  /// // {"expressions":[true], "bindings":{"x":4460}}
  ///
  /// Console.WriteLine(rego_run.QueryBundle(bundle, "example/foo"));
  /// // {"expressions":[2275]}
  /// </code>
  /// </example>
  public Bundle Build(string query, string[] entrypoints)
  {
    var result = (RegoCode)regoSetQuery(m_handle, query);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }

    foreach (var entrypoint in entrypoints)
    {
      result = (RegoCode)regoAddEntrypoint(m_handle, entrypoint);
      if (result != RegoCode.REGO_OK)
      {
        throw new RegoException(getError());
      }
    }

    var ptr = regoBuild(m_handle);
    if (ptr.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Bundle(ptr);
  }

  /// <summary>
  /// Loads a bundle from the specified path. The path should point to a directory
  /// containing a bundle in JSON format, or to a file containing a bundle in binary format
  /// </summary>
  /// <param name="path">The path to the bundle</param>
  /// <param name="format">The format of the bundle</param>
  /// <returns>The loaded bundle</returns>
  /// <exception cref="ArgumentException"></exception>
  /// <exception cref="RegoException"></exception>
  public Bundle LoadBundle(string path, BundleFormat format = BundleFormat.JSON)
  {
    RegoBundleHandle ptr;
    switch (format)
    {
      case BundleFormat.JSON:
        ptr = regoBundleLoad(m_handle, path);
        break;

      case BundleFormat.Binary:
        ptr = regoBundleLoadBinary(m_handle, path);
        break;

      default:
        throw new ArgumentException("Unsupported bundle format");
    }

    if (ptr.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Bundle(ptr);
  }

  /// <summary>
  /// Saves the specified bundle to the specified path. The path should point to a directory
  /// where the bundle will be saved in JSON format, or to a file where the bundle will be saved
  /// in binary format.
  /// </summary>
  /// <param name="path">The path to the bundle</param>
  /// <param name="bundle">The bundle to save</param>
  /// <param name="format">The format of the bundle</param>
  /// <exception cref="ArgumentException"></exception>
  /// <exception cref="RegoException"></exception>
  /// <example>
  /// <code>
  /// Interpreter rego_build = new();
  /// var bundle = rego_build.Build("a=1");
  /// rego_build.SaveBundle("bundle_doctest1", bundle);
  /// rego_build.SaveBundle("bundle_bin_doctest1.rbb", bundle, BundleFormat.Binary);
  /// 
  /// Interpreter rego_run = new();
  /// bundle = rego_run.LoadBundle("bundle_doctest1");
  /// Console.WriteLine(rego_run.QueryBundle(bundle));
  /// // {"expressions":[true], "bindings":{"a":1}}
  /// 
  /// bundle = rego_run.LoadBundle("bundle_bin_doctest1.rbb", BundleFormat.Binary);
  /// Console.WriteLine(rego_run.QueryBundle(bundle));
  /// // {"expressions":[true], "bindings":{"a":1}}
  /// </code>
  /// </example>
  public void SaveBundle(string path, Bundle bundle, BundleFormat format = BundleFormat.JSON)
  {
    RegoCode result;
    switch (format)
    {
      case BundleFormat.JSON:
        result = (RegoCode)regoBundleSave(m_handle, path, bundle.Handle);
        break;

      case BundleFormat.Binary:
        result = (RegoCode)regoBundleSaveBinary(m_handle, path, bundle.Handle);
        break;

      default:
        throw new ArgumentException("Unsupported bundle format");
    }

    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  /// <summary>
  /// Executes a query against the specified bundle. This requires that <see cref="Build"/> 
  /// was provided with a query when this bundle was built.
  /// </summary>
  /// <param name="bundle">The bundle to query</param>
  /// <returns>The query output</returns>
  /// <exception cref="RegoException"></exception>
  public Output QueryBundle(Bundle bundle)
  {
    RegoOutputHandle ptr = regoBundleQuery(m_handle, bundle.Handle);

    if (ptr.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Output(ptr);
  }

  /// <summary>
  /// Executes a query against the specified bundle, using the specified entrypoint. This requires that <see cref="Build"/> 
  /// was provided with the specified entrypoint when this bundle was built.
  /// </summary>
  /// <param name="bundle">The bundle to query</param>
  /// <param name="entrypoint">The entrypoint to use for the query</param>
  /// <returns>The query output</returns>
  /// <exception cref="RegoException"></exception>
  public Output QueryBundle(Bundle bundle, string entrypoint)
  {
    RegoOutputHandle ptr = regoBundleQueryEntrypoint(m_handle, bundle.Handle, entrypoint);

    if (ptr.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Output(ptr);
  }
}
