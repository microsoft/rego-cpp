namespace Rego;

using System;
using System.Runtime.InteropServices;

/// <summary>
/// Log levels for Rego logging
/// </summary>
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

/// <summary>
/// Logging utilities for Rego
/// </summary>
public static partial class Logging
{

    [LibraryImport("rego_shared")]
    private static partial uint regoSetDefaultLogLevel(uint level);

    /// <summary>
    /// Sets the default log level for Rego logging. The default log level is <see cref="LogLevel.Output"/>.
    /// </summary>
    /// <param name="level">The log level to set</param>
    /// <exception cref="ArgumentException"></exception>
    public static void SetDefaultLogLevel(LogLevel level)
    {
        RegoCode result = (RegoCode)regoSetDefaultLogLevel((uint)level);
        if (result != RegoCode.REGO_OK)
        {
            throw new ArgumentException($"Failed to set log level: {result}");
        }
    }
}

internal static class Interop
{
    public static string? GetString(Func<uint> getBufferSize, Func<IntPtr, uint, uint> readString)
    {
        uint size = getBufferSize();
        if (size == 0)
        {
            return "";
        }

        IntPtr buffer = Marshal.AllocHGlobal((int)size);
        if (buffer == IntPtr.Zero)
        {
            return null;
        }

        try
        {
            RegoCode result = (RegoCode)readString(buffer, size);
            if (result != RegoCode.REGO_OK)
            {
                return null;
            }

            return Marshal.PtrToStringUTF8(buffer, (int)size - 1);
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }
}