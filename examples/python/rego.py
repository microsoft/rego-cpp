"""Example of using regopy to interpret Rego policies read from the command line."""

from argparse import ArgumentParser
import os

import regopy as rp


def parse_args():
    """Parse command line arguments."""
    parser = ArgumentParser("rego.py",
                            description="Command line tool for querying Rego policies")
    parser.add_argument("--version", action="version", version="%(prog)s {}".format(rp.build_info()))
    parser.add_argument("-d", "--data", action="append",
                        help="set policy or data file(s). This flag can be repeated.")
    parser.add_argument("-i", "--input", help="set input file path.")
    parser.add_argument("query", help="the query string")
    return parser.parse_args()


def main():
    """Main function."""
    args = parse_args()
    rego = rp.Interpreter()
    if args.data:
        for path in args.data:
            if not os.path.exists(path):
                print("File {} does not exist.".format(path))
                return 1

            with open(path) as file:
                contents = file.read()
                if path.endswith(".rego"):
                    name = os.path.basename(path)
                    rego.add_module(name, contents)
                else:
                    rego.add_data_json(contents)

    if args.input:
        with open(args.input) as file:
            rego.set_input_json(file.read())

    print(rego.query(args.query))


if __name__ == "__main__":
    main()
