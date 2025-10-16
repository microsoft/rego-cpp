package enumerate

numbers := [1, 2, 3, 4, 5]
letters := ["1", "2", "3", "4", "5"]

a := item if {
    some n in numbers
    some l in letters
    to_number(l) == n
    item = [n, l]
    item[0] == 3
}

b contains [i, x] if {
    some i, x in numbers
    x > 2
}

output := [a, b]