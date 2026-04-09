"""Python example code."""

from regopy import Input, Interpreter

##############################
#####    Query Only      #####
##############################

print("Query Only")

# You can run simple queries without any input or data.
rego = Interpreter()
output = rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5")
print(output)
# {"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}

# You can access bound results using the binding method
print("x =", output.binding("x").json())

# You can also access expressions by index
print(output[0][2])

print()

##############################
####   Input and Data    #####
##############################

print("Input and Data")

# If you provide a dict, it will be converted to JSON before
# being added to the state.
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

# You can also provide JSON directly.
rego.add_data_json("""
{
    "three": {
        "bar": "Baz",
        "baz": 15,
        "be": true,
        "bop": 4.23
    }
}
""")

objects_source = """
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
rego.add_module("objects.rego", objects_source)

# Inputs can be either JSON or Rego, and provided
# as objects or as text.
rego.set_input_term("""
{
    "a": 10,
    "b": "20",
    "c": 30.0,
    "d": true
}
""")

print(rego.query("[data.one, input.b, data.objects.sites[1]] = x"))
# {"bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]}}

##############################
#####      Bundles       #####
##############################

print()
print("Bundles")

# If you want to run the same set of queries against a policy with different
# inputs, you can create a bundle and use that to save the cost of compilation.

rego_build = Interpreter()

rego_build.add_data_json("""
{"a": 7,
"b": 13}
""")

rego_build.add_module("example.rego", """
package example

foo := data.a * input.x + data.b * input.y
bar := data.b * input.x + data.a * input.y
""")

# We can specify both a default query, and specific entry points into the policy
# that should be made available to use later.
bundle = rego_build.build(
    "x=data.example.foo + data.example.bar",
    ["example/foo", "example/bar"]
)

# We can now save the bundle to disk
rego_build.save_bundle("bundle", bundle)

# And load it again
rego_run = Interpreter()
rego_run.load_bundle("bundle")

# The most efficient way to provide input to a policy is by constructing it
# manually, without the need for parsing JSON or Rego.
rego_run.set_input(Input({"x": 104, "y": 119}))

# We can query the bundle, which will use the entrypoint of the default query
# provided at build
print("query:", rego_run.query_bundle(bundle))
# query: {"expressions":[true], "bindings":{"x":4460}}

# Or we can query specific entrypoints
print("example/foo:", rego_run.query_bundle_entrypoint(bundle, "example/foo"))
# example/foo: {"expressions":[2275]}
