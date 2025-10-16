# Getting Started

Here are some examples of usage to help you get started.

## Querying and Accessing Results

```csharp
using System.Collections;
using Rego;

// You can run simple queries without any input or data.
Interpreter rego = new();
var output = rego.Query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5");
Console.WriteLine(output);
// {"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}

// You can access bound results using the Binding method
Console.WriteLine("x = {0}", output.Binding("x"));

// You can also access expressions by index
Console.WriteLine(output.Expressions()[2]);
```

## Incorporating Base and Virtual Documents

Most policies are built up from a combination of Base Documents (static
JSON data) and Virtual Documents (Rego modules). Here we see a typical
usage example incorporating all the different components of a Rego policy:

```csharp
// We can add base documents using .NET objects, which will be converted
// to JSON and then added.
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

// You can also provide JSON directly.
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

// The Rego code can be written out in-source like this or loaded from disk.
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

// inputs can be either JSON or Rego, and provided as objects (which will
// be converted to JSON) or as text.
rego.SetInputTerm("""
    {
        "a": 10,
        "b": "20",
        "c": 30.0,
        "d": true
    }
    """);

Console.WriteLine(rego.Query("[data.one, input.b, data.objects.sites[1]] = x"));
// {"bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]}}
```

## Bundles

If you want to run the same set of queries against a policy with different
inputs, you can create a bundle and use that to save the cost of compilation.

```csharp
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

// We can specify both a default query, and specific entry points into the policy
// that should be made available to use later.
var bundle = rego_build.Build("x=data.example.foo + data.example.bar",
                              ["example/foo", "example/bar"]);

// we can now save the bundle to the disk
rego_build.SaveBundle("bundle", bundle);

// and load it again
Interpreter rego_run = new();
rego_run.LoadBundle("bundle");

// the most efficient way to provide input to a policy is by constructing it
// manually, without the need for parsing JSON or Rego.

var input = Input.Create(new Dictionary<string, int>
        {
            {"x", 104 },
            {"y", 119 }
        });

rego_run.SetInput(input);

// We can query the bundle, which will use the entrypoint of the default query
// provided at build
Console.WriteLine("query: {0}",rego_run.QueryBundle(bundle));
// Query: {"expressions":[true], "bindings":{"x":4460}}

// or we can query specific entrypoints
Console.WriteLine("example/foo: {0}", rego_run.QueryBundle(bundle, "example/foo"));
// example/foo: {"expressions":[2275]}
```
