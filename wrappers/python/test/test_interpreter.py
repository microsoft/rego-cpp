"""Tests for the Python wrapper of the Rego interpreter."""

import os
from pathlib import Path

import pytest
from regopy import Interpreter, RegoError, BundleFormat, set_default_log_level


def test_query_math():
    rego = Interpreter()
    output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5")
    assert output is not None
    assert str(output) == '{"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}'
    assert len(output) == 1
    result = output[0]
    assert result.bindings["x"] == 5
    assert result.bindings["y"] == 9.4
    print(result.expressions)
    assert result.expressions[-1] == 10


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
    assert data_one["bar"].value == "Foo"
    assert data_one["be"].value is True
    assert data_one["baz"].value == 5
    assert data_one["bop"].value == 23.4
    assert x[1].value == "20"
    data_objects_sites_1 = x[2]
    assert data_objects_sites_1["name"].value == "smoke1"


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
    assert str(output) == '{"expressions":[true], "bindings":{"x":1}}'
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
    assert a["2"].value == "2"
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


def test_build():
    input0 = {
        "x": 104,
        "y": 119
    }
    data = {
        "a": 7,
        "b": 13
    }
    module = """
        package example

        foo := data.a * input.x + data.b * input.y
        bar := data.b * input.x + data.a * input.y
    """

    d = Path("bundle")
    f = Path("bundle_bin.rbb")

    set_default_log_level("Debug")

    rego_build = Interpreter()
    rego_build.add_data(data)
    rego_build.add_module("test.rego", module)
    bundle = rego_build.build("x=data.example.foo + data.example.bar", ["example/foo", "example/bar"])
    assert bundle.ok()

    rego_build.save_bundle(str(d), bundle)
    rego_build.save_bundle(str(f), bundle, BundleFormat.Binary)

    assert os.path.exists(d / "plan.json")
    assert os.path.exists(d / "data.json")
    assert os.path.exists(d / "test.rego")

    rego_run = Interpreter()
    bundle = rego_run.load_bundle(str(d))
    assert bundle.ok()

    rego_run.set_input(input0)
    output = rego_run.query_bundle(bundle)
    assert output.binding("x").value == 4460

    output = rego_run.query_bundle_entrypoint(bundle, "example/foo")
    assert output.expressions()[0].value == 2275

    bundle_bin = rego_run.load_bundle(str(f), BundleFormat.Binary)
    output_bin = rego_run.query_bundle_entrypoint(bundle_bin, "example/foo")
    assert str(output) == str(output_bin)
