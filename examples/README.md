# Examples

This directory contains examples of how to use the library from the different
languages we support (other than C++. For C++ examples see the
[tests](../tests) or [tools](../tools) directories).

At the moment we support C and C++ only, but we plan to support Rust, Python,
and .NET wrappers, with more as demand dictates.

## C++
The best way to get started with the C++ API is to look at the
[rego](../tools/main.cc) tool. It exercises most of the API and is a good
starting point for understanding how to use the library.

A more in-depth example which performs node matching and other more complex
operations can be see in the [`TestCase`](../tests/test_case.cc) class from the
test driver.

## C
The C example is a [simple command line tool](c/main.c) that takes one or more
Rego data, module, and input files and evaluates a query against them.

To build it on Linux:
```bash
cd examples/c
mkdir build
cd build
cmake .. --preset release-clang
ninja install
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
a@host:~/build$ cd dist
a@host:~/build/dist$ ./bin/regoc -d examples/scalars.rego -q data.scalars.greeting
"Hello"
```

## Python
The Python example is a [simple command line tool](python/rego.py) that takes one or more
Rego data, module, and input files and evaluates a query against them.

Examples:

```bash
a@host:~/$ pip install regopy
a@host:~/$ python rego.py -d examples/scalars.rego data.scalars.greeting
"Hello"

a@host:~/$ python rego.py -d examples/objects.rego data.objects.sites[1].name
"smoke1"

a@host:~/$ python rego.py -d examples/data0.json -d examples/data1.json -d examples/objects.rego -i examples/input0.json "[data.one, input.b, data.objects.sites[1]]"
[{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]

a@host:~/$ python rego.py "x=5; y=x + (2 - 4 * 0.25) * -3 + 7.4"
x = 5
y = 9.4

a@host:~/$ python rego.py -d examples/bodies.rego -i examples/input1.json data.bodies.e
{"one":15, "two": 15}
```