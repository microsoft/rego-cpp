"""regopy - Python wrapper for rego-cpp."""

from enum import IntEnum
import os

from ._regopy import (
    REGO_LOG_LEVEL_DEBUG,
    REGO_LOG_LEVEL_ERROR,
    REGO_LOG_LEVEL_OUTPUT,
    REGO_LOG_LEVEL_INFO,
    REGO_LOG_LEVEL_NONE,
    REGO_LOG_LEVEL_TRACE,
    REGO_LOG_LEVEL_WARN,
    REGOCPP_BUILD_DATE,
    REGOCPP_BUILD_NAME,
    REGOCPP_BUILD_TOOLCHAIN,
    REGOCPP_PLATFORM,
    REGOCPP_VERSION,
    regoSetLogLevel,
    regoSetLogLevelFromString,
    regoSetTZDataPath,
)
from .interpreter import Interpreter, RegoError
from .node import Node, NodeKind
from .output import Output

__version__ = REGOCPP_VERSION

__all__ = [
    "Interpreter", "RegoError",
    "Output",
    "Node", "NodeKind",
    "regoNew", "regoFree",
]

regoSetTZDataPath(os.path.join(os.path.basename(__file__), "tzdata"))


def set_tzdata_path(path: str):
    """Sets the path to the tzdata directory.

    Args:
        path (str): The path to the tzdata directory.
    """
    regoSetTZDataPath(path)


class LogLevel(IntEnum):
    NONE = REGO_LOG_LEVEL_NONE
    ERROR = REGO_LOG_LEVEL_ERROR
    OUTPUT = REGO_LOG_LEVEL_OUTPUT
    WARN = REGO_LOG_LEVEL_WARN
    INFO = REGO_LOG_LEVEL_INFO
    DEBUG = REGO_LOG_LEVEL_DEBUG
    TRACE = REGO_LOG_LEVEL_TRACE


def set_log_level(level: LogLevel):
    """Sets the log level.

    Args:
        level (LogLevel): The log level.
    """
    regoSetLogLevel(level)


def set_log_level_from_string(level: str):
    """Sets the log level from a string.

    Args:
        level (str): The log level. One of "None",
                     "Error", "Output", "Warn", "Info",
                     "Debug", "Trace".
    """
    regoSetLogLevelFromString(level)


def build_info() -> str:
    """Returns the build information as a string.

    The string will be in the format
    "{version} ({name}, {date}) {toolchain} on {platform}."

    Returns:
        str: The build information.
    """
    return "{} ({}, {}) {} on {}.".format(
        REGOCPP_VERSION,
        REGOCPP_BUILD_NAME,
        REGOCPP_BUILD_DATE,
        REGOCPP_BUILD_TOOLCHAIN,
        REGOCPP_PLATFORM
    )
