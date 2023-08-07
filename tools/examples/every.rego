package every

import future.keywords.every
import future.keywords.if
import data.sites as sites

names_with_dev if {
    some site in sites
    site.name == "dev"

    every server in site.servers {
        endswith(server.name, "-dev")
    }
}

array_domain if {
    every i, x in [1, 2, 3] { x-i == 1 } # array domain
}

object_domain if {
    every k, v in {"foo": "bar", "fox": "baz" } { # object domain
        startswith(k, "f")
        startswith(v, "b")
    }
}

set_domain if {
    every x in {1, 2, 3} { x != 4 } # set domain
}

xs := [2, 2, 4, 8]
larger_than_one(x) := x > 1

rule_every if {
    every x in xs { larger_than_one(x) }
}

not_less_or_equal_one if not lte_one

lte_one if {
    some x in xs
    not larger_than_one(x)
}

output {
	names_with_dev
    array_domain
    object_domain
    set_domain
    rule_every
    not_less_or_equal_one
}
