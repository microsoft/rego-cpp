"""Module providing an interface to the rego-cpp Node object."""

from enum import IntEnum

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


class NodeType(IntEnum):
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
    def __init__(self, impl):
        self._impl = impl

    @property
    def type(self) -> NodeType:
        return regoNodeType(self._impl)

    @property
    def type_name(self) -> str:
        return regoNodeTypeName(self._impl)

    @property
    def value(self) -> str:
        size = regoNodeValueSize(self._impl)
        buf = bytearray(size)
        err = regoNodeValue(self._impl, buf, size)
        if err == REGO_ERROR_BUFFER_TOO_SMALL:
            raise ValueError("Buffer too small")

        return buf.decode("utf-8")

    @property
    def __len__(self) -> int:
        return regoNodeSize(self._impl)

    def __getitem__(self, index: int) -> "Node":
        impl = regoNodeGet(self._impl, index)
        if impl:
            return Node(impl)

        return None

    def __str__(self) -> str:
        size = regoNodeJSONSize(self._impl)
        buf = bytearray(size)
        err = regoNodeJSON(self._impl, buf, size)
        if err == REGO_ERROR_BUFFER_TOO_SMALL:
            raise ValueError("Buffer too small")

        return buf.decode("utf-8")
