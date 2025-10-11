"""Module providing a Pythonic interface to the rego-cpp interpreter."""

from enum import Enum
import json
from typing import Any, Optional, Sequence

from .node import Node
from .output import Output
from .rego_shared import (
    rego_new,
    rego_add_module,
    rego_add_data_json,
    rego_set_input_term,
    rego_set_input,
    rego_get_debug_enabled,
    rego_set_debug_enabled,
    rego_set_debug_path,
    rego_get_wf_checks_enabled,
    rego_set_wf_checks_enabled,
    rego_get_strict_builtin_errors,
    rego_set_strict_builtin_errors,
    rego_get_log_level,
    rego_set_log_level,
    rego_set_query,
    rego_add_entrypoint,
    rego_is_available_builtin,
    rego_query,
    rego_build,
    rego_bundle_load,
    rego_bundle_load_binary,
    rego_bundle_save,
    rego_bundle_save_binary,
    rego_bundle_query,
    rego_bundle_query_entrypoint,
    rego_bundle_node,
    rego_bundle_ok,
    rego_free_bundle,
    rego_free,
    rego_input_from_value,
    rego_free_input,
    LogLevel
)


class BundleFormat(Enum):
    """Formats for bundle serialization."""
    JSON = 0
    Binary = 1


class Bundle:
    """Compiled base and virtual documents."""
    def __init__(self, impl):
        self._impl = impl

    def __del__(self):
        rego_free_bundle(self._impl)

    def node(self) -> Node:
        """The AST which represents the saved state of the bundle."""
        return Node(rego_bundle_node(self._impl))

    def ok(self) -> bool:
        """Whether the bundle was built successfully."""
        return rego_bundle_ok(self._impl)


class Input:
    """Used to create an in-memory Input object for passing to a policy.

    Example:
        >>> from regopy import Input, Interpreter
        >>> rego = Interpreter()
        >>> rego.set_input(Input({"a": 10, "b": "20", "c": 30.0, "d": True}))
        >>> print(rego.query("input.a"))
        {"expressions":[10]}
    """

    def __init__(self, value: Any):
        self._impl = rego_input_from_value(value)

    def __del__(self):
        if hasattr(self, "_impl"):
            rego_free_input(self._impl)


