"""Module providing an interface to the rego-cpp Node object."""

from enum import IntEnum
from typing import Union

from ._regopy import (
    REGO_NODE_ARRAY,
    REGO_NODE_BINDING,
    REGO_NODE_ERROR,
    REGO_NODE_ERROR_AST,
    REGO_NODE_ERROR_CODE,
    REGO_NODE_ERROR_MESSAGE,
    REGO_NODE_ERROR_SEQ,
    REGO_NODE_FALSE,
    REGO_NODE_FLOAT,
    REGO_NODE_INT,
    REGO_NODE_INTERNAL,
    REGO_NODE_NULL,
    REGO_NODE_OBJECT,
    REGO_NODE_OBJECT_ITEM,
    REGO_NODE_SCALAR,
    REGO_NODE_SET,
    REGO_NODE_STRING,
    REGO_NODE_TERM,
    REGO_NODE_TRUE,
    REGO_NODE_UNDEFINED,
    REGO_NODE_VAR,
    regoNodeGet,
    regoNodeJSON,
    regoNodeSize,
    regoNodeType,
    regoNodeTypeName,
    regoNodeValue,
)


class NodeKind(IntEnum):
    """Enumeration of node types."""

    Binding = REGO_NODE_BINDING
    Var = REGO_NODE_VAR
    Term = REGO_NODE_TERM
    Scalar = REGO_NODE_SCALAR
    Array = REGO_NODE_ARRAY
    Set = REGO_NODE_SET
    Object = REGO_NODE_OBJECT
    ObjectItem = REGO_NODE_OBJECT_ITEM
    Int = REGO_NODE_INT
    Float = REGO_NODE_FLOAT
    String = REGO_NODE_STRING
    Boolean = REGO_NODE_TRUE
    Null = REGO_NODE_NULL
    Undefined = REGO_NODE_UNDEFINED
    Error = REGO_NODE_ERROR
    ErrorMessage = REGO_NODE_ERROR_MESSAGE
    ErrorAst = REGO_NODE_ERROR_AST
    ErrorCode = REGO_NODE_ERROR_CODE
    ErrorSeq = REGO_NODE_ERROR_SEQ
    Internal = REGO_NODE_INTERNAL


class Node:
    """Interface for the Rego Node.

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
    x = {"a":10, "b":"20", "c":[30, 60], "d":true, "e":null}

    >>> print("x['a'] =", x["a"].value)
    x['a'] = 10

    >>> print("x['b'] =", x["b"].value)
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
        kind = regoNodeType(self._impl)
        if kind in (REGO_NODE_TRUE, REGO_NODE_FALSE):
            self._kind = NodeKind.Boolean
        else:
            self._kind = NodeKind(regoNodeType(self._impl))

        self._children = [Node(regoNodeGet(self._impl, i)) for i in range(len(self))]

    @property
    def kind(self) -> NodeKind:
        """Returns the node type."""
        return self._kind

    @property
    def kind_name(self) -> str:
        """Returns a human-readable string representation of the node kind."""
        return regoNodeTypeName(self._impl)

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

        value = regoNodeValue(self._impl)
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
        return regoNodeSize(self._impl)

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

        if self._kind == NodeKind.Array:
            if index < 0 or index >= len(self):
                raise IndexError("index out of bounds")

            return self.at(index)

        raise TypeError("index is only valid for Array nodes")

    def lookup(self, key: str) -> "Node":
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

        if self._kind == NodeKind.Object:
            for child in self._children:
                assert child.kind == NodeKind.ObjectItem
                child_key = child.at(0).value
                if child_key == key:
                    return child.at(1)

                if child_key.startswith('"') and child_key.endswith('"'):
                    child_key = child_key[1:-1]

                if child_key == key:
                    return child.at(1)

            raise KeyError(f"Key {key} not found")

        if self._kind == NodeKind.Set:
            for child in self._children:
                if child.value == key:
                    return child

            return None

        raise TypeError("lookup is only valid for OBJECT and SET nodes")

    def at(self, index: int) -> "Node":
        """Returns the child node at the given index."""
        return self._children[index]

    def __iter__(self):
        """Returns an iterator over the child nodes."""
        return iter(self._children)

    def __getitem__(self, key: Union[int, str]) -> "Node":
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
        if isinstance(key, int):
            return self.index(key)

        if isinstance(key, str):
            return self.lookup(key)

        raise TypeError("key must be an integer or a string")

    def json(self) -> str:
        """Returns the node as a JSON string."""
        return regoNodeJSON(self._impl)

    def __str__(self) -> str:
        """Returns the node as a JSON string."""
        return self.json()

    def __repr__(self) -> str:
        """Returns the node as a JSON string."""
        return "Node({}@{})".format(self.json(), self._impl)
