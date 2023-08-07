package setops

import future.keywords.in

a := {1, 2, 3, 4}
b := {3, 4, 5}
c := {4, 5, 6}
d := {2, 3, 4}
e := a | b & c | d
f := {x | some x in b & c | d}
g := [a | b]
h := [a | b, 4]
i := { a: b | c }
j := { a: b, c: d | e}
k := a - d

output {
	e == {1, 2, 3, 4, 5}
    f == {2, 3, 4, 5}
    g == [a]
    h == [{1, 2, 3, 4, 5}, 4]
    i == {
    	a: {3, 4, 5}
    }
    j == {
    	a: {3, 4, 5},
        c: {1, 2, 3, 4, 5}
    }
    k == {1}
    1 in a | b
    3 in a & b | c & d
}