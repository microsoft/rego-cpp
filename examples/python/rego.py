"""Example of using regopy to interpret Rego policies read from the command line."""

from argparse import ArgumentParser
import os

import regopy as rp


def rego_eval(args):
    rego = rp.Interpreter()
    rego.log_level = rp.log_level_from_string(args.loglevel)
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
            rego.set_input_term(file.read())

    print(rego.query(args.query))


def rego_build(args):
    rego = rp.Interpreter()
    rego.log_level = rp.log_level_from_string(args.loglevel)
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

    bundle = rego.build(args.query, args.entrypoint)
    if not bundle.ok():
        return

    rego.save_bundle(args.bundle, bundle, args.format == "binary")

    print("Bundle built successfully")


def rego_run(args):
    rego = rp.Interpreter()
    rego.log_level = rp.log_level_from_string(args.loglevel)
    if args.input:
        with open(args.input) as file:
            rego.set_input_term(file.read())

    bundle = rego.load_bundle(args.bundle, args.format == "binary")
    if not bundle.ok():
        return

    if args.entrypoint:
        print(rego.query_bundle_entrypoint(bundle, args.entrypoint))
    else:
        print(rego.query_bundle(bundle))


def parse_args():
    """Parse command line arguments."""
    parser = ArgumentParser("rego.py",
                            description="Command line tool for querying Rego policies")
    parser.add_argument("--version", action="version", version="%(prog)s {}".format(rp.build_info()))
    parser.add_argument("-l", "--loglevel", choices=["None", "Error", "Output",
                        "Warn", "Info", "Debug", "Trace"], help="Sets the logging level", default="Output")

    subparsers = parser.add_subparsers(help="rego commands", required=True)
    parser_eval = subparsers.add_parser("eval", help="Evaluate rego policies")
    parser_eval.add_argument("-d", "--data", action="append",
                             help="set policy or data file(s). This flag can be repeated.")
    parser_eval.add_argument("-i", "--input", help="set input file path.")
    parser_eval.add_argument("query", help="the query string")
    parser_eval.add_argument("-l", "--loglevel", choices=["None", "Error", "Output",
                                                          "Warn", "Info", "Debug", "Trace"],
                             help="Sets the logging level", default="Output")
    parser_eval.set_defaults(func=rego_eval)

    parser_build = subparsers.add_parser("build", help="Build rego bundles")
    parser_build.add_argument("-d", "--data", action="append",
                              help="set policy or data file(s). This flag can be repeated.")
    parser_build.add_argument("-q", "--query", help="the query string")
    parser_build.add_argument("-e", "--entrypoint", action="append",
                              help="entrypoint for the bundle. This flag can be repeated.")
    parser_build.add_argument("-b", "--bundle", default="bundle", help="Path to the bundle")
    parser_build.add_argument("-f", "--format", choices=["json", "binary"], default="json", help="Bundle format")
    parser_build.add_argument("-l", "--loglevel", choices=["None", "Error", "Output",
                                                           "Warn", "Info", "Debug", "Trace"],
                              help="Sets the logging level", default="Output")
    parser_build.set_defaults(func=rego_build)

    parser_run = subparsers.add_parser("run", help="Run compiled rego bundle")
    parser_run.add_argument("-b", "--bundle", default="bundle", help="Path to the bundle")
    parser_run.add_argument("-f", "--format", choices=["json", "binary"], default="json", help="Bundle format")
    parser_run.add_argument("-e", "--entrypoint", help="entrypoint to run.")
    parser_run.add_argument("-i", "--input", help="set input file path.")
    parser_run.add_argument("-l", "--loglevel", choices=["None", "Error", "Output",
                                                         "Warn", "Info", "Debug", "Trace"],
                            help="Sets the logging level", default="Output")
    parser_run.set_defaults(func=rego_run)

    return parser.parse_args()


def main():
    """Main function."""
    args = parse_args()
    print("rego", rp.build_info())
    args.func(args)


if __name__ == "__main__":
    main()
