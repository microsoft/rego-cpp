package functions

f(a, b, c) := sum if {
    b != -1
    g(input.key)
    sum := a + b + c
}

f(a, -1, c) := sum if {
    g(input.key)
    sum := a + c
}

f(a, b, c) := prod if {
    not g(input.key)
    prod := a * b * c
}

g("sum") if {
    true
}

g("prod") if {
    false
}

h(a, b) := result if {
    result := {
        "a": a,
        "b": b
    }
}

x = [data.functions.f(1, 2, 3), data.functions.f(1, -1, 3)]
