namespace Rego;

using System;
using System.Runtime.InteropServices;

public enum LogLevel
{
    None = 0,
    Error = 1,
    Output = 2,
    Warn = 3,
    Info = 4,
    Debug = 5,
    Trace = 6
}

public static partial class Logging
{

    [LibraryImport("rego_shared")]
    private static partial uint regoSetLogLevel(uint level);

    [LibraryImport("rego_shared", StringMarshalling = StringMarshalling.Utf8)]
    private static partial uint regoSetLogLevelFromString(string level);

    public static void SetLogLevel(LogLevel level)
    {
        RegoCode result = (RegoCode)regoSetLogLevel((uint)level);
        if (result != RegoCode.REGO_OK)
        {
            throw new ArgumentException($"Failed to set log level: {result}");
        }
    }

    public static void SetLogLevel(string level)
    {
        RegoCode result = (RegoCode)regoSetLogLevelFromString(level);
        if (result != RegoCode.REGO_OK)
        {
            throw new ArgumentException($"Failed to set log level: {result}");
        }
    }
}