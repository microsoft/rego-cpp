"""regopy"""

from enum import IntEnum

from ._regopy import (
    REGOCPP_VERSION,
    REGO_LOG_LEVEL_NONE,
    REGO_LOG_LEVEL_ERROR,
    REGO_LOG_LEVEL_WARN,
    REGO_LOG_LEVEL_INFO,
    REGO_LOG_LEVEL_DEBUG,
    REGO_LOG_LEVEL_TRACE,
    regoSetLogLevel
)
from .interpreter import Interpreter, RegoError
from .output import Output
from .node import Node, NodeType

__version__ = REGOCPP_VERSION

__all__ = [
    "Interpreter", "RegoError",
    "Output",
    "Node", "NodeType",
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
