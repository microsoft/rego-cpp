namespace Rego.Tests;

using System.Reflection;
using System.Runtime.CompilerServices;
using Rego;

// This class is used to ensure that the code snippets in the documentation compile correctly.
// For the time being, it needs to be manually kept in sync with the documentation.
// In the future, we may be able to automate this process.

[Collection("RegoDocTests")]
public class InterpreterDocTests
{
    [Fact]
    public void Class0()
    {
        Interpreter rego = new();
        var output = rego.Query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5");
        Assert.Equal("""{"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}""", output.ToString());
    }

    [Fact]
    public void Class1()
    {
        Interpreter rego = new();
        var data0 = new Dictionary<string, object>{
            {"one", new Dictionary<string, object>{
                {"bar", "Foo"},
                {"baz", 5},
                {"be", true},
                {"bop", 23.4}}},
            {"two", new Dictionary<string, object>{
                {"bar", "Bar"},
                {"baz", 12.3},
                {"be", false},
                {"bop", 42}}}};
        rego.AddData(data0);

        rego.AddDataJson("""
        {
            "three": {
                "bar": "Baz",
                "baz": 15,
                "be": true,
                "bop": 4.23
            }
        }
        """);

        var objectsSource = """
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
        """;
        rego.AddModule("objects.rego", objectsSource);

        rego.SetInputTerm("""
        {
            "a": 10,
        "b": "20",
            "c": 30.0,
            "d": true
        }
        """);

        var output = rego.Query("x=[data.one, input.b, data.objects.sites[1]]");
        Assert.Equal("""{"expressions":[true], "bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4},"20",{"name":"smoke1"}]}}""", output.ToString());
    }

    [Fact]
    public void Class2()
    {
        Interpreter rego_build = new();
        rego_build.AddDataJson("""
        {"a": 7,
        "b": 13}
        """);

        rego_build.AddModule("example.rego", """
            package example

            foo := data.a * input.x + data.b * input.y
            bar := data.b * input.x + data.a * input.y
        """);

        var bundle = rego_build.Build("x=data.example.foo + data.example.bar", ["example/foo", "example/bar"]);

        rego_build.SaveBundle("bundle_doctest0", bundle);
        Interpreter rego_run = new();
        rego_run.LoadBundle("bundle_doctest0");

        var input = Input.Create(new Dictionary<string, int>
        {
            {"x", 104 },
            {"y", 119 }
        });

        rego_run.SetInput(input);

        var output = rego_run.QueryBundle(bundle);
        Assert.Equal("""{"expressions":[true], "bindings":{"x":4460}}""", output.ToString());

        output = rego_run.QueryBundle(bundle, "example/foo");
        Assert.Equal("""{"expressions":[2275]}""", output.ToString());
    }

    [Fact]
    public void AddModule()
    {
        Interpreter rego = new();
        var source = """
            package scalars
        
            greeting := "Hello"
            max_height := 42
            pi := 3.14159
            allowed := true
            location := null
        """;

        rego.AddModule("scalars.rego", source);
        var output = rego.Query("data.scalars.greeting");
        Assert.Equal("""{"expressions":["Hello"]}""", output.ToString());
    }

    [Fact]
    public void AddDataJson()
    {
        Interpreter rego = new();
        var data = """
          {
            "one": {
              "bar": "Foo",
              "baz": 5,
              "be": true,
              "bop": 23.4
            },
            "two": {
              "bar": "Bar",
              "baz": 12.3,
              "be": false,
              "bop": 42
            }
          }
        """;
        rego.AddDataJson(data);
        var output = rego.Query("data.one.bar");
        Assert.Equal("""{"expressions":["Foo"]}""", output.ToString());
    }

    [Fact]
    public void SetInputTerm()
    {
        Interpreter rego = new();
        var input = """
            {
                "a": 10,
                "b": "20",
                "c": 30.0,
                "d": true
            }
            """;
        rego.SetInputTerm(input);
        var output = rego.Query("input.a");
        Assert.Equal("""{"expressions":[10]}""", output.ToString());
    }

    [Fact]
    public void SetInput()
    {
        Interpreter rego = new();
        var input = Input.Create(new Dictionary<string, int>
        {
            {"x", 104 },
            {"y", 119 }
        });
        rego.SetInput(input);
        var output = rego.Query("x=input.x");
        Assert.Equal("""{"expressions":[true], "bindings":{"x":104}}""", output.ToString());
    }

