#include "trieste/logging.h"

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
  rego.set_input_term(R"({
    "a": 10,
    "b": "20",
    "c": 30.0,
    "d": true
})");

  auto output =
    rego.query_output("[data.one, input.a, data.objects.sites[1]] = x");
  std::cout << output.json() << std::endl;

  // we can get the value of bindings using the `binding` method on output
  auto x = output.binding("x");
  // as Rego integer can be arbitrary precision,
  // rego-cpp exposes an arbitrary precision integer type
  // called BigInt.
  rego::BigInt x_1 = rego::try_get_int(x->at(1)).value();
  std::cout << "x[1] = " << x_1 << std::endl;
  // we can get values from objects using try_get_item
  rego::Node x_2 = x->at(2);
  std::string x_2_val =
    rego::try_get_string(rego::try_get_item(x->at(2), "name").value()).value();
  std::cout << "x[2][\"name\"] = " << x_2_val << std::endl;

  // if we need to reuse a policy, we can build a bundle and save it for re-use
  rego.set_query("[data.one, input.b, data.objects.sites[1]] = x");
  rego.entrypoints({"objects/sites", "objects/rect"});
  rego::Node bundle_node = rego.build();
  if (bundle_node == rego::ErrorSeq)
  {
    rego::logging::Error() << bundle_node;
    return 1;
  }

  rego.save_bundle("bundle", bundle_node);

  rego::Interpreter rego_run;

  bundle_node = rego_run.load_bundle("bundle");
  if (bundle_node == rego::ErrorSeq)
  {
    rego::logging::Error() << bundle_node;
    return 1;
  }

  rego::Bundle bundle = rego::BundleDef::from_node(bundle_node);

  // we can also assemble the input term manually
  rego::Node input = rego::object({
    rego::object_item(rego::string("a"), rego::number(10)),
    rego::object_item(rego::string("b"), rego::string("20")),
    rego::object_item(rego::string("c"), rego::number(30.0)),
    rego::object_item(rego::string("d"), rego::boolean(true)),
  });

  rego.set_input(input);

  std::cout << rego.output_to_string(rego.query_bundle(bundle)) << std::endl;

  // we can also query bundle entrypoints directly
  std::cout << rego.output_to_string(rego.query_bundle(bundle, "objects/sites"))
            << std::endl;
}
