"""Module providing an interface to a rego-cpp output object."""

from .node import Node
from ._regopy import (
    regoFreeOutput,
    regoOutputBinding,
    regoOutputNode,
    regoOutputOk,
    regoOutputString,
)


class Output:
    def __init__(self, impl):
        self._impl = impl

    def __del__(self):
        regoFreeOutput(self._impl)

    def __str__(self):
        regoOutputString(self._impl)

    def node(self) -> Node:
        return Node(regoOutputNode(self._impl))

    def binding(self, name: str) -> Node:
        impl = regoOutputBinding(self._impl, name)
        if impl:
            return Node(impl)

        return None

    def ok(self) -> bool:
        regoOutputOk(self._impl)
