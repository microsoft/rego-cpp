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
        """Initializer.

        The only way to create an output is to query a rego interpreter.
        As such, this initializer should not be called directly.
        """
        self._impl = impl

    def __del__(self):
        """Destructor."""
        regoFreeOutput(self._impl)

    def __str__(self) -> str:
        """Returns the output as a human readable string.

        If the result of :func:`~regopy.Output.ok` is false, the result will be an string
        containing error information.

        Returns:
            str: A string representation of the output.
        """
        return regoOutputString(self._impl)

    def __repr__(self) -> str:
        """Returns a string representation of the output."""
        return "Output({}@{})".format(regoOutputString(self._impl), self._impl)

    def node(self) -> Node:
        """Returns the root node of the output."""
        return Node(regoOutputNode(self._impl))

    def binding(self, name: str) -> Node:
        """Attempts to return the binding for the given variable name.

        Returns:
            Node: The binding for the given variable name.

        Raises:
            ValueError: If the variable is not bound.
        """
        impl = regoOutputBinding(self._impl, name)
        if impl:
            return Node(impl)

        raise ValueError(f"Variable {name} is not bound.")

    def ok(self) -> bool:
        """Returns whether the output is ok.

        The output of a successful query will always be a Node. However, if there
        was an error that arose with the Rego engine, then this Node will be of
        kind `NodeKind.ErrorSeq` and contain one or more errors. This method
        gives a quick way, without inspecting the result node, of finding whether
        it is ok.
        """
        regoOutputOk(self._impl)
