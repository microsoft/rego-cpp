package bodies

import future.keywords.if
import future.keywords.in

d_sum := result {
    result := d with input.a as a - 1
}

d_diff := result {
    result := d with input.a as a + 1
}

inner := [x, y] if {
    x := input.foo
    y := input.bar
}

middle := [a, b] if {
    a := inner with input.foo as 100
    b := input
}

outer := result if {
    result := middle with input as {"foo": 200, "bar": 300}
}

mycount(x) := count(x)

mock_count(x) := 0 if "x" in x
mock_count(x) := count(x) if not "x" in x

use_count := result {
    result := mycount([1, 2, 3]) with count as mock_count
}

use_mock := result {
    result := mycount(["x", "y", "z"]) with count as mock_count
}

output := [d_sum, d_diff, outer, use_count, use_mock]
