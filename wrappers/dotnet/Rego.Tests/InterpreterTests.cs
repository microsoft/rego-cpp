namespace Rego.Tests;

using Rego;

[Collection("RegoTests")]
public class InterpreterTest
{
    [Fact]
    public void Interpreter_new()
    {
        var interpreter = new Interpreter();
        Assert.False(interpreter.Handle.IsInvalid);
        var version = Interpreter.Version;
        Assert.NotEmpty(version);
        var buildInfo = Interpreter.BuildInfo;
        Assert.NotEmpty(buildInfo);
    }

    [Fact]
    public void Interpreter_query_math()
    {
        var interpreter = new Interpreter();
        var output = interpreter.Query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5");
        Assert.True(output.Ok);
        Assert.Equal(1, output.Count);
        var x = output.Binding("x");
        Assert.Equal(5, x.Value);
        var y = output.Binding("y");
        Assert.InRange((double)y.Value, 9.39, 9.41);
        var expressions = output.Expressions();
        Assert.Single(expressions);
        Assert.Equal(10, expressions[0].Value);
    }

    [Fact]
    public void Interpreter_input_data()
    {
        var input0 = new Dictionary<string, object>{
        {"a", 10},
        {"b", "20"},
        {"c", 30.0},
        {"d", true}};
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
        var data1 = new Dictionary<string, object>{
        {"three", new Dictionary<string, object>{
            {"bar", "Baz"},
            {"baz", 15},
            {"be", true},
            {"bop", 4.23}}}};
        var module = """
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
        Interpreter rego = new();
        rego.SetInput(input0);
        rego.AddData(data0);
        rego.AddData(data1);
        rego.AddModule("objects", module);
        var output = rego.Query("x=[data.one, input.b, data.objects.sites[1]]");
        Assert.True(output.Ok);
        var x = output.Binding("x");
        var data_one = x[0];
        Assert.Equal("\"Foo\"", data_one["bar"].Value);
        Assert.True((bool)data_one["be"].Value);
        Assert.Equal(5, data_one["baz"].Value);
        Assert.InRange((double)data_one["bop"].Value, 23.39, 23.41);
        Assert.Equal("\"20\"", x[1].Value);
        var data_objects_sites_1 = x[2];
        Assert.Equal("\"smoke1\"", data_objects_sites_1["name"].Value);
    }

    [Fact]
    public void Interpreter_multiple_inputs()
    {
        var input0 = new Dictionary<string, object>{
        {"a", 10}};
        var input1 = new Dictionary<string, object>{
        {"a", 4}};
        var input2 = new Dictionary<string, object>{
        {"a", 7}};
        var module = """
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
        Interpreter rego = new();
        rego.AddModule("multi", module);
        rego.SetInput(input0);
        var output = rego.Query("x = data.multi.a");
        Assert.True(output.Ok);
        Assert.Equal("{\"bindings\":{\"x\":1}}", output.ToString());
        Assert.Equal(1.0, output.Binding("x").Value);
        rego.SetInput(input1);
        output = rego.Query("x = data.multi.a");
        Assert.True(output.Ok);
        Assert.Equal(41, output.Binding("x").Value);
        rego.SetInput(input2);
        Assert.True(output.Ok);
        output = rego.Query("x = data.multi.a");
        Assert.Equal(70, output.Binding("x").Value);
    }

    [Fact]
    public void Interpreter_time()
    {
        var rego = new Interpreter();
        var output = rego.Query("x=time.clock([1727267567139080131, \"America/Los_Angeles\"])");
        Assert.True(output.Ok);
        if(rego.IsBuiltIn("time.clock"))
        {
            var clock = output.Binding("x");
            Assert.Equal(3, clock.Count);
            Assert.Equal(5, clock[0].Value);
            Assert.Equal(32, clock[1].Value);
            Assert.Equal(47, clock[2].Value);
        }
        else
        {
            Assert.Equal(0, output.Count);
        }
    }

    [Fact]
    public void Interpreter_sets()
    {
        Interpreter rego = new();
        var output = rego.Query("""a = {1, "2", false, 4.3}""");
        Assert.True(output.Ok);
        var a = output.Binding("a");
        Assert.Equal(4, a.Count);
        Assert.Equal(1, a[1].Value);
        Assert.Equal("\"2\"", a["2"].Value);
        Assert.False((bool)a[false].Value);
        Assert.Equal(4.3, a[4.3].Value);
        Assert.False(a.TryGetValue(6, out Node? _));
    }

    [Fact]
    public void Interpreter_badModule()
    {
        Interpreter rego = new();
        Assert.Throws<RegoException>(() => rego.AddModule("bad", "\n\nx <> 1"));
    }
}