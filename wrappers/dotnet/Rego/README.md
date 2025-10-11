# Rego

## What is Rego?

Rego is a language developed by [Open Policy Agent (OPA)](https://www.openpolicyagent.org/) for use in defining policies in cloud systems.

We think Rego is a great language, and you can learn more about it [from the OPA website](https://www.openpolicyagent.org/docs/latest/policy-language/). However, it has some limitations. Primarily, the interpreter is only accessible via the command line or a web server. Further, the only option for using the language in-process is via an interface in Go.

The rego-cpp project provides the ability to integrate Rego natively into a wider range of languages. We currrently support C, C++, Rust, Python and, via this package, .NET, and are largely compatible with v1.8.0 of the language. You can learn more about our implementation [on our Github page](https://github.com/microsoft/rego-cpp).

## Getting Started

Here is an example of how to use the library to execute Rego queries:

```csharp
using Rego;

var rego = new Interpreter();

// If you provide an object, it will be converted to JSON before
// being added to the state.
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
rego.AddModule("objects", objectsSource);

// inputs can be either JSON or Rego, and provided
// as objects (which will be converted to JSON) or as
// text.
rego.SetInputTerm("""
    {
        "a": 10,
        "b": "20",
        "c": 30.0,
        "d": true
    }
    """);

Console.WriteLine(rego.Query("[data.one, input.b, data.objects.sites[1]] = x"));
// Result: {"bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]}}
```
