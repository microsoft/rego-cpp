package keywords

import future.keywords

letters := ["a", "b", "c", "d"]

a := 3
b := 5
c := 4 if input.a > a else := 5 if {input.a > a} else := -1 if input.a <= a


d := val if {
    some i, x in letters
    x == "b"
    some j in [1, 2, "a"]
    x == letters[j]
    i == j
    "d" in letters
    val := "found"
}

e contains "0" if {
    input.d
    input.b == "20"
}

e contains "a" if {
    input.a > a
}

e contains "1" if input.key == "sum"

e contains a + b if input.key == "sum"

e contains "2" if input.key == "prod"

e contains a * b if input.key == "prod"

f["a"] := a
f["b"] := b
f["c"] if c
f["sum"] := sum {
    sum := a + b
}

output := [c, d, e, f]