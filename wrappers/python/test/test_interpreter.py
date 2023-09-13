"""Tests for the Python wrapper of the Rego interpreter."""

from regopy import Interpreter


def test_query_math():
    rego = Interpreter()
    output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4")
    assert output is not None
    assert str(output) == "x = 5\ny = 9.4\n"
    assert output.binding("x").json() == "5"
    assert output.binding("y").json() == "9.4"
