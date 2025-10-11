"""Tests for the regopy Input builder."""

from collections.abc import Mapping
from typing import Sequence, Set
from regopy import Interpreter, Input

from enum import IntEnum
import json
import random
import pytest


NumTests = 10
MaxValueSize = 16
MaxObjectDepth = 3

AlphaNum = "abcdefghijklmnopqrstuvwxyz0123456789"


class ValueType(IntEnum):
    Int = 0
    Float = 1
    String = 2
    Boolean = 3
    Null = 4
    Object = 5
    Array = 6

    @staticmethod
    def choose_scalar() -> "ValueType":
        return ValueType(random.randint(0, 4))

    @staticmethod
    def choose(depth=MaxObjectDepth) -> "ValueType":
        if depth > 0:
            return ValueType(random.randint(0, 6))

        return ValueType.choose_scalar()

    @staticmethod
    def random_scalar():
        return ValueType.choose_scalar().generate()

    @staticmethod
    def random(depth: int):
        return ValueType.choose(depth).generate(depth)

    def generate(self, depth=MaxObjectDepth):
        match self.value:
            case ValueType.Int:
                return random.randint(-2**63, 2**63)

            case ValueType.Float:
                return random.random() * 1000

            case ValueType.String:
                length = random.randint(4, 32)
                indices = [random.randint(0, len(AlphaNum) - 1) for _ in range(length)]
                return "".join(AlphaNum[i] for i in indices)

            case ValueType.Boolean:
                return True if random.random() > 0.5 else False

            case ValueType.Null:
                return None

            case ValueType.Object:
                length = random.randint(0, MaxValueSize)
                return {ValueType.String.generate(): ValueType.random(depth - 1) for _ in range(length)}

            case ValueType.Array:
                length = random.randint(0, MaxValueSize)
                return [ValueType.random(depth - 1) for _ in range(length)]


def assert_equal(lhs, rhs):
    assert isinstance(lhs, type(rhs))

    if isinstance(lhs, float):
        assert lhs == pytest.approx(rhs)
        return

    if isinstance(lhs, str):
        assert lhs == rhs
        return

    if isinstance(lhs, Mapping):
        assert len(rhs) == len(rhs)
        for k in lhs:
            assert k in rhs
            assert_equal(lhs[k], rhs[k])

        return

    if isinstance(lhs, Sequence):
        assert len(lhs) == len(rhs)
        for a, b in zip(lhs, rhs):
            assert_equal(a, b)

        return

    assert lhs == rhs


@pytest.mark.parametrize("value", [ValueType.Int.generate() for _ in range(NumTests)])
def test_int(value: int):
    rego = Interpreter()
    rego.set_input(Input(value))
    output = rego.query("x = input")
    assert output.binding("x").value == value


@pytest.mark.parametrize("value", [ValueType.Float.generate() for _ in range(NumTests)])
def test_float(value: float):
    rego = Interpreter()
    rego.set_input(Input(value))
    output = rego.query("x = input")
    assert output.binding("x").value == pytest.approx(value)


@pytest.mark.parametrize("value", [ValueType.String.generate() for _ in range(NumTests)])
def test_string(value: float):
    rego = Interpreter()
    rego.set_input(Input(value))
    output = rego.query("x = input")
    assert output.binding("x").value == value


@pytest.mark.parametrize("value", [True, False, None])
def test_bool_none(value: int):
    rego = Interpreter()
    rego.set_input(Input(value))
    output = rego.query("x = input")
    assert output.binding("x").value == value


@pytest.mark.parametrize("value", [ValueType.Object.generate(1) for _ in range(NumTests)])
def test_object(value: dict):
    rego = Interpreter()
    rego.set_input(Input(value))
    output = rego.query("x = input")
    value_obj = json.loads(output.binding("x").json())
    assert_equal(value_obj, value)


@pytest.mark.parametrize("value", [ValueType.Array.generate(1) for _ in range(NumTests)])
def test_array(value: dict):
    rego = Interpreter()
    rego.set_input(Input(value))
    output = rego.query("x = input")
    value_obj = json.loads(output.binding("x").json())
    assert_equal(value_obj, value)
