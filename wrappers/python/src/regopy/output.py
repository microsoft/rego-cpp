"""Module providing an interface to a rego-cpp output object."""

from ._regopy import (
    regoFreeOutput,
    regoOutputBinding,
    regoOutputNode,
    regoOutputOk,
    regoOutputString,
)
from .node import Node


class Output:
    """Interface for the Rego output.

    Outputs can either be examined as strings, or inspected
    using Rego Nodes. It is also possible to extract bindings
    for specific variables.

    Examples:
        >>> from regopy import Interpreter
        >>> rego = Interpreter()
        >>> output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4")
        >>> print(output)
        x = 5
        y = 9.4

        >>> x = output.binding("x")
        >>> print(x.json())
        5
    """

    def __init__(self, impl):
        self._impl = impl

    def __del__(self):
        regoFreeOutput(self._impl)

    def __str__(self) -> str:
        return regoOutputString(self._impl)

    def node(self) -> Node:
        return Node(regoOutputNode(self._impl))

    def binding(self, name: str) -> Node:
        impl = regoOutputBinding(self._impl, name)
        if impl:
            return Node(impl)

        return None

    def ok(self) -> bool:
        regoOutputOk(self._impl)
