"""Module providing an interface to the rego-cpp Node object."""

import json
from typing import Any, Optional, Union

from .rego_shared import (
    NodeKind,
    rego_node_type,
    rego_node_type_name,
    rego_node_value,
    rego_node_size,
    rego_node_get,
    rego_node_json
)


class Node:
    """Interface for a Rego Node.

    Rego Nodes are the basic building blocks of a Rego result. They
    exist in a tree structure. Each node has a kind, which is one of
    the variants of [`NodeKind`]. Each node also has zero or more
    children, which are also nodes.

    Examples:
    >>> from regopy import Interpreter
    >>> rego = Interpreter()
    >>> output = rego.query('x={"a": 10, "b": "20", "c": [30.0, 60], "d": true, "e": null}')
    >>> x = output.binding("x")
    >>> print("x =", x.json())
    x = {"a":10, "b":"20", "c":[30,60], "d":true, "e":null}

    >>> print("x['a'] =", x["a"].value)
    x['a'] = 10

    >>> print("x['b'] =", '"' + x["b"].value + '"')
    x['b'] = "20"

    >>> print("x['c'][0] =", x["c"][0].value)
    x['c'][0] = 30.0

    >>> print("x['c'][1] =", x["c"][1].value)
    x['c'][1] = 60

    >>> print("x['d'] =", x["d"].value)
    x['d'] = True

    >>> print("x['e'] =", x["e"].value)
    x['e'] = None
    """

    ValueKinds = [NodeKind.Int, NodeKind.Float, NodeKind.String, NodeKind.Boolean, NodeKind.Null]

    def __init__(self, impl):
        """Creates a new node."""
        self._impl = impl
        self._kind = rego_node_type(self._impl)

        size = rego_node_size(self._impl)
        if self._kind == NodeKind.Object:
            self._children = {}
            for i in range(size):
                child = Node(rego_node_get(self._impl, i))
                assert child._kind == NodeKind.ObjectItem
                key = rego_node_json(child.at(0)._impl)
                self._children[key] = child.at(1)
        elif self._kind == NodeKind.Set:
            self._children = {}
            for i in range(size):
                child = Node(rego_node_get(self._impl, i))
                key = rego_node_json(child._impl)
                self._children[key] = child
        else:
            self._children = [None] * size

    @property
    def kind(self) -> NodeKind:
        """Returns the node type."""
        return self._kind

    @property
    def kind_name(self) -> str:
        """Returns a human-readable string representation of the node kind."""
        return rego_node_type_name(self._impl)

    @property
    def value(self) -> Union[str, int, float, bool, None]:
        """Returns the node value.

        If the node has a singular value (i.e is a `Scalar` or a `Term` containing a scalar)
        then this will return that value. Otherwise it will raise a RegoError.

        Returns:
            Union[str, int, float, bool, None]: The node value.

        Raises:
            RegoError: If the node does not have a singular value.

        Examples:
            >>> from regopy import Interpreter
            >>> rego = Interpreter()
            >>> output = rego.query('x=10; y="20"; z=true')
            >>> x = output.binding("x")
            >>> print("x =", x.value)
            x = 10

            >>> y = output.binding("y")
            >>> print("y =", y.value)
            y = "20"

            >>> z = output.binding("z")
            >>> print("z =", z.value)
            z = True
        """
        if self._kind == NodeKind.Term or self._kind == NodeKind.Scalar:
            return self.at(0).value

        if self._kind not in Node.ValueKinds:
            raise ValueError("Node does not have a value")

        if hasattr(self, "_value"):
            return self._value

        value = rego_node_value(self._impl)
        if self._kind == NodeKind.Int:
            self._value = int(value)
        elif self._kind == NodeKind.Float:
            self._value = float(value)
        elif self._kind == NodeKind.Boolean:
            self._value = value == "true"
        elif self._kind == NodeKind.Null:
            self._value = None
        elif self._kind == NodeKind.String:
            self._value = value
        else:
            raise ValueError("Node does not have a value")

        return self._value

    def __len__(self) -> int:
        """Returns the number of child nodes."""
        if self._kind == NodeKind.Term:
            return len(self.at(0))

        return len(self._children)

    def index(self, index: int) -> "Node":
        """Returns the node at an index of an array.

        Returns:
            Node: The child node.

        Raises:
            IndexError: If the index is out of bounds.
            TypeError: If the node is not an array.
        """
        if self._kind == NodeKind.Term:
            return self.at(0).index(index)

        if self._kind in [NodeKind.Array, NodeKind.Terms, NodeKind.Results]:
            if index < 0 or index >= len(self):
                raise IndexError("index out of bounds")

            return self.at(index)

        raise TypeError("index is only valid for Array nodes")

    def lookup(self, key: Any) -> Optional["Node"]:
        """Returns the child node for the given key.

        If this is of kind Object or Set, then the key must be a string.

        Returns:
            Node: The child node.

        Raises:
            KeyError: If the key is not found.
            TypeError: If the node does not support lookup
        """
        if self._kind == NodeKind.Term:
            return self.at(0).lookup(key)

        key = json.dumps(key)
        if self._kind == NodeKind.Object:
            if key in self._children:
                return self._children[key]

            key = '"' + key + '"'
            if key in self._children:
                return self._children[key]

            raise KeyError(f"Key {key} not found")

        if self._kind == NodeKind.Set:
            if key in self._children:
                return self._children[key]

            return None

        raise TypeError("lookup is only valid for OBJECT and SET nodes")

    def at(self, index: int) -> "Node":
        """Returns the child node at the given index."""
        if self._children[index] is None:
            self._children[index] = Node(rego_node_get(self._impl, index))

        return self._children[index]

    def __iter__(self):
        """Returns an iterator over the child nodes."""
        return iter(self._children)

    def __getitem__(self, key: Union[int, str]) -> Optional["Node"]:
        """Returns the child node for the given key.

        If this is of kind OBJECT or SET, then the key must be a string.
        Otherwise, the key must be an integer. If the key is out of bounds,
        then this will raise a RegoError.

        Returns:
            Node: The child node.

        Raises:
            RegoError: If the key is out of bounds.
            TypeError: If the key is not an integer or a string.
        """
        kind = self._kind
        if kind == NodeKind.Term:
            kind = self.at(0)._kind

        if kind in [NodeKind.Set, NodeKind.Object]:
            return self.lookup(key)

        if isinstance(key, int):
            return self.index(key)

    def json(self) -> str:
        """Returns the node as a JSON string."""
        return rego_node_json(self._impl)

    def __str__(self) -> str:
        """Returns the node as a JSON string."""
        return self.json()

    def __repr__(self) -> str:
        """Returns the node as a JSON string."""
        return "Node({}@{})".format(self.json(), self._impl)
