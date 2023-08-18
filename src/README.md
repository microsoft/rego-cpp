# Notes

## Implementation questions

1. Missing/incorrect grammar items
    - expr vs term
    - modulo operator
2. What should the correct behavior be for floating point modulo? (currently undefined)
3. Why are global rule references not allowed in rule function definitions?
4. Object unifications: why aren't vars allowed as keys? Why are additional keys not allowed which are not unified?
5. Why can you not chain else-if statements but you can chain else statements with bodies?
6. Something weird seems to happen with sets as object literal keys:

```rego
package bin

import future.keywords.in

a := {1, 2, 3, 4}
b := {3, 4, 5}
c := {4, 5, 6}
d := { a: b | c }

output {	
    d == {
    	{1, 2, 3, 4}: {3, 4, 5}
    }
}
```
7. Big number support is unclear.