class Interpreter:
    """Pythonic interface to the rego-cpp interpreter.

    This wraps the Rego C API, and handles passing calls to
    the C API and converting the results to Python types.

    Examples:
        >>> from regopy import Interpreter
        >>> rego = Interpreter()
        >>> print(rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4;2 * 5"))
        {"expressions":[true, true, 10], "bindings":{"x":5, "y":9.4}}

        >>> input = {
        ...     "a": 10,
        ...     "b": "20",
        ...     "c": 30.0,
        ...     "d": True
        ... }
        >>> data0 = {
        ...     "one": {
        ...         "bar": "Foo",
        ...         "baz": 5,
        ...         "be": True,
        ...         "bop": 23.4
        ...     },
        ...     "two": {
        ...         "bar": "Bar",
        ...         "baz": 12.3,
        ...         "be": False,
        ...         "bop": 42
        ...     }
        ... }
        >>> data1 = {
        ...     "three": {
        ...         "bar": "Baz",
        ...         "baz": 15,
        ...         "be": True,
        ...         "bop": 4.23
        ...     }
        ... }
        >>> module = '''
        ...     package objects
        ...
        ...     rect := {`width`: 2, "height": 4}
        ...     cube := {"width": 3, `height`: 4, "depth": 5}
        ...     a := 42
        ...     b := false
        ...     c := null
        ...     d := {"a": a, "x": [b, c]}
        ...     index := 1
        ...     shapes := [rect, cube]
        ...     names := ["prod", `smoke1`, "dev"]
        ...     sites := [{"name": "prod"}, {"name": names[index]}, {"name": "dev"}]
        ...     e := {
        ...         a: "foo",
        ...         "three": c,
        ...         names[2]: b,
        ...         "four": d,
        ...     }
        ...     f := e["dev"]
        ... '''
        >>> rego.set_input(input)
        >>> rego.add_data(data0)
        >>> rego.add_data(data1)
        >>> rego.add_module("objects", module)
        >>> print(rego.query("x=[data.one, input.b, data.objects.sites[1]]"))
        {"expressions":[true], "bindings":{"x":[{"bar":"Foo", "baz":5, "be":true, "bop":23.4},"20",{"name":"smoke1"}]}}
    """

    def __init__(self):
        """Initializer."""
        self._impl = rego_new()

    def __del__(self):
        """Destructor."""
        rego_free(self._impl)

    def add_module(self, name: str, source: str):
        """Adds a module (i.e. a virtual document) to the interpreter.

        Args:
            name (str): The module name.
            source (str): The module source.

        Raises:
            RegoError: If an error occurs in the Rego interpreter.

        Example:
            >>> from regopy import Interpreter
            >>> module = '''
            ...   package scalars
            ...
            ...   greeting := "Hello"
            ...   max_height := 42
            ...   pi := 3.14159
            ...   allowed := true
            ...   location := null
            ... '''
            >>> rego = Interpreter()
            >>> rego.add_module("scalars", module)
            >>> output = rego.query("data.scalars.greeting")
            >>> print(output)
            {"expressions":["Hello"]}
        """
        rego_add_module(self._impl, name, source)

    def add_data_json(self, json: str):
        """Adds data (i.e. a base document) to the interpreter.

        The data should be provided as single JSON-encoded object.

        Args:
            json (str): The data JSON.

        Raises:
            RegoError: If an error occurs in the Rego interpreter.

        Example:
            >>> from regopy import Interpreter
            >>> rego = Interpreter()
            >>> data = '''
            ...   {
            ...     "one": {
            ...       "bar": "Foo",
            ...       "baz": 5,
            ...       "be": true,
            ...       "bop": 23.4
            ...     },
            ...     "two": {
            ...       "bar": "Bar",
            ...       "baz": 12.3,
            ...       "be": false,
            ...       "bop": 42
            ...     }
            ...   }
            ... '''
            >>> rego.add_data_json(data)
            >>> output = rego.query("data.one.bar")
            >>> print(output)
            {"expressions":["Foo"]}
        """
        rego_add_data_json(self._impl, json)

    def add_data(self, data: Any):
        """Adds data (i.e. a base document) to the interpreter.

        The data should be provided as single, JSON-encodable Python object.

        Args:
            data (Any): The data.

        Raises:
            RegoError: If an error occurs in the Rego interpreter.

        See Also:
            :func:`~regopy.Interpreter.add_data_json`
        """
        self.add_data_json(json.dumps(data))

    def set_input_term(self, term: str):
        """Sets the input term of the interpreter.

        This can be called several times during the lifetime of the interpreter.

        Args:
            term (str): The input term.

        Raises:
            RegoError: If an error occurs in the Rego interpreter.

        Example:
            >>> from regopy import Interpreter
            >>> input = '''
            ...   {
            ...     "a": 10,
            ...     "b": "20",
            ...     "c": 30.0,
            ...     "d": true
            ...   }
            ... '''
            >>> rego = Interpreter()
            >>> rego.set_input_term(input)
            >>> output = rego.query("input.a")
            >>> print(output)
            {"expressions":[10]}
        """
        rego_set_input_term(self._impl, term)

    def set_input(self, value: Input):
        """Sets the input document of the interpreter.

        This can be called several times during the lifetime of the interpreter.
        The value must be JSON encodable.

        Args:
            value (Input): The input value.

        Raises:
            RegoError: If an error occurs in the Rego interpreter.

        See Also:
            :func:`~regopy.Interpreter.set_input_json`
        """
        if not isinstance(value, Input):
            value = Input(value)

        rego_set_input(self._impl, value._impl)

    @property
    def debug_enabled(self) -> bool:
        """Whether debug mode is enabled.

        When debug mode is enabled, the Rego interpreter will output extensive
        debugging information about the compliation process, including intermediary
        ASTs and the generated bytecode. This is mostly useful for debugging the
        Rego compiler itself, and may not be as useful for debugging Rego policies.
        """
        return rego_get_debug_enabled(self._impl)

    @debug_enabled.setter
    def debug_enabled(self, value: bool):
        rego_set_debug_enabled(self._impl, value)

    def set_debug_path(self, path: str):
        """Sets the path to the directory where this will write debug AST files.

        This is only useful when debug mode is enabled.

        Args:
            path (str): The path to the directory where this will write debug AST files.
        """
        rego_set_debug_path(self._impl, path)

    @property
    def well_formed_checks_enabled(self) -> bool:
        """Whether the Rego interpreter will perform well-formedness checks.

        When enabled, the Rego interpreter will perform well-formedness checks on
        the AST during each pass of the compiler.
        """
        return rego_get_wf_checks_enabled(self._impl)

    @well_formed_checks_enabled.setter
    def well_formed_checks_enabled(self, value: bool):
        rego_set_wf_checks_enabled(self._impl, value)

    @property
    def log_level(self) -> LogLevel:
        """The log level of the interpreter"""
        return rego_get_log_level(self._impl)

    @log_level.setter
    def log_level(self, value: LogLevel):
        rego_set_log_level(self._impl, value)

    @property
    def strict_built_in_errors(self) -> bool:
        """Whether the interpreter will forward errors thrown by the built-ins.

        By default, the Rego interpreter will catch errors thrown by the built-ins
        and return them as an Undefined result. When this is enabled, the interpreter
        will instead forward the error to the caller.
        """
        return rego_get_strict_builtin_errors(self._impl)

    @strict_built_in_errors.setter
    def strict_built_in_errors(self, value: bool):
        rego_set_strict_builtin_errors(self._impl, value)

    def is_builtin(self, name: str) -> bool:
        """Returns whether the given name is a built-in function.

        Args:
            name (str): The name of the function.

        Returns:
            bool: Whether the given name is a built-in function.
        """
        return rego_is_available_builtin(self._impl, name)

    def query(self, query: str) -> Output:
        """Performs a query against the current set of base and virtual documents.

        While the Rego interpreter can be used to perform simple queries, in most cases
        users will want to load one or more base documents (using :func:`~regopy.Interpreter.add_data` or
        :func:`~regopy.Interpreter.add_data_json`) and one or more Rego modules
        (using :func:`~regopy.Interpreter.add_module` or :func:`~regopy.Interpreter.add_module_file`).
        Then, multiple queries can be performed by providing an input
        (using :func:`~regopy.Interpreter.set_input` or :func:`~regopy.Interpreter.set_input_json`)
        and then calling this method.

        Args:
            query (str): The query.

        Returns:
            Output: The query result.

        Raises:
            RegoError: If an error occurs in the Rego interpreter.

        Examples:
            >>> from regopy import Interpreter
            >>> input0 = {"a": 10}
            >>> input1 = {"a": 4}
            >>> input2 = {"a": 7}
            >>> multi = '''
            ...   package multi
            ...
            ...   default a := 0
            ...
            ...   a := val {
            ...       input.a > 0
            ...       input.a < 10
            ...       input.a % 2 == 1
            ...       val := input.a * 10
            ...   } {
            ...       input.a > 0
            ...       input.a < 10
            ...       input.a % 2 == 0
            ...       val := input.a * 10 + 1
            ...   }
            ...
            ...   a := input.a / 10 {
            ...       input.a >= 10
            ...   }
            ... '''
            >>> rego = Interpreter()
            >>> rego.add_module("multi", multi)
            >>> rego.set_input(input0)
            >>> output = rego.query("data.multi.a")
            >>> print(output)
            {"expressions":[1]}
            >>> rego.set_input(input1)
            >>> output = rego.query("data.multi.a")
            >>> print(output)
            {"expressions":[41]}
            >>> rego.set_input(input2)
            >>> output = rego.query("data.multi.a")
            >>> print(output)
            {"expressions":[70]}
        """
        impl = rego_query(self._impl, query)
        return Output(impl)

    def build(self, query: Optional[str], entrypoints: Sequence[str] = tuple()) -> Bundle:
        """Builds a bundle, which contains a compiled version of the base and virtual documents.

        Args:
            query (str): The query string. Either a query or at least one entrypoint
                         must be provided.

            entrypoints (Sequence[str]): Zero or more entrypoints, in the form `<document>/<rule>`,
                                         for example `objects/sites` or `parent/child/foo`. There must
                                         either be at least one entrypoint or a query for the bundle
                                         to be produced.

        Returns:
            Bundle: contains the compiled base and virtual documents, and plans to execute the
                    specified entrypoints (and the query, if provided). Can be written to disk and
                    loaded back again by a different interpreter.

        Raises:
            RegoError: If an error occurs during compilation.

        Example:
            >>> from regopy import Interpreter
            >>> input0 = {
            ...     "x": 104,
            ...     "y": 119
            ... }
            >>> data = {
            ...     "a": 7,
            ...     "b": 13
            ... }
            >>> module = '''
            ...     package example
            ...
            ...     foo := data.a * input.x + data.b * input.y
            ...     bar := data.b * input.x + data.a * input.y
            ... '''
            >>> rego_build = Interpreter()
            >>> rego_build.add_data(data)
            >>> rego_build.add_module("example.rego", module)
            >>> bundle = rego_build.build("x=data.example.foo + data.example.bar",
            ...                     ["example/foo", "example/bar"])
            >>> rego_run = Interpreter()
            >>> rego_run.set_input(input0)
            >>> output = rego_run.query_bundle(bundle)
            >>> print(output)
            {"expressions":[true], "bindings":{"x":4460}}
        """
        if query is not None:
            rego_set_query(self._impl, query)
        else:
            assert len(entrypoints) > 0, "no query or entrypoints specified"

        for e in entrypoints:
            rego_add_entrypoint(self._impl, e)

        return Bundle(rego_build(self._impl))

    def save_bundle(self, path: str, bundle: Bundle, format=BundleFormat.JSON):
        """Save a bundle to disk.

        Args:
            path (str): A path to a location on disk. If being saved in default
                        mode, this will be a directory. Otherwise, it will become
                        a single binary file.
            bundle (Bundle): The bundle to save
            binary (bool): Whether to save the bundle as a directory (default) or
                           as a single binary file.

        Raises:
            RegoError: If an error occurs during compilation.

        There are two modes for bundle serialization. The default, which uses JSON,
        creates a directory at the specified path (if it does not already exist)
        and then writes at least two files: `plan.json` (which contains the compiled
        virtual documents and plans for execution) and `data.json` (which contains the
        base documents merged into a single JSON hierarchy). Module source files
        will also be copied into the directory.

        The second mode is binary serialization. This uses the Rego Bundle Binary
        format (TODO URL) to create a single file which contains all the bundle
        information.

        Example:
            >>> from regopy import Interpreter
            >>> rego_build = Interpreter()
            >>> bundle = rego_build.build("a=1")
            >>> rego_build.save_bundle("bundle", bundle)
            >>> rego_build.save_bundle("bundle_bin.rbb", bundle, BundleFormat.Binary)
            >>>
            >>> rego_run = Interpreter()
            >>> bundle = rego_run.load_bundle("bundle")
            >>> output = rego_run.query_bundle(bundle)
            >>> print(output.binding("a"))
            1
            >>> bundle = rego_run.load_bundle("bundle_bin.rbb", BundleFormat.Binary)
            >>> output = rego_run.query_bundle(bundle)
            >>> print(output.binding("a"))
            1
        """
        match format:
            case BundleFormat.JSON:
                rego_bundle_save(self._impl, path, bundle._impl)
            case BundleFormat.Binary:
                rego_bundle_save_binary(self._impl, path, bundle._impl)
            case _:
                raise NotImplementedError()

    def load_bundle(self, path: str, format=BundleFormat.JSON) -> Bundle:
        """Loads a bundle from the disk.

        Args:
            path (str): A path to a location on disk. If being saved in default
                        mode, this will be a directory. Otherwise, it will become
                        a single binary file.
            bundle (Bundle): The bundle to save
            binary (bool): Whether to save the bundle as a directory (default) or
                           as a single binary file.

        Returns:
            bundle (Bundle): The serialized bundle

        Raises:
            RegoError: If an error occurs during compilation.
        """
        if format == BundleFormat.JSON:
            return Bundle(rego_bundle_load(self._impl, path))

        if format == BundleFormat.Binary:
            return Bundle(rego_bundle_load_binary(self._impl, path))

        raise NotImplementedError()

    def query_bundle(self, bundle: Bundle) -> Output:
        """Performs a query using the compiled policy in the bundle.

        Args:
            bundle (Bundle): The bundle to execute

        Returns:
            output (Output): The result of the query

        Raises:
            RegoError: If an error occurs during execution

        This method requires that a query was provided to `:func:`~regopy.Interpreter.build`.
        Otherwise, it will throw an exception.
        """
        return Output(rego_bundle_query(self._impl, bundle._impl))

    def query_bundle_entrypoint(self, bundle: Bundle, entrypoint: str) -> Output:
        """Performs a query using the compiled policy in the bundle.

        Args:
            bundle (Bundle): The bundle to execute
            entrypoint (str): The entrypoint to execute

        Returns:
            output (Output): The result of the query

        Raises:
            RegoError: If an error occurs during execution

        This method requires that the specified entrypoint was provided to
        `:func:`~regopy.Interpreter.build`. Otherwise, it will throw an exception.
        """
        return Output(rego_bundle_query_entrypoint(self._impl, bundle._impl, entrypoint))

    def __repr__(self) -> str:
        """Returns a string representation of the interpreter."""
        return "Interpreter({})".format(self._impl)
