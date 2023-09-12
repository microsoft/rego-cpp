# `regopy`

[Rego](https://www.openpolicyagent.org/docs/latest/policy-language/)
is the native query language of the Open Policy Agent project. If you want to
learn more about Rego as a language, and its various use cases, we refer
you to the language documentation above which OPA provides.

This module is a wrapper around `rego-cpp`, an open source cross-platform C++
implementation of the Rego language compiler and runtime developed and maintained
by Microsoft. You can learn more about that project
[here](https://github.com/microsoft/rego-cpp). As much as possible in this
wrapper we try to provide idiomatic Python interfaces to the Rego query engine.
We hope the project is of use to those wishing to leverage the power of Rego
within a Python context.

> **Warning**
> While this project has progressed to the point that we support full Rego language
> (see [Language Support](#language-support) below) we do not support all built-ins.
> That said, we have verified compliance with the OPA Rego test suite. Even so, it
> should still be considered experimental software and used with discretion.

## Language Support

At present we support v0.55.0 of Rego as defined by OPA, with the following grammar:

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
- `bits`
- `casts`
- `numbers`
- `objects`
- `regex`
- `semver`
- `sets`
- `strings`
- `types`
- `units`
- miscellaneous
    * `base64_encode`
    * `base64_decode`
    * `json.marshal`
    * `opa.runtime`
    * `print`
    * `time.now_ns`

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
- `base64*`
- `crypto*`
- `glob*`
- `graphql`
- `invalidkeyerror`
- `json*`
- `jwt*`
- `net*`
- `planner-ir`
- `providers-aws`
- `reachable`
- `urlbuiltins`
- `walkbuiltin`
