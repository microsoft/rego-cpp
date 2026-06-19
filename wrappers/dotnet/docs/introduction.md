## What is Rego?

Rego is a language developed by [Open Policy Agent (OPA)](https://www.openpolicyagent.org/) for use in defining policies in cloud systems.

We think Rego is a great language, and you can learn more about it [from the OPA website](https://www.openpolicyagent.org/docs/latest/policy-language/). However, it has some limitations. Primarily, the interpreter is only accessible via the command line or a web server. Further, the only option for using the language in-process is via an interface in Go.

## Language Support

We support v1.18.1 of Rego as defined by OPA. For the full supported grammar, see the
[Language Support section of the main README](https://github.com/microsoft/rego-cpp#language-support).

### Builtins

We support the majority of the standard Rego built-ins, and provide a robust
mechanism for including custom built-ins (via the CPP API). The following builtins
are NOT supported at present:

- `crypto.x509.parse_and_verify_certificates_with_options` - Not yet implemented (no OPA conformance tests available)
- `graphql.*` - Not planned
- `http.send` - Not planned
- `json.match_schema`/`json.verify_schema` - Not planned
- `net.*` - Not planned
- `providers.aws.sign_req` - Not planned
- `rego.metadata.chain`/`rego.metadata.rule`/`rego.parse_module` - Not planned
- `strings.render_template` - Not planned
- `time` - This is entirely platform dependent at the moment, depending on whether
           there is a compiler on that platform which supports `__cpp_lib_chrono >= 201907L`.


### Compatibility with the OPA Rego Go implementation

Our goal is to achieve and maintain full compatibility with the reference Go
implementation. We have developed a test driver which runs the same tests
and validates that we produce the same outputs. At this stage we pass all
the non-builtin specific test suites, which we clone from the
[OPA repository](https://github.com/open-policy-agent/opa/tree/main/test/cases/testdata).
To build with the OPA tests available for testing, use one of the following presets:
- `release-clang-opa`
- `release-opa`