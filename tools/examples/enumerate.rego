package enumerate

import future.keywords.in

numbers := [1, 2, 3, 4, 5]
letters := ["1", "2", "3", "4", "5"]

a := item[1] {
    some n in numbers
    some l in letters
    to_number(l) == n
    item = [n, l]
    item[0] == 3
    print(item[0], item[1])
}