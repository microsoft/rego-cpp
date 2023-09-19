#include <rego/rego.hh>

int main()
{
  rego::Interpreter rego;
  rego.add_module("objects", R"(package objects

rect := {`width`: 2, "height": 4}
cube := {"width": 3, `height`: 4, "depth": 5}
a := 42
b := false
c := null
d := {"a": a, "x": [b, c]}
index := 1
shapes := [rect, cube]
names := ["prod", `smoke1`, "dev"]
sites := [{"name": "prod"}, {"name": names[index]}, {"name": "dev"}]
e := {
    a: "foo",
    "three": c,
    names[2]: b,
    "four": d,
}
f := e["dev"])");
  rego.add_data_json(R"({
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
})");
  rego.add_data_json(R"({
    "three": {
        "bar": "Baz",
        "baz": 15,
        "be": true,
        "bop": 4.23
    }
})");
  rego.set_input_json(R"({
    "a": 10,
    "b": "20",
    "c": 30.0,
    "d": true
})");
  std::cout << rego.query("[data.one, input.b, data.objects.sites[1]] = x")
            << std::endl;
}
