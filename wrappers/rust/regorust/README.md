# `regorust`

[Rego](https://www.openpolicyagent.org/docs/latest/policy-language/)
is the native query language of the Open Policy Agent project. If you want to
learn more about Rego as a language, and its various use cases, we refer
you to the language documentation above which OPA provides.

This crate is a wrapper around `rego-cpp`, an open source cross-platform C++
implementation of the Rego language compiler and runtime developed and maintained
by Microsoft. You can learn more about that project
[here](https://github.com/microsoft/rego-cpp). As much as possible in this
wrapper we try to provide idiomatic Rust interfaces to the Rego query engine.
We hope the project is of use to those wishing to leverage the power of Rego
within a Rust context.

## Example Usage

```rust
use regorust::Interpreter;

fn main() {
    // The Interpreter is the main interface into the library
    let rego = Interpreter::new();
    match rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4") {
      Ok(result) => {
        let x = result.binding("x").expect("cannot get x");
        let y = result.binding("y").expect("cannot get y");
        println!("x = {}", x.json().unwrap());
        println!("y = {}", y.json().unwrap());
      }
      Err(e) => {
        panic!("error: {}", e);
      }
    }
}
```

```rust
use regorust::Interpreter;

fn main() {
  let input = r#"
    {
      "a": 10,
      "b": "20",
      "c": 30.0,
      "d": true
    }
  "#;
  let data0 = r#"
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
  "#;
  let data1 = r#"
    {
      "three": {
        "bar": "Baz",
        "baz": 15,
        "be": true,
        "bop": 4.23
      }
    }        
  "#;
  let module = r#"
    package objects
 
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
    f := e["dev"]                
  "#;
  let rego = Interpreter::new();
  rego.set_input_json(input);
  rego.add_data_json(data0);
  rego.add_data_json(data1);
  rego.add_module("objects", module);
  match rego.query("x=[data.one, input.b, data.objects.sites[1]]") {
    Ok(result) => {
      println!("{}", result.to_str().unwrap());
      let x = result.binding("x").expect("cannot get x");
      let data_one = x.index(0).unwrap();
      if let NodeValue::String(bar) = data_one
          .lookup("bar")
          .unwrap()
          .value()
          .unwrap()
      {
        println!("data.one.bar = {}", bar);
      }
    }
    Err(e) => {
      panic!("error: {}", e);
    }
  }
}
```

## Language Support

We support v1.18.1 of Rego as defined by OPA. For the full supported grammar, see the
[Language Support section of the main README](https://github.com/microsoft/rego-cpp#language-support).

### Builtins

We support the majority of the standard Rego built-ins, and provide a robust
mechanism for including custom built-ins (via the CPP API). The following builtins
are NOT supported at present:

- `crypto.x509.parse_and_verify_certificates_with_options` - Not yet implemented (no OPA conformance tests available)
- `graphql.*` - Not planned
- `http.send` - Not planned
- `json.match_schema`/`json.verify_schema` - Not planned
- `net.*` - Not planned
- `providers.aws.sign_req` - Not planned
- `rego.metadata.chain`/`rego.metadata.rule`/`rego.parse_module` - Not planned
- `strings.render_template` - Not planned
- `time` - This is entirely platform dependent at the moment, depending on whether
           there is a compiler on that platform which supports `__cpp_lib_chrono >= 201907L`.


### Compatibility with the OPA Rego Go implementation

Our goal is to achieve and maintain full compatibility with the reference Go
implementation. We have developed a test driver which runs the same tests
and validates that we produce the same outputs. At this stage we pass all
the non-builtin specific test suites, which we clone from the
[OPA repository](https://github.com/open-policy-agent/opa/tree/main/test/cases/testdata).
To build with the OPA tests available for testing, use one of the following presets:
- `release-clang-opa`
- `release-opa`