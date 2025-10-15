# Examples

This directory contains examples of how to use the library from the different
languages we support (other than C++. For C++ examples see the
[tests](../tests) or [tools](../tools) directories).

At the moment we support C, C++, Rust, Python, and dotnet. The C API has been
designed to make it straightforward to wrap for other languages. To see some
examples, look at the [wrappers directory](../wrappers).

## C++
You can begin by looking at [`example.cc`](./cpp/example.cc) to see some sample
usage of the API. [`custom_builtin.cc`](./cpp/custom_builtin.cc) shows how to
add your own custom builtins (only supported in C++ at this point in time).

From there, the best way to get a more in-depth understanding of the C++ API
is to look at the [rego](../tools/main.cc) tool.

## C
There are two C examples. The first, [example.c](c/example.c) exercises the C API,
showing all the various ways it can be used. The second C example is a
[command line tool](c/main.c) that takes one or more
Rego data, module, and input files and evaluates a query against them.

To build it on Linux:
```bash
a@host:rego-cpp$ cd examples/c
a@host:c$ mkdir build
a@host:c$ cd build
a@host:build$ cmake .. --preset release-clang
a@host:build$ ninja install
```

and on Windows:

```cmd
cd examples\c
mkdir build
cd build
cmake .. --preset release
cmake --build . --config Release --target INSTALL
```

The resulting executable can be called from within the `dist` directory:

```bash
a@host:build$ cd dist
a@host:dist$ ./bin/regoc -d examples/scalars.rego -q data.scalars.greeting
{"expressions":["Hello"]}
```

## Python
The Python example is a [simple command line tool](python/rego.py) that takes zero or more
Rego data, module, and input files and evaluates a query against them.

Examples:

```bash
a@host:python$ pip install regopy
a@host:python$ python rego.py eval -d examples/scalars.rego data.scalars.greeting
{"expressions":["Hello"]}

a@host:python$ python rego.py eval -d examples/objects.rego data.objects.sites[1].name
{"expressions":["smoke1"]}

a@host:python$ python rego.py eval -d examples/data0.json -d examples/data1.json -d examples/objects.rego -i examples/input0.json "[data.one, input.b, data.objects.sites[1]]"
{"expressions":[[{"bar":"Foo", "baz":5, "be":true, "bop":23.4},"20",{"name":"smoke1"}]]}

a@host:python$ python rego.py eval "x=5; y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5"
{"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}

a@host:python$ python rego.py eval -d examples/bodies.rego -i examples/input1.json data.bodies.e
{"expressions":[{"one":15, "two":15}]}
```


## Rust
The Rust example is another [simple command line tool](rust/src/main.rs) that takes zero or more
Rego data, module, and input files and evaluates a query against them.

Examples:

```bash
a@host:rust$ cargo run -- eval -d examples/scalars.rego data.scalars.greeting
{"expressions":["Hello"]}

a@host:rust$ cargo run -- eval -d examples/objects.rego data.objects.sites[1].name
{"expressions":["smoke1"]}

a@host:rust$ cargo run -- eval -d examples/data0.json -d examples/data1.json -d examples/objects.rego -i examples/input0.json "[data.one, input.b, data.objects.sites[1]]"
{"expressions":[[{"bar":"Foo", "baz":5, "be":true, "bop":23.4},"20",{"name":"smoke1"}]]}

a@host:rust$ cargo run -- eval "x=5; y=x + (2 - 4 * 0.25) * -3 + 7.4;5 * 2"
{"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}

a@host:rust$ cargo run -- eval -d examples/bodies.rego -i examples/input1.json data.bodies.e
{"expressions":[{"one":15, "two":15}]}
```

## dotnet
The [dotnet example](dotnet/Program.cs) does not provide a full command-line tool,
but it does show how use all the features of the wrapper:

```bash
a@host:dotnet$ dotnet run
Query Only
{"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}
x = 5
10

Input and Data
{"expressions":[true], "bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4},"20",{"name":"smoke1"}]}}

Bundles
query: {"expressions":[true], "bindings":{"x":4460}}
example/foo: {"expressions":[2275]}
```
