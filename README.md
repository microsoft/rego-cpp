# `rego-cpp`

This project is an effort to create a C++ interpreter for the OPA policy language,
[Rego](https://www.openpolicyagent.org/docs/latest/policy-language/). Our goal is
to build both a standalone executable and a library such that programmers who are working
in C++ can interpret Rego programs natively. To achieve this we are building our
interpreter on top of the experimental term rewriter
[Trieste](https://github.com/microsoft/trieste).

> **Warning**
> While this project has progressed to the point that we support full Rego language
> (see [Language Support](#language-support) below) we do not support all built-ins
> and do not yet have a system built to verify compatibility with the reference Go
> implementation. As such, it should still be considered experimental software and
> used with discretion.

## Getting Started

Start by installing [CMake](https://cmake.org/) in the way appropriate for your
environment.

### Linux

> **Note**
> At the moment, you must use `clang++` to build the project on Linux.

Create a build directory and initialize the cmake project:

    mkdir build
    cd build
    cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=dist -DREGOCPP_BUILD_TOOLS=1 -DREGOCPP_BUILD_TESTS=1

You can then build and run the tests using:

    make install
    ctest

If you wish to use Ninja, then be sure to pass `-G Ninja` as well when configuring the project.
Then:

    ninja install
    ctest

Similarly, if you want to use clang you can indicate this by passing `-DCMAKE_CXX_COMPILER=clang++`
during configuration.

### Windows

Create a build directory and initialize the cmake project:

    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=dist -DREGOCPP_BUILD_TOOLS=1 -DREGOCPP_BUILD_TESTS=1

You can then build and run the tests using:

    cmake --build . --config Release --target INSTALL
    ctest -C Release

### Using the Interpreter

The interpreter will be located at `build/dist/bin/rego_interpreter`. Here are
some example commands using the provided example files and run from the suggested
`dist` install directory:

    ./bin/rego_interpreter -d examples/scalars.rego -q data.scalars.greeting
    "Hello"

    ./bin/rego_interpreter -d examples/objects.rego -q data.objects.sites[1].name
    "smoke1"

    ./bin/rego_interpreter -d examples/data0.json examples/data1.json examples/objects.rego -i examples/input0.json  -q "[data.one, input.b, data.objects.sites[1]]"
    [{"bar": "Foo", "baz": 5, "be": true, "bop": 23.4}, "20", {"name": "smoke1"}]

    ./bin/rego_interpreter -q "5 + (2 - 4 * 0.25) * -3 + 7.4"
    9.4

    ./bin/rego_interpreter -d examples/bodies.rego -i examples/input1.json -q data.bodies.e
    {"one": 15, "two": 15}

## Language Support

At present we support v0.55.0 of the Rego grammar as defined by OPA:

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
arith-operator  = "+" | "-" | "*" | "/"
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

At the moment only support a few builtins, but are actively working on adding
all the standard builtins. The following builtins are currently supported:

- `aggregates`
- `arrays`
- `numbers`
- `sets`
- `strings`
- miscellaneous
    - `print`
    - `to_number`
    - `base64_encode`
    - `base64_decode`

### Compatibility with the OPA Rego Go implementation

Our goal is to achieve and maintain full compatibility with the reference Go
implementation. We have developed a test driver which runs the same tests
and validates that we produce the same outputs. We currently pass the following
test case suites:

- `aggregates`
- `arithmetic`
- `array`
- `comparisonexpr`
- `compositebasedereference`
- `helloworld`
- `indexing`
- `intersection`
- `numbersrange`
- `rand`
- `replacen`
- `sets`
- `sprintf`
- `strings`
- `trim`
- `trimleft`
- `trimprefix`
- `trimright`
- `trimspace`
- `trimsuffix`
- `union` 

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
