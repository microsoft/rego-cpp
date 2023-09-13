"""Module providing an interface to the rego-cpp Node object."""

from enum import IntEnum
from typing import Union

from ._regopy import (
    REGO_ERROR_BUFFER_TOO_SMALL,
    REGO_NODE_BINDING,
    REGO_NODE_VAR,
    REGO_NODE_TERM,
    REGO_NODE_SCALAR,
    REGO_NODE_ARRAY,
    REGO_NODE_SET,
    REGO_NODE_OBJECT,
    REGO_NODE_OBJECT_ITEM,
    REGO_NODE_INT,
    REGO_NODE_FLOAT,
    REGO_NODE_STRING,
    REGO_NODE_TRUE,
    REGO_NODE_FALSE,
    REGO_NODE_NULL,
    REGO_NODE_UNDEFINED,
    REGO_NODE_ERROR,
    REGO_NODE_ERROR_MESSAGE,
    REGO_NODE_ERROR_AST,
    REGO_NODE_ERROR_CODE,
    REGO_NODE_ERROR_SEQ,
    REGO_NODE_INTERNAL,
    regoNodeType,
    regoNodeTypeName,
    regoNodeValueSize,
    regoNodeValue,
    regoNodeSize,
    regoNodeGet,
    regoNodeJSONSize,
    regoNodeJSON
)


class NodeKind(IntEnum):
    """Enumeration of node types."""

    BINDING = REGO_NODE_BINDING
    VAR = REGO_NODE_VAR
    TERM = REGO_NODE_TERM
    SCALAR = REGO_NODE_SCALAR
    ARRAY = REGO_NODE_ARRAY
    SET = REGO_NODE_SET
    OBJECT = REGO_NODE_OBJECT
    OBJECT_ITEM = REGO_NODE_OBJECT_ITEM
    INT = REGO_NODE_INT
    FLOAT = REGO_NODE_FLOAT
    STRING = REGO_NODE_STRING
    TRUE = REGO_NODE_TRUE
    FALSE = REGO_NODE_FALSE
    NULL = REGO_NODE_NULL
    UNDEFINED = REGO_NODE_UNDEFINED
    ERROR = REGO_NODE_ERROR
    ERROR_MESSAGE = REGO_NODE_ERROR_MESSAGE
    ERROR_AST = REGO_NODE_ERROR_AST
    ERROR_CODE = REGO_NODE_ERROR_CODE
    ERROR_SEQ = REGO_NODE_ERROR_SEQ
    INTERNAL = REGO_NODE_INTERNAL


class Node:
    """Interface for the Rego Node.

    Rego Nodes are the basic building blocks of a Rego result. They
    exist in a tree structure. Each node has a kind, which is one of
    the variants of [`NodeKind`]. Each node also has zero or more
    children, which are also nodes.

    Example:
    >>> from regopy import Interpreter
    >>> rego = Interpreter()
    >>> output = rego.query("x={"a": 10, "b": "20", "c": [30.0, 60], "d": true, "e": null}")
    >>> x = output.binding("x")
    >>> print("x =", x.json())
    x = {"a": 10, "b": "20", "c": [30.0, 60], "d": true, "e": null}
    >>> print("x['a'] =", x["a"].value)
    x['a'] = 10
    >>> print("x['b'] =", x["b"].value)
    x['b'] = 20
    >>> print("x['c'][0] =", x["c"][0].value)
    x['c'][0] = 30.0
    >>> print("x['c'][1] =", x["c"][1].value)
    x['c'][1] = 60
    >>> print("x['d'] =", x["d"].value)
    x['d'] = True
    >>> print("x['e'] =", x["e"].value)
    x['e'] = None
    """

    ValueKinds = [NodeKind.INT, NodeKind.FLOAT, NodeKind.STRING, NodeKind.TRUE, NodeKind.FALSE, NodeKind.NULL]

    def __init__(self, impl):
        """Creates a new node."""
        self._impl = impl
        self._kind = regoNodeType(self._impl)
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
            y = 20

            >>> z = output.binding("z")
            >>> print("z =", z.value)
            z = True
        """
        if self._kind == NodeKind.Term or self._kind == NodeKind.SCALAR:
            return self._children[0].value()

        if self._kind not in Node.ValueKinds:
            raise ValueError("Node does not have a value")

        if hasattr(self, "_value"):
            return self._value

        size = regoNodeValueSize(self._impl)
        buf = bytearray(size)
        err = regoNodeValue(self._impl, buf, size)
        if err == REGO_ERROR_BUFFER_TOO_SMALL:
            raise ValueError("Buffer too small")

        value = buf.decode("utf-8")
        if self._kind == NodeKind.INT:
            self.value = int(value)

        if self._kind == NodeKind.FLOAT:
            self.value = float(value)

        if self._kind == NodeKind.TRUE:
            self.value = True

        if self._kind == NodeKind.FALSE:
            self.value = False

        if self._kind == NodeKind.NULL:
            self.value = None

        return self.value

    @property
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
            return self.index(0).index(index)

        if self._kind == NodeKind.ARRAY:
            if index < 0 or index >= len(self):
                raise IndexError("index out of bounds")

            return self._children[index]

        raise TypeError("index is only valid for ARRAY nodes")

    def lookup(self, key: str) -> "Node":
        """Returns the child node for the given key.

        If this is of kind OBJECT or SET, then the key must be a string.

        Returns:
            Node: The child node.
        
        Raises:
            KeyError: If the key is not found.
            TypeError: If the node does not support lookup
        """
        if self._kind == NodeKind.Term:
            return self.index(0).lookup(key)

        if self._kind == NodeKind.OBJECT:
            for child in self._children:
                if child.kind == NodeKind.OBJECT_ITEM and child.index(0).value == key:
                    return child.index(1)

            raise KeyError(f"Key {key} not found")

        if self._kind == NodeKind.SET:
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
        size = regoNodeJSONSize(self._impl)
        buf = bytearray(size)
        err = regoNodeJSON(self._impl, buf, size)
        if err == REGO_ERROR_BUFFER_TOO_SMALL:
            raise ValueError("Buffer too small")

        return buf.decode("utf-8")

    def __str__(self) -> str:
        """Returns the node as a JSON string."""
        return self.json()
