"""regopy - Python wrapper for rego-cpp."""

import os

from .interpreter import Interpreter
from .node import Node, NodeKind
from .output import Output
from .rego_shared import LogLevel, RegoError, rego_version

__version__ = rego_version()

__all__ = [
    "Interpreter", "RegoError", "LogLevel",
    "Output",
    "Node", "NodeKind"
]


def set_log_level(level: LogLevel):
    """Sets the log level.

    Args:
        level (LogLevel): The log level.
    """
    from .rego_shared import rego_set_log_level
    rego_set_log_level(level)


def set_log_level_from_string(level: str):
    """Sets the log level from a string.

    Args:
        level (str): The log level. One of "None",
                     "Error", "Output", "Warn", "Info",
                     "Debug", "Trace".
    """
    from .rego_shared import rego_set_log_level_from_string
    rego_set_log_level_from_string(level)


def build_info() -> str:
    """Returns the build information as a string.

    The string will be in the format
    "{version} ({name}, {date}) {toolchain} on {platform}."

    Returns:
        str: The build information.
    """
    from .rego_shared import rego_build_info
    return rego_build_info()