    [Fact]
    public void Query()
    {
        Interpreter rego = new();
        var input0 = new Dictionary<string, int> { { "a", 10 } };
        var input1 = new Dictionary<string, int> { { "a", 4 } };
        var input2 = new Dictionary<string, int> { { "a", 7 } };
        var multi = """
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
            """;
        rego.AddModule("multi.rego", multi);
        rego.SetInput(input0);
        var output = rego.Query("data.multi.a");
        Assert.Equal("""{"expressions":[1]}""", output.ToString());
        rego.SetInput(input1);
        output = rego.Query("data.multi.a");
        Assert.Equal("""{"expressions":[41]}""", output.ToString());
        rego.SetInput(input2);
        output = rego.Query("data.multi.a");
        Assert.Equal("""{"expressions":[70]}""", output.ToString());
    }

    [Fact]
    public void Build()
    {
        var input = new Dictionary<string, int>{
            {"x", 104 },
            {"y", 119 }};
        var data = new Dictionary<string, int>{
            {"a", 7},
            {"b", 13}};
        var module = """
            package example
            foo := data.a * input.x + data.b * input.y
            bar := data.b * input.x + data.a * input.y
        """;

        Interpreter rego_build = new();
        rego_build.AddData(data);
        rego_build.AddModule("example.rego", module);
        var bundle = rego_build.Build("x=data.example.foo + data.example.bar", ["example/foo", "example/bar"]);

        Interpreter rego_run = new();
        rego_run.SetInput(input);
        var output = rego_run.QueryBundle(bundle);
        Assert.Equal("""{"expressions":[true], "bindings":{"x":4460}}""", output.ToString());

        output = rego_run.QueryBundle(bundle, "example/foo");
        Assert.Equal("""{"expressions":[2275]}""", output.ToString());
    }

    [Fact]
    public void SaveBundle()
    {
        Interpreter rego_build = new();
        var bundle = rego_build.Build("a=1");
        rego_build.SaveBundle("bundle_doctest1", bundle);
        rego_build.SaveBundle("bundle_bin_doctest1.rbb", bundle, BundleFormat.Binary);
        Interpreter rego_run = new();
        bundle = rego_run.LoadBundle("bundle_doctest1");
        var output = rego_run.QueryBundle(bundle);
        Assert.Equal("""{"expressions":[true], "bindings":{"a":1}}""", output.ToString());
        bundle = rego_run.LoadBundle("bundle_bin_doctest1.rbb", BundleFormat.Binary);
        output = rego_run.QueryBundle(bundle);
        Assert.Equal("""{"expressions":[true], "bindings":{"a":1}}""", output.ToString());
    }
}

[Collection("RegoDocTests")]
public class InputDocTests
{
    [Fact]
    public void Class()
    {
        Interpreter rego = new();
        var input = Input.Create(new Dictionary<string, object>{
            { "a", 10 },
            { "b", "20" },
            { "c", 30.0 },
            { "d", true }
        });

        rego.SetInput(input);
        var output = rego.Query("input.a");
        Assert.Equal("""{"expressions":[10]}""", output.ToString());
    }
}

[Collection("RegoDocTests")]
public class NodeDocTests
{
    [Fact]
    public void Class()
    {
        Interpreter rego = new();
        var output = rego.Query("""x={"a": 10, "b": "20", "c": [30.5, 60], "d": true, "e": null}""");
        var x = output.Binding("x");
        Assert.Equal("""{"a":10, "b":"20", "c":[30.5,60], "d":true, "e":null}""", x.ToString());

        Assert.Equal("10", x["a"].ToString());

        Assert.Equal("\"20\"", x["b"].ToString());

        Assert.Equal("30.5", x["c"][0].ToString());

        Assert.Equal("60", x["c"][1].ToString());

        Assert.Equal("true", x["d"].ToString());

        Assert.Equal("null", x["e"].ToString());
    }

    [Fact]
    public void Value()
    {
        Interpreter rego = new();
        var output = rego.Query("""x=10; y="20"; z=true""");
        var x = output.Binding("x");
        Assert.IsType<Int64>(x.Value);
        Assert.Equal("10", x.Value.ToString());

        var y = output.Binding("y");
        Assert.IsType<String>(y.Value);
        Assert.Equal("\"20\"", y.Value.ToString());

        var z = output.Binding("z");
        Assert.IsType<Boolean>(z.Value);
        Assert.Equal(true.ToString(), z.Value.ToString());
    }
}