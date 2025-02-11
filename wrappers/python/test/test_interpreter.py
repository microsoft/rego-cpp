"""Tests for the Python wrapper of the Rego interpreter."""

import pytest
from regopy import Interpreter, RegoError


def test_query_math():
    rego = Interpreter()
    output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5")
    assert output is not None
    assert str(output) == '{"expressions":[10], "bindings":{"x":5, "y":9.4}}'
    assert output.binding("x").json() == "5"
    assert output.binding("y").json() == "9.4"
    assert output.expressions()[0].json() == "10"


def test_input_data():
    input0 = {
        "a": 10,
        "b": "20",
        "c": 30.0,
        "d": True
    }
    data0 = {
        "one": {
            "bar": "Foo",
            "baz": 5,
            "be": True,
            "bop": 23.4
        },
        "two": {
            "bar": "Bar",
            "baz": 12.3,
            "be": False,
            "bop": 42
        }
    }
    data1 = {
        "three": {
            "bar": "Baz",
            "baz": 15,
            "be": True,
            "bop": 4.23
        }
    }
    module = """
    package objects

    rect := {"width": 2, "height": 4}
    cube := {"width": 3, "height": 4, "depth": 5}
    a := 42
    b := false
    c := null
    d := {"a": a, "x": [b, c]}
    index := 1
    shapes := [rect, cube]
    names := ["prod", "smoke1", "dev"]
    sites := [{"name": "prod"}, {"name": names[index]}, {"name": "dev"}]
    e := {
        a: "foo",
        "three": c,
        names[2]: b,
        "four": d,
    }
    f := e["dev"]
    """
    rego = Interpreter()
    rego.set_input(input0)
    rego.add_data(data0)
    rego.add_data(data1)
    rego.add_module("objects", module)
    output = rego.query("x=[data.one, input.b, data.objects.sites[1]]")
    assert output is not None
    x = output.binding("x")
    data_one = x[0]
    assert data_one["bar"].value == '"Foo"'
    assert data_one["be"].value is True
    assert data_one["baz"].value == 5
    assert data_one["bop"].value == 23.4
    assert x[1].value == '"20"'
    data_objects_sites_1 = x[2]
    assert data_objects_sites_1["name"].value == '"smoke1"'


def test_multiple_inputs():
    input0 = {"a": 10}
    input1 = {"a": 4}
    input2 = {"a": 7}
    module = """
    package multi

    default a := 0

    a := val {
        input.a > 0
        input.a < 10
        input.a % 2 == 1
        val := input.a * 10
    } {
        input.a > 0
        input.a < 10
        input.a % 2 == 0
        val := input.a * 10 + 1
    }

    a := input.a / 10 {
        input.a >= 10
    }
    """
    rego = Interpreter()
    rego.add_module("multi", module)
    rego.set_input(input0)
    output = rego.query("x = data.multi.a")
    assert str(output) == """{"bindings":{"x":1}}"""
    assert output.binding("x").value == 1
    rego.set_input(input1)
    output = rego.query("x = data.multi.a")
    assert output is not None
    assert output.binding("x").value == 41
    rego.set_input(input2)
    output = rego.query("x = data.multi.a")
    assert output is not None
    assert output.binding("x").value == 70


def test_time():
    rego = Interpreter()
    if rego.is_builtin("time.clock"):
        output = rego.query("""x=time.clock([1727267567139080131, "America/Los_Angeles"])""")
        assert output is not None
        clock = output.binding("x")
        assert len(clock) == 3
        assert clock[0].value == 5
        assert clock[1].value == 32
        assert clock[2].value == 47
    else:
        pytest.skip("test_time skipped: time.clock not available")


def test_set():
    rego = Interpreter()
    output = rego.query("""a = {1, "2", false, 4.3}""")
    assert output is not None
    a = output.binding("a")
    assert len(a) == 4
    assert a[1].value == 1
    assert a["2"].value == '"2"'
    assert not a[False].value
    assert a[4.3].value == 4.3
    assert a[6] is None


def test_bad_module():
    rego = Interpreter()
    try:
        rego.add_module("bad", "package bad\n\nx <> 1")
    except RegoError:
        pass
    else:
        raise AssertionError("Expected RegoError")
