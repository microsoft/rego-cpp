# TODO

This README tracks the current state of the grammar we support, and also planning
and brainstorming information. The work plan has been moved to Github issues/milestones.

## Current Supported Grammar

```ebnf
module          = package policy
package         = "package" var
policy          = { rule }
rule            = [ "default" ] rule-head { rule-body }
rule-head       = var ( rule-head-set | rule-head-obj | rule-head-func | rule-head-comp )
rule-head-comp  = [ assign-operator expr ] [ "if" ]
rule-head-obj   = "[" term "]" [ assign-operator term ] [ "if" ]
rule-head-func  = "(" rule-args ")" [ assign-operator term ] [ "if" ]
rule-head-set   = "contains" term [ "if" ] | "[" term "]"
rule-args       = term { "," term }
rule-body       = [ "else" [ assign-operator term ] [ "if" ] ] ( "{" query "}" ) | literal
rule-body-item  = expr | rule
query           = literal { ( ";" | ( [CR] LF ) ) literal }
literal         = ( some-decl | expr | "not" expr )
some-decl       = "some" term { "," term } { "in" expr }
expr            = term | expr-call | expr-infix | expr-parens | unary-expr
expr-call       = var [ "." var ] "(" [ expr { "," expr } ] ")"
expr-infix      = expr infix-operator expr
expr-parens     = "(" expr ")"
unary-expr      = "-" expr
term            = ref | var | scalar | array | object | set
infix-operator  = assign-operator | bool-operator | arith-operator
bool-operator   = "==" | "!=" | "<" | ">" | ">=" | "<="
arith-operator  = "+" | "-" | "*" | "/" | "%"
assign-operator = ":=" | "="
ref             = ( var | expr-call ) { ref-arg }
ref-arg         = ref-arg-dot | ref-arg-brack
ref-arg-brack   = "[" ( scalar | var | array | object | set ) "]"
ref-arg-dot     = "." var
var             = ( ALPHA | "_" ) { ALPHA | DIGIT | "_" }
scalar          = string | NUMBER | TRUE | FALSE | NULL
string          = STRING | raw-string
raw-string      = "`" { CHAR-"`" } "`"
array           = "[" expr { "," expr } "]"
object          = "{" object-item { "," object-item } "}"
object-item     = (scalar | ref | var) ":" expr
set             = empty-set | non-empty-set
non-empty-set   = "{" expr { "," expr } "}"
empty-set       = "set(" ")"
```

## v0.52.0 Grammar

This is the grammar we are targeting:

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

## Implementation questions

1. Missing/incorrect grammar items
    - expr vs term
    - modulo operator
2. What should the correct behavior be for floating point modulo? (currently undefined)
3. Why are global rule references not allowed in rule function definitions?
4. Object unifications: why aren't vars allowed as keys? Why are additional keys not allowed which are not unified?
5. Why can you not chain else-if statements but you can chain else statements with bodies?

## Unification

Once all the `AssignInfix` nodes are constructed correctly (after the `assign` pass) we
can begin to flatten assign statements to end up with a simple sequence of unification
statements. 

|                   expression                  |                                             flattened form                                                              |        method        |
| --------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- | -------------------  |
| `a = <term>`                                  | `a = <term>`                                                                                                            | noop                 |
| `<term> = a`                                  | `a = <term>`                                                                                                            | swap                 |
| `[a, b] = [3, 4]`                             | <code>a = 3<br>b = 4</code>                                                                                             | split                |
| `[a, 3] = [5, b]`                             | <code>a = 5<br>3 = b</code>                                                                                             | split                |
| `{"one": a, "two": b} = {"one": 3, "two": 4}` | <code>a = 3<br>b = 4</code>                                                                                             | decompose            |
| `{a: 4, b: 3} = {"one": 3, "two": 4}`         | <code>foo = {"one": 3, "two": 4}<br>t_0 = foo[i_0]<br>t_0 = 4<br>a = i_0<br>t_1 = foo[i_1]<br>t_1 = 3<br>b = i_1</code> | (unsupported)        |
| `[a, b] = array[i]`                           | <code>foo = array[i]<br>a = foo[0]<br>b = foo[1]</code>                                                                 | indirect             |
| `{"one": a, "two": b} = array[i]`             | <code>foo = array[i]<br>a = foo["one"]<br>b = foo["two"]</code>                                                         | indirect + decompose |
| `{a: 4, b: 3} = array[i]`                     | <code>foo = array[i]<br>t_0 = foo[i_0]<br>t_0 = 4<br>a = i_0<br>t_1 = foo[i_1]<br>t_1 = 3<br>b = i_1</code>             | (unsupported)        |
| `foo[[a, 5]]`                                 | <code>x = foo[i]<br>[a, 5] = x</code>                                                                                   | add index            |
