# Rego Bundle Binary Format      {#rbb}

The OPA standard for bundles is a JSON document, as described in the
[IR documentation](https://www.openpolicyagent.org/docs/ir). However, this
JSON can be bulky and quite cumbersome to parse. We have designed a binary
format which is more lightweight while retaining the same information and
basic layout. The pseudo-BNF grammar below describes the format. This format
is implemented in [`bundle_binary.cc`](src/bundle_binary.cc).

## Basic Types

The following basic types are used as terminals in the rest of the grammar. Each
type must be serialised in little-endian format.

|                    |                                           |
|--------------------|-----------------------------------------------------------------|
| `byte`             | 1 byte (8-bits)                                                 |
| `signed_byte(n)`   | 8-bit, two's complement signed integer for which the value is n |
| `unsigned_byte(n)` | 8-bit unsigned integer for which the value is n                 |
| `uint16`           | 2 bytes (16-bit unsigned integer)                               |
| `int32`            | 4 bytes (32-bit signed integer, two's complement)               |
| `uint32`           | 4 bytes (32-bit unsigned integer)                               |
| `int64`            | 8 bytes (64-bit signed integer, two's complement)               |
| `uint64`           | 8 bytes (64-bit unsigned integer)                               |
| `double`           | 8 bytes (64-bit IEEE 754-2008 binary floating point)            |
| `decimal128`       | 16 bytes (128-bit IEEE 754-2008 decimal floating point)         |
| `hex(s)`           | raw binary sequence that matches s                              |
| `loc(id)`          | `uint64` location of non-terminal `id` in the bundle            |
| `bson`             | a [BSON](https://bsonspec.org/spec.html) document               |
| `local`            | 4 bytes (32-bit unsigned integer) local index                   |

## Grammar

The following specifies the rest of the grammar. Note that we use the * operator
as shorthand for repetition (e.g. (`byte*2`) is `byte byte`). When used as a unary
operator, * means that the repetition can occur 0 or more times. We will begin with a
pseudo-BNF description of the overall format, and then will provide additional details
on the `header` token. Some notes for reading the grammar:

- In most cases, the context is given as a comment in the grammar where needed
- Unless otherwise specified, a `uint*` before a `*_list` token is the number of tokens in that list
- The `table` token in `plans` and `functions` gives the locations of individual plans and functions
  in the file.

```
bundle ::= header static plans funcs data
header ::= hex(0x5245474F42554E) uint8*8 uint32 uint32 uint64 loc(static) loc(plans) loc(funcs) loc(data)
bundle_crc32 ::= uint32
static ::= signed_byte(1) files strings builtin_funcs query
strings ::= uint32 loc_list
builtin_funcs ::= uint32 bf_list
query ::= signed_byte(1)                                                        # Undefined
          signed_byte(2) string                                                 # Query string
bf_list ::= builtin_func bf_list | ""
builtin_func ::= string builtin_decl                                            # name of function, decl
builtin_decl ::= builtin_args builtin_return
builtin_args ::= signed_byte(1)                                                 # Var args
               | signed_byte(2) uint8 ba_list
ba_list ::= builtin_arg ba_list | ""
builtin_result ::= signed_byte(1)                                               # Void
                   signed_byte(2) string string builtin_type
builtin_arg ::= string string builtin_type                                      # name, description, type
builtin_type ::= signed_byte(1)                                                 # Any
               | signed_byte(2)                                                 # Number
               | signed_byte(3)                                                 # String
               | signed_byte(4)                                                 # Boolean
               | signed_byte(5)                                                 # Null
               | signed_byte(6) builtin_type                                    # Array (dynamic)
               | signed_byte(7) byte bt_list                                    # Array (static)
               | signed_byte(8) builtin_type builtin_type                       # Object (dynamic)
               | signed_byte(9) byte bkv_list                                   # Object (static)
               | signed_byte(10) builtin_type builtin_type byte bkv_list        # Object (hybrid)
               | signed_byte(11) builtin_type                                   # Set
               | signed_byte(12) byte bt_list                                   # TypeSeq
bt_list ::= builtin_type bt_list | ""
bkv_list ::= builtin_type builtin_type bkv_list | ""
files ::= uint32 f_list
f_list ::= file f_list | ""
file ::= string string                                                          # name, contents
str_list ::= string str_list | ""
string ::= uint32 byte*
plans ::= signed_byte(2) uint32 p_list table
table ::= uint64 uint32 loc_list
loc_list ::= loc_index loc_list | ""
loc_index ::= string uint64                                                     # name of item, location in file
p_list ::= plan p_list | ""
plan ::= string blocks                                                          # name, blocks
blocks ::= uint32 b_list
b_list ::= block b_list | ""
block ::= uint32 s_list
s_list ::= statement s_list | ""
statement ::= signed_byte(1) source local operand                               # ArrayAppendStmt(array, value)
            | signed_byte(2) source int64 local                                 # AssignIntStmt(value, target)
            | signed_byte(3) source operand local                               # AssignVarOnceStmt(source, target)
            | signed_byte(4) source operand local                               # AssignVarStmt(source, target)
            | signed_byte(5) source blocks                                      # BlockStmt(blocks)
            | signed_byte(6) source uint32                                      # BreakStmt(index)
            | signed_byte(7) source operands operands local                     # CallDynamicStmt(path, args, result)
            | signed_byte(8) source string operands local                       # CallStmt(func, args, result)
            | signed_byte(9) source operand operand local                       # DotStmt(source, key, target)
            | signed_byte(10) source operand operand                            # EqualStmt(a, b)
            | signed_byte(11) source operand                                    # IsArrayStmt(source)
            | signed_byte(12) source local                                      # IsDefinedStmt(source)
            | signed_byte(13) source operand                                    # IsObjectStmt(source)
            | signed_byte(14) source operand                                    # IsSetStmt(source)
            | signed_byte(15) source local                                      # IsUndefinedStmt(source)
            | signed_byte(16) source operand local                              # LenStmt(source, target)
            | signed_byte(17) source int32 local                                # MakeArrayStmt(capacity, target)
            | signed_byte(18) source local                                      # MakeNullStmt(target)
            | signed_byte(19) source int64 local                                # MakeNumberIntStmt(value, target)
            | signed_byte(20) source int32 local                                # MakeNumberRefStmt(index, target)
            | signed_byte(21) source local                                      # MakeObjectStmt(target)
            | signed_byte(22) source local                                      # MakeSetStmt(target)
            | signed_byte(23) source operand operand                            # NotEqualStmt(a, b)
            | signed_byte(24) source block                                      # NotStmt(block)
            | signed_byte(25) source operand operand local                      # ObjectInsertOnceStmt(key, value, object)
            | signed_byte(26) source operand operand local                      # ObjectInsertStmt(key, value, object)
            | signed_byte(27) source local local local                          # ObjectMergeStmt(a, b, target)
            | signed_byte(28) source local                                      # ResetLocalStmt(target)
            | signed_byte(29) source local                                      # ResultSetAddStmt(value)
            | signed_byte(30) source local                                      # ReturnLocalStmt(source)
            | signed_byte(31) source local local local block                    # ScanStmt(source, key, value, block)
            | signed_byte(32) source operand local                              # SetAddStmt(value, set)
            | signed_byte(33) source local ipath operand block                  # WithStmt(local, path, value, block)
source ::= signed_byte(1)                                                       # no source
         | signed_byte(2) uint32 uint32 uint32                                  # file index, position in file, length of string
operand ::= signed_byte(1) local                                                # local
            signed_byte(2) uint32                                               # string index
            signed_byte(3)                                                      # boolean (true)
            signed_byte(4)                                                      # boolean (false)
operands ::= uint8 o_list
ipath ::= uint8 i32_list
i32_list ::= int32 i32_list | ""
o_list ::= operand o_list | ""
funcs ::= signed_byte(3) uint32 f_list table
f_list ::= function f_list | ""
function ::= string path params local blocks                                    # name, dynamic path, parameters, return, blocks
path ::= uint8 str_list
params ::= uint8 id_list
id_list ::= local id_list | ""
data ::= signed_byte(4) bson
```

### `header`

The header consists of the following elements:

|         Name        |  Token   |                            Description                                        |
|---------------------|----------|-------------------------------------------------------------------------------|
| Magic Word          | byte*8   |  `REGOBUND`                                                                   |
| Rego Version        | byte     | The major version of Rego required to execute the bundle.                     |
| Rego Binary Version | byte     | The version of the Rego Binary format used to encode the file                 |
| Query Plan Index    | sbyte    | The index of the plan that represents the query. -1 if no query was compiled. |
| Reserved            | 5 * byte | Reserved header bytes                                                         |
| Local Count         | uint32   | Number of locals variables in the program.                                    |
| CRC32               | uint32   | CRC32 of everything after the header                                          |
| Size                | uint64   | Size of the bundle file (not including the header)                            |
| `loc([token])`      | uint64   | Location of `token` within the file as number of bytes from the start.        |
