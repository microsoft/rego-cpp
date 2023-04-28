package bodies

a := 5
b := 3
c := true { a; b; a > b }

d := sum {
    sum := a + b
}

e := {"one": x, "two": y + z} {
    x := a * b
    c
    y := d
    z := x - y
}

f := {
    b > a
}

g := {
    h
    i
    a < b
}
