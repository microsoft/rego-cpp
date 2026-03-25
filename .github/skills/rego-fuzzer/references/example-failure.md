## Example: Fuzzer Failure Output

This is a real fuzzer failure output from `rego_to_bundle` with seed `1452196526`.
It demonstrates the structure of failure output for diagnosing issues.

### Command

```bash
./tools/rego_fuzzer rego_to_bundle -c 1 -s 1452196526
```

### Output

```
Testing x1, seed: 1452196526

: unexpected rego-templatestring, expected a rego-STRING, rego-INT, rego-FLOAT,
rego-true, rego-false or rego-null                                              $85
~~~
(rego-templatestring)


============
Pass: index_strings_locals, seed: 1452196526
------------
(top
  {}
  (rego-bundle
    {
      $6 = rego-basedocument
      $86 =
        rego-virtualdocument
        rego-virtualdocument
      h = rego-function
      tegj = rego-function}
    (rego-basedocument
      {}
      (rego-ident 2:$6)
      (rego-baseobject
        {... symbol table ...}
        (rego-baseobjectitem
          (rego-ident 3:$17)
          (rego-dataterm
            (rego-scalar
              (rego-true))))
        ...
        (rego-baseobjectitem
          (rego-ident 3:$82)
          (rego-dataterm
            (rego-scalar
              (rego-templatestring))))))  <-- the offending node
    ...
    (rego-modulefileseq)))
------------
(top
  {}
  (rego-bundle
    {
      h = rego-function
      tegj = rego-function}
    (rego-data
      (rego-object
        ...
        (rego-objectitem
          (rego-term
            (rego-scalar
              (rego-STRING 3:$82)))
          (rego-term
            (rego-scalar
              (rego-templatestring))))))  <-- still present in output
    ...
    (rego-modulefileseq)))
============
Failed pass: index_strings_locals, seed: 1452196526
```

### Output Structure

The failure output has this structure:

1. **Header**: `Testing xN, seed: S`
2. **WF error message**: Describes the well-formedness violation — which node type was
   found and what types were expected. The `$NN` is the node id, `~~~` underlines the
   error location, and the indented `(rego-X)` shows the offending node.
3. **Separator**: `============`
4. **Pass identification**: `Pass: <pass_name>, seed: <seed>`
5. **Input AST**: The full AST that was fed **into** the failing pass (between `---` separators)
6. **Output AST**: The full AST the pass **produced** (between `---` and `===` separators)
7. **Failure summary**: `Failed pass: <pass_name>, seed: <seed>`

### How to Read This Example

- The **error message** says `rego-templatestring` was unexpected inside a `Scalar` — only
  `STRING`, `INT`, `FLOAT`, `true`, `false`, and `null` are valid there according to the
  output WF of the `index_strings_locals` pass.
- The **input AST** shows where the `rego-templatestring` entered the pass: nested inside
  `(rego-dataterm (rego-scalar (rego-templatestring)))` in the base document data.
- The **output AST** shows that the pass propagated the `rego-templatestring` through to
  its output unchanged, violating the output WF.
- The **fix** would be to add a rewrite rule or error rule in the `index_strings_locals`
  pass to handle the `TemplateString` node type.

### Successful Output (for comparison)

A successful run produces only the header line and exits with code 0:

```
Testing x3, seed: 42
```
