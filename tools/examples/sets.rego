package sets

a := 1
b := 1
c := 2
d := 3

e := "one"
f := "one"
g := "two"
h := "three" {
	d > c
	c > b
	b >= a
}

i := {"foo": 1}
j := {"foo": 1}
k := {"foo": 2, "bar": 1, "baz": 1}
l := {"foo": 3, "bar": 2, "baz": 1}

int_set := {4, a, c, d, b}
string_set := {"four", g, f, e}
obj_set := {i, j, k, l}
array_set := {[a, b], [b, a], [a, c], [a, d], [b, c], [b, d], [c, d]}

empty_set := set()

m {
	data.sets.int_set[1]
	string_set["one"]
	data.sets.obj_set[{"foo": 1}]
	array_set[[1, 1]]
}
