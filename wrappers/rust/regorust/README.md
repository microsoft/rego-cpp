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

> **Warning**
> While this project has progressed to the point that we support full Rego language
> (see [Language Support](#language-support) below) we do not support all built-ins.
> That said, we have verified compliance with the OPA Rego test suite. Even so, it
> should still be considered experimental software and used with discretion.

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

We support v0.68.0 of Rego as defined by OPA, with the following grammar:

```ebnf
module          = package { import } policy
package         = "package" ref
import          = "import" ref [ "as" var ]
policy          = { rule }
rule            = [ "default" ] rule-head { rule-body }
rule-head       = ( ref | var ) ( rule-head-set | rule-head-obj | rule-head-func | rule-head-comp )
rule-head-comp  = [ assign-operator term ] [ "if" ]
rule-head-obj   = "[" term "]" [ assign-operator term ] [ "if" ]
rule-head-func  = "(" rule-args ")" [ assign-operator term ] [ "if" ]
rule-head-set   = "contains" term [ "if" ] | "[" term "]"
rule-args       = term { "," term }
rule-body       = [ "else" [ assign-operator term ] [ "if" ] ] ( "{" query "}" ) | literal
query           = literal { ( ";" | ( [CR] LF ) ) literal }
literal         = ( some-decl | expr | "not" expr ) { with-modifier }
with-modifier   = "with" term "as" term
some-decl       = "some" term { "," term } { "in" expr }
expr            = term | expr-call | expr-infix | expr-every | expr-parens | unary-expr
expr-call       = var [ "." var ] "(" [ expr { "," expr } ] ")"
expr-infix      = expr infix-operator expr
expr-every      = "every" var { "," var } "in" ( term | expr-call | expr-infix ) "{" query "}"
expr-parens     = "(" expr ")"
unary-expr      = "-" expr
membership      = term [ "," term ] "in" term
term            = ref | var | scalar | array | object | set | membership | array-compr | object-compr | set-compr
array-compr     = "[" term "|" query "]"
set-compr       = "{" term "|" query "}"
object-compr    = "{" object-item "|" query "}"
infix-operator  = assign-operator | bool-operator | arith-operator | bin-operator
bool-operator   = "==" | "!=" | "<" | ">" | ">=" | "<="
arith-operator  = "+" | "-" | "*" | "/" | "%"
bin-operator    = "&" | "|"
assign-operator = ":=" | "="
ref             = ( var | array | object | set | array-compr | object-compr | set-compr | expr-call ) { ref-arg }
ref-arg         = ref-arg-dot | ref-arg-brack
ref-arg-brack   = "[" ( scalar | var | array | object | set | "_" ) "]"
ref-arg-dot     = "." var
var             = ( ALPHA | "_" ) { ALPHA | DIGIT | "_" }
scalar          = string | NUMBER | TRUE | FALSE | NULL
string          = STRING | raw-string
raw-string      = "`" { CHAR-"`" } "`"
array           = "[" term { "," term } "]"
object          = "{" object-item { "," object-item } "}"
object-item     = ( scalar | ref | var ) ":" term
set             = empty-set | non-empty-set
non-empty-set   = "{" term { "," term } "}"
empty-set       = "set(" ")"
```

> [!NOTE]
> This grammar corresponds to Rego with `rego.v1` enabled (See [OPA v1.0](https://www.openpolicyagent.org/docs/latest/opa-1) for more info).

Definitions:
```
[]     optional (zero or one instances)
{}     repetition (zero or more instances)
|      alternation (one of the instances)
()     grouping (order of expansion)
STRING JSON string
NUMBER JSON number
TRUE   JSON true
FALSE  JSON false
NULL   JSON null
CHAR   Unicode character
ALPHA  ASCII characters A-Z and a-z
DIGIT  ASCII characters 0-9
CR     Carriage Return
LF     Line Feed
```

### Builtins

At the moment support the following builtins are available:

- `aggregates`
- `arrays`
- `bits`
- `casts`
- `encoding`
- `graphs`
- `numbers`
- `objects`
- `regex`
- `semver`
- `sets`
- `strings`
- `time`
- `types`
- `units`
- `uuid`
- miscellaneous
    * `opa.runtime`
    * `print`

### Compatibility with the OPA Rego Go implementation

Our goal is to achieve and maintain full compatibility with the reference Go
implementation. We have developed a test driver which runs the same tests
and validates that we produce the same outputs. At this stage we pass all
the non-builtin specific test suites, which we clone from the
[OPA repository](https://github.com/open-policy-agent/opa/tree/main/test/cases/testdata).
To build with the OPA tests available for testing, use one of the following presets:
- `release-clang-opa`
- `release-opa`

At present, we are **NOT** passing the following test suites in full:
- `crypto*`
- `glob*`
- `graphql`
- `invalidkeyerror`
- `json*`  (except `jsonbuiltins`)
- `jwt*`
- `net*`
- `planner-ir`
- `providers-aws`
