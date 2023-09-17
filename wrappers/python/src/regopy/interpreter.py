"""Module providing a Pythonic interface to the rego-cpp interpreter."""

import json
from typing import Any

from ._regopy import (
    REGO_ERROR,
    regoAddDataJSON,
    regoAddModule,
    regoFree,
    regoGetDebugEnabled,
    regoGetError,
    regoGetStrictBuiltInErrors,
    regoGetWellFormedChecksEnabled,
    regoNew,
    regoQuery,
    regoSetDebugEnabled,
    regoSetDebugPath,
    regoSetInputJSON,
    regoSetStrictBuiltInErrors,
    regoSetWellFormedChecksEnabled
)
from .output import Output


class RegoError(Exception):
    """Exception raised when an error occurs in the rego-cpp interpreter."""

    def __init__(self, message: str):
        """Initializer.

        Args:
            message (str): The error message.
        """
        Exception.__init__(self, message)


class Interpreter:
    """Pythonic interface to the rego-cpp interpreter.

    This wraps the Rego C API, and handles passing calls to
    the C API and converting the results to Python types.

    Examples:
        >>> from regopy import Interpreter
        >>> rego = Interpreter()
        >>> print(rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4"))
        x = 5
        y = 9.4

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
        x = [{"bar":"Foo", "baz":5, "be":true, "bop":23.4}, "20", {"name":"smoke1"}]
    """

    def __init__(self):
        """Initializer."""
        self._impl = regoNew()

    def __del__(self):
        """Destructor."""
        regoFree(self._impl)

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
            "Hello"
        """
        err = regoAddModule(self._impl, name, source)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

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
            "Foo"
        """
        err = regoAddDataJSON(self._impl, json)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

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

    def set_input_json(self, json: str):
        """Sets the input document of the interpreter.

        This can be called several times during the lifetime of the interpreter.
        The input should be provided as single JSON-encoded value.

        Args:
            json (str): The input JSON.

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
            >>> rego.set_input_json(input)
            >>> output = rego.query("input.a")
            >>> print(output)
            10
        """
        err = regoSetInputJSON(self._impl, json)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

    def set_input(self, value: Any):
        """Sets the input document of the interpreter.

        This can be called several times during the lifetime of the interpreter.
        The value must be JSON encodable.

        Args:
            value (Any): The input value.

        Raises:
            RegoError: If an error occurs in the Rego interpreter.

        See Also:
            :func:`~regopy.Interpreter.set_input_json`
        """
        self.set_input_json(json.dumps(value))

    @property
    def debug_enabled(self) -> bool:
        """Whether debug mode is enabled.

        When debug mode is enabled, the Rego interpreter will output extensive
        debugging information about the compliation process, including intermediary
        ASTs and the generated bytecode. This is mostly useful for debugging the
        Rego compiler itself, and may not be as useful for debugging Rego policies.
        """
        return regoGetDebugEnabled(self._impl)

    @debug_enabled.setter
    def debug_enabled(self, value: bool):
        regoSetDebugEnabled(self._impl, value)

    def set_debug_path(self, path: str):
        """Sets the path to the directory where this will write debug AST files.

        This is only useful when debug mode is enabled.

        Args:
            path (str): The path to the directory where this will write debug AST files.
        """
        err = regoSetDebugPath(self._impl, path)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

    @property
    def well_formed_checks_enabled(self) -> bool:
        """Whether the Rego interpreter will perform well-formedness checks.

        When enabled, the Rego interpreter will perform well-formedness checks on
        the AST during each pass of the compiler.
        """
        return regoGetWellFormedChecksEnabled(self._impl)

    @well_formed_checks_enabled.setter
    def well_formed_checks_enabled(self, value: bool):
        regoSetWellFormedChecksEnabled(self._impl, value)

    @property
    def strict_built_in_errors(self) -> bool:
        """Whether the interpreter will forward errors thrown by the built-ins.

        By default, the Rego interpreter will catch errors thrown by the built-ins
        and return them as an Undefined result. When this is enabled, the interpreter
        will instead forward the error to the caller.
        """
        return regoGetStrictBuiltInErrors(self._impl)

    @strict_built_in_errors.setter
    def strict_built_in_errors(self, value: bool):
        regoSetStrictBuiltInErrors(self._impl, value)

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
            1
            >>> rego.set_input(input1)
            >>> output = rego.query("data.multi.a")
            >>> print(output)
            41
            >>> rego.set_input(input2)
            >>> output = rego.query("data.multi.a")
            >>> print(output)
            70
        """
        impl = regoQuery(self._impl, query)
        if impl is None:
            raise RegoError(regoGetError(self._impl))

        return Output(impl)

    def __repr__(self) -> str:
        """Returns a string representation of the interpreter."""
        return "Interpreter({})".format(self._impl)
