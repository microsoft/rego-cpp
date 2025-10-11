"""Tests for the Python wrapper for Rego Nodes."""

from regopy import Interpreter


def test_node_access():
    rego = Interpreter()
    result = rego.query('x={"a": 10, "b": "20", "c": [30.0, 60], "d": true, "e": null}')
    assert result is not None
    x = result.binding("x")
    assert x is not None
    assert x.json() == '{"a":10, "b":"20", "c":[30,60], "d":true, "e":null}'

    assert x["a"].value == 10
    assert x["b"].value == "20"
    assert x["c"][0].value == 30.0
    assert x["c"][1].value == 60
    assert x["d"].value
    assert x["e"].value is None
