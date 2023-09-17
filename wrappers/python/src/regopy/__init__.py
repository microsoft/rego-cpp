"""regopy - Python wrapper for rego-cpp."""

from enum import IntEnum

from ._regopy import (
    REGO_LOG_LEVEL_DEBUG,
    REGO_LOG_LEVEL_ERROR,
    REGO_LOG_LEVEL_INFO,
    REGO_LOG_LEVEL_NONE,
    REGO_LOG_LEVEL_TRACE,
    REGO_LOG_LEVEL_WARN,
    REGOCPP_BUILD_DATE,
    REGOCPP_BUILD_NAME,
    REGOCPP_BUILD_TOOLCHAIN,
    REGOCPP_PLATFORM,
    REGOCPP_VERSION,
    regoSetLogLevel
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


class LogLevel(IntEnum):
    NONE = REGO_LOG_LEVEL_NONE
    ERROR = REGO_LOG_LEVEL_ERROR
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
