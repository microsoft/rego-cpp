"""Module providing a Pythonic interface to the rego-cpp interpreter."""

from enum import IntEnum
import json

from .output import Output
from ._regopy import (
    REGO_ERROR,
    REGO_LOG_LEVEL_NONE,
    REGO_LOG_LEVEL_ERROR,
    REGO_LOG_LEVEL_WARN,
    REGO_LOG_LEVEL_INFO,
    REGO_LOG_LEVEL_DEBUG,
    REGO_LOG_LEVEL_TRACE,
    regoNew,
    regoFree,
    regoAddModule,
    regoAddDataJSON,
    regoSetInputJSON,
    regoSetDebugEnabled,
    regoGetDebugEnabled,
    regoSetDebugPath,
    regoSetWellFormedChecksEnabled,
    regoGetWellFormedChecksEnabled,
    regoQuery,
    regoSetStrictBuiltInErrors,
    regoGetStrictBuiltInErrors,
    regoGetError
)


class LogLevel(IntEnum):
    NONE = REGO_LOG_LEVEL_NONE
    ERROR = REGO_LOG_LEVEL_ERROR
    WARN = REGO_LOG_LEVEL_WARN
    INFO = REGO_LOG_LEVEL_INFO
    DEBUG = REGO_LOG_LEVEL_DEBUG
    TRACE = REGO_LOG_LEVEL_TRACE


class RegoError(Exception):
    def __init__(self, message: str):
        Exception.__init__(self, message)


class Interpreter:
    def __init__(self):
        self._impl = regoNew()

    def __del__(self):
        regoFree(self._impl)

    def add_module(self, name: str, source: str):
        err = regoAddModule(self._impl, name, source)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

    def add_data_json(self, json: str):
        err = regoAddDataJSON(self._impl, json)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

    def add_data(self, data):
        self.add_data_json(json.dumps(data))

    def set_input_json(self, json: str):
        err = regoSetInputJSON(self._impl, json)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

    def set_input(self, data):
        self.set_input_json(json.dumps(data))

    @property
    def debug_enabled(self) -> bool:
        return regoGetDebugEnabled(self._impl)

    @debug_enabled.setter
    def debug_enabled(self, value: bool):
        regoSetDebugEnabled(self._impl, value)

    def set_debug_path(self, path: str):
        err = regoSetDebugPath(self._impl, path)
        if err == REGO_ERROR:
            raise RegoError(regoGetError(self._impl))

    @property
    def well_formed_checks_enabled(self) -> bool:
        return regoGetWellFormedChecksEnabled(self._impl)

    @well_formed_checks_enabled.setter
    def well_formed_checks_enabled(self, value: bool):
        regoSetWellFormedChecksEnabled(self._impl, value)

    def query(self, query: str) -> Output:
        impl = regoQuery(self._impl, query)
        if impl == 0:
            raise RegoError(regoGetError(self._impl))

        return Output(impl)

    @property
    def strict_built_in_errors(self) -> bool:
        return regoGetStrictBuiltInErrors(self._impl)

    @strict_built_in_errors.setter
    def strict_built_in_errors(self, value: bool):
        regoSetStrictBuiltInErrors(self._impl, value)
