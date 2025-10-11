"""Python example code."""

from regopy import Input, Interpreter

rego = Interpreter()
with open("examples/objects.rego") as f:
    rego.add_module("objects.rego", f.read())

rego.add_data({
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
})

rego.add_data({
    "three": {
        "bar": "Baz",
        "baz": 15,
        "be": True,
        "bop": 4.23
    }
})

rego.set_input(Input({
    "a": 10,
    "b": "20",
    "c": 30.0,
    "d": True
}))

print(rego.query("[data.one, input.b, data.objects.sites[1]] = x"))

bundle = rego.build("[data.one, input.b, data.objects.sites[1]] = x", ["objects/sites"])

print(rego.query_bundle(bundle))

print(rego.query_bundle_entrypoint(bundle, "objects/sites"))