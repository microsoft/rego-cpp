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