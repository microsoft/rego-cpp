# Built-in Function Implementation Pattern

When OPA adds new built-in functions, rego-cpp must provide matching implementations. This document shows the exact patterns used.

## File Structure

Each OPA namespace has a dedicated file in `src/builtins/`:
```
src/builtins/
├── builtins.hh          # Factory declarations
├── array.cc             # array.concat, array.reverse, array.slice
├── strings.cc           # strings.*, concat, contains, etc.
├── ...                  # one file per namespace
```

## Implementation Pattern

A built-in consists of three parts: behavior function, factory function, and namespace dispatch.

### 1. Behavior Function (anonymous namespace)

```cpp
#include "builtins.hh"
#include "rego.hh"

namespace
{
  using namespace rego;
  namespace bi = rego::builtins;

  Node my_func(const Nodes& args)
  {
    // Unwrap and validate arguments
    Node x = unwrap_arg(args, UnwrapOpt(0).func("namespace.my_func").type(Array));
    if (x->type() == Error)
    {
      return x;
    }

    // Implement the built-in logic
    // Return an AST node (Array, Object, Set, JSONString, Int, Float, True, False, Null)
    return result_node;
  }
}
```

### 2. Factory Function (anonymous namespace)

```cpp
namespace
{
  BuiltIn my_func_factory()
  {
    const Node my_func_decl = bi::Decl
      << (bi::ArgSeq
          << (bi::Arg
              << (bi::Name ^ "x")
              << (bi::Description ^ "description of x")
              << (bi::Type << bi::Any)))
      << (bi::Result
          << (bi::Name ^ "result")
          << (bi::Description ^ "description of result")
          << (bi::Type << bi::Any));
    return BuiltInDef::create({"namespace.my_func"}, my_func_decl, my_func);
  }
}
```

### 3. Namespace Dispatch Function

In the same file, inside `namespace rego::builtins`:

```cpp
namespace rego
{
  namespace builtins
  {
    BuiltIn my_namespace(const Location& name)
    {
      assert(name.view().starts_with("my_namespace."));
      std::string_view view = name.view().substr(13); // skip "my_namespace."
      if (view == "my_func")
      {
        return my_func_factory();
      }
      // ... more functions in this namespace
      return nullptr;  // unknown function
    }
  }
}
```

## Type Constants for Declarations

Available types for `bi::Type`:
- `bi::Any` — any type
- `bi::String`, `bi::Number`, `bi::Boolean`, `bi::Null` — scalars
- `bi::DynamicArray << (bi::Type << bi::Any)` — array of any
- `bi::StaticArray << (bi::Type << bi::String) << ...` — fixed-type array
- `bi::DynamicObject << (bi::Type << bi::String) << (bi::Type << bi::Any)` — object
- `bi::Set << (bi::Type << bi::Any)` — set

## Argument Unwrapping

```cpp
// By position and expected type:
Node arg = unwrap_arg(args, UnwrapOpt(0).func("name.func").type(ExpectedType));

// Multiple accepted types:
Node arg = unwrap_arg(args, UnwrapOpt(0).func("name.func").types({Array, Set}));
```

## Important: AST Node Wrapping

Array/Object/Set children are wrapped in Term nodes. **Never** compare `child->type()` directly:
```cpp
// WRONG — will not match because child is a Term wrapping an Array
if (child->type() == Array) { ... }

// CORRECT — unwrap handles the Term wrapper
auto maybe = unwrap(child, Array);
if (maybe.success) { ... use maybe.node ... }
```
Read the well-formedness definitions in each pass to understand the node structure.

## Adding to an Existing Namespace

1. Add behavior function and factory in the namespace's `.cc` file
2. Add a new `if` branch in the namespace dispatch function

## Adding a New Namespace

1. Create `src/builtins/<namespace>.cc` with the pattern above
2. Add declaration to `src/builtins/builtins.hh`:
   ```cpp
   BuiltIn my_namespace(const Location& name);
   ```
3. Add routing in `src/builtins.cc` `BuiltInsDef::lookup()` dispatch tree
4. Add source file to `src/CMakeLists.txt`

## Placeholder for Unsupported Built-ins

For built-ins that intentionally cannot be supported:

```cpp
BuiltIn http(const Location& name)
{
  // ...
  return BuiltInDef::placeholder(
    name, decl, "http.send is not supported");
}
```

## Internal / Compiler-Generated Built-ins

Some OPA built-ins are not user-facing — they are emitted by the compiler during
desugaring (e.g., `internal.template_string` for `$"..."` template strings).
These follow the same registration pattern but have distinct characteristics:

- **Name prefix**: `internal.` — routed through the `internal` namespace dispatch
- **Not in OPA's public built-in docs**: Discovered by inspecting OPA's IR plan output
- **Called from compiler-generated code only**: The desugaring pass in `src/rego_to_bundle.cc` (or a dedicated pass) emits `ExprCall` nodes that reference these functions
- **Argument conventions may differ**: Internal built-ins may receive pre-processed arguments (e.g., arrays with sentinel values like empty sets for undefined)

### Investigating Internal Built-ins

Use OPA's plan IR to discover the exact calling convention:
```bash
/tmp/opa build --bundle <dir> --target plan -e <entrypoint> -o bundle.tar.gz
tar xzf bundle.tar.gz && python3 -m json.tool plan.json
```

Look at `static.builtin_funcs` for the declaration and `funcs.funcs[].blocks` for actual call sites.

### Example: `internal.template_string`
- **Signature**: `internal.template_string(array[any]) -> string`
- **Array contents**: Interleaved literal string chunks and expression values
- **Undefined encoding**: Potentially-undefined expressions are wrapped in a set at the IR level (empty set = undefined → produces `"<undefined>"` in output; set with one element = defined value)
- **Stringification**: Each non-string element is stringified (JSON-like representation); strings are used raw (not quoted)
