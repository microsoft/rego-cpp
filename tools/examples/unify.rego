package unify

tuples = [[1, 1], [1, 2], [2, 3], [3, 5], [5, 8]]
letters = {["a", 1], ["b", 2], ["c", 3], ["d", 4]}
mixed = [[0, 1, 2], [3, 4, 5, 6], {"foo": 7}]

a := {
	"foo": 1,
    "bar": 2,
}

b := x + y {
    some x, y
    [1, x] = tuples[i]
    {"foo": y, "bar": 2} = a
    i != 0
}

c {
    i != 0
    x = tuples[i]
    i = x[1] - 1
}

d {
	tuple := tuples[i]
    tuple[0] >= 3
    tuple[1] >= 5
}

e := key {
    target = 2
    some key
	a[key] = target
}

f := x + y {
    letters[["a", x]]
    letters[["c", y]]
}

g := j {
	x = mixed[i]
    x[j] = 6
}

h := j {
	x = mixed[i]
    x[j] = 7
}

output := [a, b, c, d, e, f, g, h]
