package bodies

val := -1
a := 5
b := 3
c := a + b { a; b; a > b }
d := val {
	input.a < a
	val := a + data.bodies.b
} {
	input.a >= data.bodies.a
	val := a - b
}

e := {"one": x, "two": y + z} {
	x := a * b
	c
	y := d
	z := x - y
}

default f := false

f {
	b < a
	not d
}

f {
	input.c > d
}

g {
	d == a + b
	a < b
}
