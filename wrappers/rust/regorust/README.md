# `regorust`

[Rego](https://www.openpolicyagent.org/docs/latest/policy-language/)
is the native query language of the Open Policy Agent project. If you want to
learn more about Rego as a language, and its various use cases, we refer
you to the language documentation above which OPA provides.

This crate is a wrapper around `rego-cpp`, an open source cross-platform C++
implementation of the Rego language compiler and runtime developed and maintained
by Microsoft. You can learn more about that project
[here](https://github.com/microsoft/rego-cpp). As much as possible in this
wrapper we try to provide idiomatic Rust interfaces to the Rego query engine.
We hope the project is of use to those wishing to leverage the power of Rego
within a Rust context.
