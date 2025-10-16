package bodies

val := -1
a := 5
b := 3

c := a + b if {
	a
	b
	a > b
}

d := val if {
	input.a < a
	val := a + data.bodies.b
}

d := val if {
	input.a >= data.bodies.a
	val := a - b
}

e := {"one": x, "two": y + z} if {
	x := a * b
	c
	y := d
	z := x - y
}

default f := false

f if {
	b < a
	not d
}

f if {
	input.c > d
}

g if {
	d == a + b
	a < b
}
