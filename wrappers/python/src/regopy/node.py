"""Module providing an interface to the rego-cpp Node object."""

from enum import IntEnum
import json
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
    REGO_NODE_TERMS,
    REGO_NODE_BINDINGS,
    REGO_NODE_RESULTS,
    REGO_NODE_RESULT,
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
    Terms = REGO_NODE_TERMS
    Bindings = REGO_NODE_BINDINGS
    Results = REGO_NODE_RESULTS
    Result = REGO_NODE_RESULT
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

        size = regoNodeSize(self._impl)
        if self._kind == NodeKind.Object:
            self._children = {}
            for i in range(size):
                child = Node(regoNodeGet(self._impl, i))
                assert child._kind == NodeKind.ObjectItem
                key = regoNodeJSON(child.at(0)._impl)
                self._children[key] = child.at(1)
        elif self._kind == NodeKind.Set:
            self._children = {}
            for i in range(size):
                child = Node(regoNodeGet(self._impl, i))
                key = regoNodeJSON(child._impl)
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
            self._children[index] = Node(regoNodeGet(self._impl, index))

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
        kind = self._kind
        if kind == NodeKind.Term:
            kind = self.at(0)._kind

        if kind in [NodeKind.Set, NodeKind.Object]:
            return self.lookup(key)

        if isinstance(key, int):
            return self.index(key)

    def json(self) -> str:
        """Returns the node as a JSON string."""
        return regoNodeJSON(self._impl)

    def __str__(self) -> str:
        """Returns the node as a JSON string."""
        return self.json()

    def __repr__(self) -> str:
        """Returns the node as a JSON string."""
        return "Node({}@{})".format(self.json(), self._impl)
