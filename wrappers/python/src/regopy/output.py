"""Module providing an interface to a rego-cpp output object."""

import json
from typing import Union
from .node import Node
from .rego_shared import (
    rego_output_ok,
    rego_output_string,
    rego_output_node,
    rego_output_expressions_at_index,
    rego_output_binding_at_index,
    rego_free_output
)


class Result:
    """A result from a Rego output.

    Each result contains a list of terms, and set of bindings.

    Examples:
        >>> from regopy import Interpreter
        >>> rego = Interpreter()
        >>> output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5")
        >>> result = output[0]
        >>> print(result)
        {"expressions": [true, true, 10], "bindings": {"x": 5, "y": 9.4}}

        >>> print(result["x"])
        5

        >>> print(result[2])
        10
    """

    def __init__(self, obj: dict):
        self.expressions = obj.get("expressions", [])
        self.bindings = obj.get("bindings", {})

    def __str__(self) -> str:
        return json.dumps({"expressions": self.expressions, "bindings": self.bindings})

    def __repr__(self) -> str:
        return f"Result({self.expressions}, {self.bindings})"

    def __getitem__(self, key: Union[str, int]):
        if isinstance(key, int):
            return self.expressions[key]
        else:
            return self.bindings[key]


class Output:
    """Interface for the Rego output.

    Outputs can either be examined as strings, or inspected
    using Rego Nodes. It is also possible to extract bindings
    for specific variables.

    Examples:
        >>> from regopy import Interpreter
        >>> rego = Interpreter()
        >>> output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5")
        >>> print(output)
        {"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}

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
        if rego_output_ok(impl):
            if rego_output_string(impl) == "undefined":
                output = {}
            else:
                output = json.loads(rego_output_string(impl))

            if isinstance(output, list):
                self.results = [Result(obj) for obj in output]
            else:
                self.results = [Result(output)]
        else:
            self.results = []

    def __del__(self):
        """Destructor."""
        rego_free_output(self._impl)

    def __str__(self) -> str:
        """Returns the output as a human readable string.

        If the result of :func:`~regopy.Output.ok` is false, the result will be an string
        containing error information.

        Returns:
            str: A string representation of the output.
        """
        return rego_output_string(self._impl)

    def __repr__(self) -> str:
        """Returns a string representation of the output."""
        return "Output({}@{})".format(rego_output_string(self._impl), self._impl)

    def __len__(self) -> int:
        """Returns the number of results in the output."""
        return len(self.results)

    def __getitem__(self, index: int) -> Result:
        """Returns the result at the given index."""
        return self.results[index]

    def node(self) -> Node:
        """Returns the root node of the output."""
        return Node(rego_output_node(self._impl))

    def expressions(self, index=0) -> Node:
        """Returns the output terms at the given index.

        Args:
            index (int, optional): The index of the term to return.

        Returns:
            Node: A node contains a list of node objects
        """
        return Node(rego_output_expressions_at_index(self._impl, index))

    def binding(self, name: str, index=0) -> Node:
        """Attempts to return the binding for the given variable name.

        Args:
            name (str): The name of the variable to return.
            index (int, optional): The index of the binding to return.

        Returns:
            Node: The binding for the given variable name.

        Raises:
            ValueError: If the variable is not bound.
        """
        impl = rego_output_binding_at_index(self._impl, index, name)
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
        return rego_output_ok(self._impl)
