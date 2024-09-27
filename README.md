# `rego-cpp`

This project is an effort to create a C++ interpreter for the OPA policy language,
[Rego](https://www.openpolicyagent.org/docs/latest/policy-language/). Our goal is
to build both a standalone executable and a library such that programmers who are working
in C++ can interpret Rego programs natively. To achieve this we are building our
interpreter on top of the experimental term rewriter
[Trieste](https://github.com/microsoft/trieste).

> **Warning**
> While this project has progressed to the point that we support full Rego language
> (see [Language Support](#language-support) below) we do not support all built-ins.
> That said, we have verified compliance with the OPA Rego test suite. Even so, it
> should still be considered experimental software and used with discretion.

## Getting Started

Start by installing [CMake](https://cmake.org/) in the way appropriate for your
environment.

### Linux

Create a build directory and initialize the cmake project:

    mkdir build
    cd build
    cmake .. --preset release-clang

You can then build and run the tests using:

    ninja install
    ctest

### Windows

Create a build directory and initialize the cmake project:

    mkdir build
    cd build
    cmake .. --preset release

You can then build and run the tests using:

    cmake --build . --config Release --target INSTALL
    ctest -C Release

### Using the `rego` CLI

The interpreter tool will be located at `build/dist/bin/rego`. Here are
some example commands using the provided example files and run from the suggested
`dist` install directory:

    ./bin/rego -d examples/scalars.rego -q data.scalars.greeting
    {"expressions":["Hello"]}

    ./bin/rego -d examples/objects.rego -q data.objects.sites[1].name
    {"expressions":["smoke1"]}

    ./bin/rego -d examples/data0.json examples/data1.json examples/objects.rego -i examples/input0.json  -q "[data.one, input.b, data.objects.sites[1]]"
    {"expressions":[[{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]]}

    ./bin/rego -q "5 + (2 - 4 * 0.25) * -3 + 7.4"
    {"bindings":{"x":5, "y":9.4}}

    ./bin/rego -d examples/bodies.rego -i examples/input1.json -q data.bodies.e
    {"expressions":[{"one":15, "two":15}]}

You can run the test driver from the same directory:

    ./bin/rego_test tests/regocpp.yaml

### Using the `rego` Library

See the [examples](examples/README.md) directory for examples of how to use the
library from different langauages.

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
- `json*` (except `jsonbuiltins`)
- `jwt*`
- `net*`
- `planner-ir`
- `providers-aws`

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
