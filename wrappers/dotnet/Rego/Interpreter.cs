namespace Rego;

using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;

internal enum RegoCode
{
  REGO_OK = 0,
  REGO_ERROR = 1,
  REGO_ERROR_BUFFER_TOO_SMALL = 2,
  REGO_ERROR_INVALID_LOG_LEVEL = 3,
  REGO_ERROR_MANUAL_TZDATA_NOT_SUPPORTED = 4
}

public class RegoException(string message) : Exception(message)
{
}

public sealed partial class RegoHandle : SafeHandle
{
  [LibraryImport("rego_shared")]
  private static partial void regoFree(IntPtr ptr);


  public RegoHandle() : base(IntPtr.Zero, true)
  {
  }

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

public sealed partial class RegoOutputHandle : SafeHandle
{
  [LibraryImport("rego_shared")]
  private static partial void regoFreeOutput(IntPtr ptr);

  public RegoOutputHandle() : base(IntPtr.Zero, true)
  {
  }

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

public partial class Interpreter
{
  [LibraryImport("rego_shared")]
  private static partial RegoHandle regoNew();

  [LibraryImport("rego_shared")]
  private static partial RegoHandle regoNewV1();

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial RegoOutputHandle regoQuery(RegoHandle ptr, string query);

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoGetError(RegoHandle ptr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoAddModule(RegoHandle ptr, string name, string source);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoAddDataJSON(RegoHandle ptr, string json);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoSetInputTerm(RegoHandle ptr, string term);

  [LibraryImport("rego_shared")]
  private static partial byte regoGetDebugEnabled(RegoHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial void regoSetDebugEnabled(RegoHandle ptr, byte enabled);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial uint regoSetDebugPath(RegoHandle ptr, string path);

  [LibraryImport("rego_shared")]
  private static partial void regoSetWellFormedChecksEnabled(RegoHandle ptr, byte enabled);

  [LibraryImport("rego_shared")]
  private static partial byte regoGetWellFormedChecksEnabled(RegoHandle ptr);

  [LibraryImport("rego_shared")]
  private static partial void regoSetStrictBuiltInErrors(RegoHandle ptr, byte enabled);

  [LibraryImport("rego_shared")]
  private static partial byte regoGetStrictBuiltInErrors(RegoHandle ptr);

  [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
  private static partial byte regoIsBuiltIn(RegoHandle ptr, string name);

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoVersion();

  [LibraryImport("rego_shared")]
  private static partial IntPtr regoBuildInfo();


  private readonly RegoHandle m_handle;

  public Interpreter(bool v1_compatible)
  {
    if (v1_compatible)
      m_handle = regoNewV1();
    else
      m_handle = regoNew();
  }

  public Interpreter() : this(false)
  {
  }

  private string getError()
  {
    return Marshal.PtrToStringUTF8(regoGetError(m_handle))!;
  }

  public static string Version
  {
    get
    {
      string version = Marshal.PtrToStringUTF8(regoVersion())!;
      return version;
    }
  }

  public static string BuildInfo
  {
    get
    {
      string buildInfo = Marshal.PtrToStringUTF8(regoBuildInfo())!;
      return buildInfo;
    }
  }

  public RegoHandle Handle
  {
    get
    {
      return m_handle;
    }
  }

  public void AddModule(string name, string source)
  {
    var result = (RegoCode)regoAddModule(m_handle, name, source);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  public void AddDataJson(string json)
  {
    var result = (RegoCode)regoAddDataJSON(m_handle, json);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  public void AddData<T>(T data)
  {
    AddDataJson(JsonSerializer.Serialize(data));
  }

  public void SetInputTerm(string term)
  {
    var result = (RegoCode)regoSetInputTerm(m_handle, term);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

  public void SetInput<T>(T data)
  {
    SetInputTerm(JsonSerializer.Serialize(data));
  }

  public Output Query(string query)
  {
    var ptr = regoQuery(m_handle, query);
    if (ptr.IsInvalid)
    {
      throw new RegoException(getError());
    }

    return new Output(ptr);
  }

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

  public void SetDebugPath(string path)
  {
    var result = (RegoCode)regoSetDebugPath(m_handle, path);
    if (result != RegoCode.REGO_OK)
    {
      throw new RegoException(getError());
    }
  }

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

  public bool IsBuiltIn(string name)
  {
    return regoIsBuiltIn(m_handle, name) != 0;
  }
}
