from enum import Enum
import math
from typing import List, Tuple


core_names = [
    "count",
    "max",
    "min",
    "sort",
    "sum",
    "product",
    "equal",
    "gt",
    "gte",
    "lt",
    "lte",
    "neq",
    "to_number",
    "walk",
    "abs",
    "ceil",
    "floor",
    "round",
    "plus",
    "minus",
    "mul",
    "div",
    "rem",
    "and",
    "or",
    "intersection",
    "union",
    "concat",
    "startswith",
    "endswith",
    "contains",
    "format_int",
    "indexof",
    "indexof_n",
    "lower",
    "upper",
    "replace",
    "split",
    "sprintf",
    "substring",
    "trim",
    "trim_left",
    "trim_right",
    "trim_space",
    "trim_prefix",
    "trim_suffix",
    "is_array",
    "is_boolean",
    "is_null",
    "is_number",
    "is_object",
    "is_set",
    "is_string",
    "type_name"
]

prefix_names = [
    "array",
    "providers",
    "base64",
    "base64url",
    "bits",
    "crypto",
    "glob",
    "graph",
    "graphql",
    "hex",
    "http",
    "internal",
    "json",
    "io",
    "net",
    "numbers",
    "object",
    "opa",
    "rand",
    "rego",
    "regex",
    "semver",
    "strings",
    "time",
    "units",
    "urlquery",
    "uuid",
    "yaml"
]


class NodeKind(Enum):
    Leaf = 0
    Length = 1
    FirstChar = 2
    LastChar = 3


class Node:
    def __init__(self, names: List[str]):
        self.kind = NodeKind.Leaf
        self.threshold = 0
        self.names = names
        self.split()

    def split(self):
        if len(self.names) == 1:
            return

        lengths = [len(n) for n in self.names]
        first_char = [ord(n[0]) for n in self.names]
        last_char = [ord(n[-1]) for n in self.names]

        t0, e0 = Node.find_split(lengths)
        t1, e1 = Node.find_split(first_char)
        t2, e2 = Node.find_split(last_char)
        if e0 >= e1 and e0 >= e2:
            self.kind = NodeKind.Length
            self.threshold = t0
            values = lengths
        elif e1 >= e0 and e1 >= e2:
            self.kind = NodeKind.FirstChar
            self.threshold = t1
            values = first_char
        else:
            self.kind = NodeKind.LastChar
            self.threshold = t2
            values = last_char

        left = []
        right = []
        for v, n in zip(values, self.names):
            if v <= self.threshold:
                left.append(n)
            else:
                right.append(n)

        if len(left) == 0 or len(right) == 0:
            self.kind = NodeKind.Leaf
            self.threshold = 0
            return

        self.left = Node(left)
        self.right = Node(right)

    def to_cpp(self, file, top_level):
        match self.kind:
            case NodeKind.Leaf:
                if top_level:
                    for name in self.names:
                        file.write(f"if(view == \"{name}\"){{ return builtins::{name}(name); }}")
                else:
                    for name in self.names:
                        file.write(f"if(view == \"{name}\"){{ return {name}_factory(); }}")
            case NodeKind.Length:
                file.write(f"if(length > {self.threshold}){{")
                self.right.to_cpp(file, top_level)
                file.write("}")
                self.left.to_cpp(file, top_level)
            case NodeKind.FirstChar:
                file.write(f"if(first_char > '{chr(self.threshold)}'){{")
                self.right.to_cpp(file, top_level)
                file.write("}")
                self.left.to_cpp(file, top_level)
            case NodeKind.LastChar:
                file.write(f"if(last_char > '{chr(self.threshold)}'){{")
                self.right.to_cpp(file, top_level)
                file.write("}")
                self.left.to_cpp(file, top_level)

    @staticmethod
    def find_split(values: List[int]) -> Tuple[int, float]:
        sorted_values = list(sorted(values))
        i = 0
        t_max = sorted_values[i]
        n = len(sorted_values)
        e_max = 0
        while i < n:
            t = sorted_values[i]
            while i < n and sorted_values[i] == t:
                i += 1

            if i == n:
                return t_max, e_max

            left = i / n
            right = (n - i) / n
            e = -left * math.log(left) - right * math.log(right)
            if e > e_max:
                e_max = e
                t_max = t

            i += 1

        return t_max, e_max


if __name__ == "__main__":
    tree = Node(core_names)
    with open("core.txt", "w") as file:
        tree.to_cpp(file, False)
        file.write("return nullptr;")

    tree = Node(prefix_names)
    with open("prefix.txt", "w") as file:
        tree.to_cpp(file, True)
        file.write("return nullptr;")
