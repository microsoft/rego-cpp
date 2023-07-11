package functions

f(a, b, c) := sum {
	b != -1
    g(input.key)
    sum := a + b + c
}

f(a, -1, c) := sum {
    g(input.key)
    sum := a + c
}

f(a, b, c) := prod {
    not g(input.key)
    prod := a * b * c
}

g("sum") {
    true
}

g("prod") {
    false
}

h(a, b) := result {
    result := {
        "a": a,
        "b": b
    }
}