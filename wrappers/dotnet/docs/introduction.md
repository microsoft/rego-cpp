## What is Rego?

Rego is a language developed by [Open Policy Agent (OPA)](https://www.openpolicyagent.org/) for use in defining policies in cloud systems.

We think Rego is a great language, and you can learn more about it [from the OPA website](https://www.openpolicyagent.org/docs/latest/policy-language/). However, it has some limitations. Primarily, the interpreter is only accessible via the command line or a web server. Further, the only option for using the language in-process is via an interface in Go.

The rego-cpp project provides the ability to integrate Rego natively into a wider range of languages. We currrently support C, C++, Rust, Python and, via this package, .NET, and are largely compatible with v1.8.0 of the language. You can learn more about our implementation [on our Github page](https://github.com/microsoft/rego-cpp).
