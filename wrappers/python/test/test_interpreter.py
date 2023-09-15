"""Tests for the Python wrapper of the Rego interpreter."""

from regopy import Interpreter


def test_query_math():
    rego = Interpreter()
    output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4")
    assert output is not None
    assert str(output) == "x = 5\ny = 9.4"
    assert output.binding("x").json() == "5"
    assert output.binding("y").json() == "9.4"


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
    assert str(output) == "x = 1"
    assert output.binding("x").value == 1
    rego.set_input(input1)
    output = rego.query("x = data.multi.a")
    assert output is not None
    assert output.binding("x").value == 41
    rego.set_input(input2)
    output = rego.query("x = data.multi.a")
    assert output is not None
    assert output.binding("x").value == 70
