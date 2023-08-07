package compr

import future.keywords.if
import data.apps as apps
import data.sites as sites

region := "west"
names := [name | sites[i].region == region; name := sites[i].name]

app_to_hostnames := {app.name: hostnames |
    app := apps[_]
    hostnames := [hostname |
                    name := app.servers[_]
                    s := sites[_].servers[_]
                    s.name == name
                    hostname := s.hostname]
}

a := [1, 2, 3, 4, 3, 4, 3, 4, 5]
b := {x | x = a[_]}

output {
    names == ["smoke", "dev"]
    app_to_hostnames == {
    	"mongodb": ["oxygen"],
        "mysql": ["lithium","carbon"],
        "web": ["hydrogen", "helium", "beryllium", "boron", "nitrogen"]
    }
    b == {1, 2, 3, 4, 5}
}
