use regorust::{BundleFormat, Input, Interpreter};
use std::path::Path;

fn main() {
    //////////////////////
    ///// Query Only /////
    //////////////////////

    println!("Query Only");

    // You can run simple queries without any input or data.
    let rego = Interpreter::new();
    let output = rego
        .query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5")
        .expect("Failed to evaluate query");
    println!("{}", output);
    // {"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}

    // You can access bound results using the binding method
    let x = output.binding("x").expect("x is not bound");
    println!("x = {}", x.json().unwrap());

    // You can also access expressions by index
    let exprs = output.expressions().expect("no expressions");
    println!("{}", exprs.index(2).unwrap().json().unwrap());

    println!();

    ////////////////////////
    //// Input and Data ////
    ////////////////////////

    println!("Input and Data");

    // You can provide JSON data directly.
    rego.add_data_json(
        r#"{
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
    }"#,
    )
    .expect("Failed to add data");

    rego.add_data_json(
        r#"{
        "three": {
            "bar": "Baz",
            "baz": 15,
            "be": true,
            "bop": 4.23
        }
    }"#,
    )
    .expect("Failed to add data");

    rego.add_module(
        "objects.rego",
        r#"package objects

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
"#,
    )
    .expect("Failed to add module");

    // Inputs can be either JSON or Rego, and provided as text.
    rego.set_input_json(
        r#"{
        "a": 10,
        "b": "20",
        "c": 30.0,
        "d": true
    }"#,
    )
    .expect("Failed to set input");

    println!(
        "{}",
        rego.query("[data.one, input.b, data.objects.sites[1]] = x")
            .expect("Failed to evaluate query")
    );
    // {"bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]}}

    ///////////////////
    ///// Bundles  ////
    ///////////////////

    println!();
    println!("Bundles");

    // If you want to run the same set of queries against a policy with different
    // inputs, you can create a bundle and use that to save the cost of compilation.

    let rego_build = Interpreter::new();

    rego_build
        .add_data_json(
            r#"{"a": 7,
"b": 13}"#,
        )
        .expect("Failed to add data");

    rego_build
        .add_module(
            "example.rego",
            r#"package example

foo := data.a * input.x + data.b * input.y
bar := data.b * input.x + data.a * input.y
"#,
        )
        .expect("Failed to add module");

    // We can specify both a default query, and specific entry points into the policy
    // that should be made available to use later.
    let bundle = rego_build
        .build(
            &Some("x=data.example.foo + data.example.bar"),
            &["example/foo", "example/bar"],
        )
        .expect("Failed to build bundle");

    // We can now save the bundle to disk
    let bundle_path = Path::new("bundle");
    rego_build
        .save_bundle(bundle_path, &bundle, BundleFormat::JSON)
        .expect("Failed to save bundle");

    // And load it again
    let rego_run = Interpreter::new();
    rego_run
        .load_bundle(bundle_path, BundleFormat::JSON)
        .expect("Failed to load bundle");

    // The most efficient way to provide input to a policy is by constructing it
    // manually, without the need for parsing JSON or Rego.
    let input = Input::new()
        .str("x")
        .int(104)
        .objectitem()
        .str("y")
        .int(119)
        .objectitem()
        .object(2)
        .validate()
        .expect("Failed to create input");

    rego_run.set_input(&input).expect("Failed to set input");

    // We can query the bundle, which will use the entrypoint of the default query
    // provided at build
    println!(
        "query: {}",
        rego_run
            .query_bundle(&bundle)
            .expect("Failed to query bundle")
    );
    // query: {"expressions":[true], "bindings":{"x":4460}}

    // Or we can query specific entrypoints
    println!(
        "example/foo: {}",
        rego_run
            .query_bundle_entrypoint(&bundle, "example/foo")
            .expect("Failed to query bundle entrypoint")
    );
    // example/foo: {"expressions":[2275]}
}
