"""regopy - Python wrapper for rego-cpp."""

from .interpreter import Bundle, BundleFormat, Input, Interpreter
from .node import Node, NodeKind
from .output import Output
from .rego_shared import LogLevel, RegoError, rego_version

__version__ = rego_version()

__all__ = [
    "Bundle", "BundleFormat", "Input", "Interpreter", "RegoError", "LogLevel",
    "Output",
    "Node", "NodeKind"
]


def log_level_from_string(level: str):
    """Sets the log level from a string.

    Args:
        level (str): The log level. One of "None",
                     "Error", "Output", "Warn", "Info",
                     "Debug", "Trace".
    """
    from .rego_shared import rego_log_level_from_string
    return rego_log_level_from_string(level)


def set_default_log_level(level: str):
    """Sets the default log level from a string.

    Args:
        level (str): The log level. One of "None",
                     "Error", "Output", "Warn", "Info",
                     "Debug", "Trace".
    """
    from .rego_shared import rego_set_default_log_level
    log_level = log_level_from_string(level)
    return rego_set_default_log_level(log_level)


def build_info() -> str:
    """Returns the build information as a string.

    The string will be in the format
    "{version} ({name}, {date}) {toolchain} on {platform}."

    Returns:
        str: The build information.
    """
    from .rego_shared import rego_build_info
    return rego_build_info()
