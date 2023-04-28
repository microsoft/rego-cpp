# `rego-cpp`

This project is an effort to create a C++ interpreter for the OPA policy language,
[Rego](https://www.openpolicyagent.org/docs/latest/policy-language/). Our goal is
to build both a standalone executable and a library such that programmers who are working
in C++ can interpret Rego programs natively. To achieve this we are building our
interpereter on top of the experimental term rewriter
[Trieste](https://github.com/microsoft/trieste).

> **Warning**
> This project is still in the early stages, and we do not currently support many
> language features.

## Getting Started

> **Note**
> At the moment we only support Linux, though we intend to support all major platforms
> once we support all the major language features.

First you will need to install the build dependencies in the manner appropriate to
your system:

1. `clang++`
2. `clang-format`
3. `ninja`
4. `cmake`

Then, create a build directory and initialize the cmake project:

    mkdir build
    cd build
    cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Debug

Then, use Ninja to build the project, after which you can run the tests:

    ninja install
    ctest

The interpreter will be located at `build/dist/bin/rego_interpreter`. Here are
some example commands using the provided example files:

    ./bin/rego_interpreter -d examples/scalars.rego -q data.scalars.greeting
    "Hello"

    ./bin/rego_interpreter -d examples/objects.rego -q data.objects.sites[1].name
    "smoke1"

    ./bin/rego_interpreter -d examples/data0.json -d examples/data1.json -d examples/objects.rego -d examples/input.json  -q "[data.one, input.b, data.objects.sites[data.objects.index]]"
    [{"bar": "Foo", "baz": 5, "be": true, "bop": 23.4}, undefined, {"name": "smoke1"}]

    ./bin/rego_interpreter -q "5 + (2 - 4 * 0.25) * -3 + 7.4"
    9.4

    ./bin/rego_interpreter -d examples/bodies.rego -q data.bodies.e
    {"one": 15, "two": 15}
    
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
