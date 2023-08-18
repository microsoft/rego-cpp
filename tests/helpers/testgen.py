"""Script to generate test rego files."""

from random import getrandbits, random
from typing import List


def operand(num_bits: int) -> int:
    """Generates a random operand."""
    result = getrandbits(num_bits)
    if random() < 0.5:
        result = -result

    return result


def bigint_testcase(num_bits: int) -> List[str]:
    """Generates a test case for bigint."""
    operators = {
        '+': ("a", "tadd", lambda x, y: x + y),
        '-': ("b", "tsub", lambda x, y: x - y),
        '*': ("c", "tmul", lambda x, y: x * y),
        '/': ("d", "tdiv", lambda x, y: x // y),
        '%': ("e", "tmod", lambda x, y: x % y),
    }

    lines = []
    lines.append("- note: bigint/{}-bit".format(num_bits))
    lines.append("  modules:")
    lines.append("  - |")
    lines.append("    package bigint")
    lines.append("    ")
    query = []
    want_result = []
    for operator in operators:
        lhs = operand(num_bits)
        if operator == '/' or operator == '%':
            rhs = operand(num_bits // 2)
            if rhs == 0:
                rhs = 1
        else:
            rhs = operand(num_bits)
        var, name, op = operators[operator]
        result = op(lhs, rhs)
        lines.append("    {} := {} {} {}".format(name, lhs, operator, rhs))
        query.append("{} = data.bigint.{}".format(var, name))
        want_result.append("    - {}: {}".format(var, result))

    lines.append("  query: {}".format("; ".join(query)))
    lines.append("  want_result:")
    for line in want_result:
        lines.append(line)
    return lines


def bigint(path: str, bits: range):
    """Generates a test rego file for bigint."""

    with open(path, "w") as file:
        file.write("cases:\n")
        for num_bits in bits:
            lines = bigint_testcase(num_bits)
            file.write("\n".join(lines))
            file.write("\n")


def main():
    bigint("bigint.yaml", range(4, 256, 8))


if __name__ == "__main__":
    main()
