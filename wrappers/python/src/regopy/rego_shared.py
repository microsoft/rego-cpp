from collections.abc import Mapping, Sequence, Set
import ctypes
from enum import IntEnum
import os
from typing import Any


class LogLevel(IntEnum):
    NONE = 0
    ERROR = 1
    OUTPUT = 2
    WARN = 3
    INFO = 4
    DEBUG = 5
    TRACE = 6


class Code(IntEnum):
    OK = 0
    ERROR = 1
    ERROR_BUFFER_TOO_SMALL = 2
    ERROR_INVALID_LOG_LEVEL = 3
    ERROR_INPUT_NULL = 5
    ERROR_INPUT_MISSING_ARGUMENTS = 6
    ERROR_INPUT_OBJECT_ITEM = 7


class NodeKind(IntEnum):
    """Enumeration of node types."""

    Binding = 1000
    Var = 1001
    Term = 1002
    Scalar = 1003
    Array = 1004
    Set = 1005
    Object = 1006
    ObjectItem = 1007
    Int = 1008
    Float = 1009
    String = 1010
    True_ = 1011
    False_ = 1012
    Boolean = 1111  # simplifies issue with True and False
    Null = 1013
    Undefined = 1014
    Terms = 1015
    Bindings = 1016
    Results = 1017
    Result = 1018
    Error = 1800
    ErrorMessage = 1801
    ErrorAst = 1802
    ErrorCode = 1803
    ErrorSeq = 1804
    Internal = 1999


class RegoError(Exception):
    """Exception raised when an error occurs in the rego-cpp interpreter."""

    def __init__(self, message: str, code=Code.ERROR):
        """Initializer.

        Args:
            message (str): The error message.
        """
        self.code = Code(code)
        Exception.__init__(self, message)


module_dir = os.path.dirname(__file__)
if os.path.exists(os.path.join(module_dir, "rego_shared.dll")):
    path = os.path.join(module_dir, "rego_shared.dll")
elif os.path.exists(os.path.join(module_dir, "librego_shared.so")):
    path = os.path.join(module_dir, "librego_shared.so")
elif os.path.exists(os.path.join(module_dir, "librego_shared.dylib")):
    path = os.path.join(module_dir, "librego_shared.dylib")
else:
    raise RegoError("Could not find rego_shared library")

rego = ctypes.cdll.LoadLibrary(path)


rego.regoSetLogLevel.restype = ctypes.c_uint32
rego.regoSetLogLevel.argtypes = [ctypes.c_void_p, ctypes.c_uint32]


def rego_set_log_level(impl: ctypes.c_void_p, level: LogLevel):
    res = rego.regoSetLogLevel(impl, level)
    if res != Code.OK:
        raise RegoError("Could not set log level", res)


rego.regoSetDefaultLogLevel.restype = ctypes.c_uint32
rego.regoSetDefaultLogLevel.argtypes = [ctypes.c_uint32]


def rego_set_default_log_level(level: LogLevel):
    res = rego.regoSetDefaultLogLevel(level)
    if res != Code.OK:
        raise RegoError("Could not set default log level", res)


rego.regoGetLogLevel.restype = ctypes.c_uint32
rego.regoGetLogLevel.argtypes = [ctypes.c_void_p]


def rego_get_log_level(impl: ctypes.c_void_p):
    value = rego.regoGetLogLevel(impl)
    try:
        return LogLevel(value)
    except ValueError:
        raise RegoError("Could not get the log level", Code.ERROR)


rego.regoLogLevelFromString.restype = ctypes.c_uint32
rego.regoLogLevelFromString.argtypes = [ctypes.c_char_p]


def rego_log_level_from_string(level: str):
    p = ctypes.create_string_buffer(level.encode("utf-8"))
    value = rego.regoLogLevelFromString(p)
    try:
        return LogLevel(value)
    except ValueError:
        raise RegoError("Could not convert the string to a log level", Code.ERROR)


rego.regoBuildInfoSize.restype = ctypes.c_uint32
rego.regoBuildInfoSize.argtypes = []
rego.regoBuildInfo.restype = ctypes.c_uint32
rego.regoBuildInfo.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_build_info() -> str:
    size = rego.regoBuildInfoSize()
    buf = ctypes.create_string_buffer(size)
    res = rego.regoBuildInfo(buf, size)
    if res != Code.OK:
        raise RegoError("Unable to obtain build info", res)

    return buf.value.decode("utf-8")


rego.regoVersionSize.restype = ctypes.c_uint32
rego.regoVersionSize.argtypes = []
rego.regoVersion.restype = ctypes.c_uint32
rego.regoVersion.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_version() -> str:
    size = rego.regoVersionSize()
    buf = ctypes.create_string_buffer(size)
    res = rego.regoVersion(buf, size)
    if res != Code.OK:
        raise RegoError("Unable to obtain version", res)

    return buf.value.decode("utf-8")


rego.regoErrorSize.restype = ctypes.c_uint32
rego.regoErrorSize.argtypes = [ctypes.c_void_p]
rego.regoError.restype = ctypes.c_uint32
rego.regoError.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_get_error(impl: ctypes.c_void_p) -> str:
    size = rego.regoErrorSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoError(impl, buf, size)
    if res != Code.OK:
        raise RegoError("Unable to obtain error message", res)

    return buf.value.decode("utf-8")


rego.regoNew.restype = ctypes.c_void_p
rego.regoNew.argtypes = []
rego_new = rego.regoNew


rego.regoFree.restype = None
rego.regoFree.argtypes = [ctypes.c_void_p]
rego_free = rego.regoFree


rego.regoAddModule.restype = ctypes.c_uint32
rego.regoAddModule.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]


def rego_add_module(impl: ctypes.c_void_p, name: str, module: str):
    p_name = ctypes.create_string_buffer(name.encode("utf-8"))
    p_module = ctypes.create_string_buffer(module.encode("utf-8"))
    res = rego.regoAddModule(impl, p_name, p_module)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoAddDataJSON.restype = ctypes.c_uint32
rego.regoAddDataJSON.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_add_data_json(impl: ctypes.c_void_p, data: str):
    p_data = ctypes.create_string_buffer(data.encode("utf-8"))
    res = rego.regoAddDataJSON(impl, p_data)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetInputTerm.restype = ctypes.c_uint32
rego.regoSetInputTerm.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_set_input_term(impl: ctypes.c_void_p, term: str):
    p_input = ctypes.create_string_buffer(term.encode("utf-8"))
    res = rego.regoSetInputTerm(impl, p_input)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetInput.restype = ctypes.c_uint32
rego.regoSetInput.argtypes = [ctypes.c_void_p, ctypes.c_void_p]


def rego_set_input(impl: ctypes.c_void_p, p_input: ctypes.c_void_p):
    res = rego.regoSetInput(impl, p_input)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetDebugEnabled.restype = None
rego.regoSetDebugEnabled.argtypes = [ctypes.c_void_p, ctypes.c_bool]


def rego_set_debug_enabled(impl: ctypes.c_void_p, enabled: bool):
    return rego.regoSetDebugEnabled(impl, enabled)


rego.regoGetDebugEnabled.restype = ctypes.c_bool
rego.regoGetDebugEnabled.argtypes = [ctypes.c_void_p]


def rego_get_debug_enabled(impl: ctypes.c_void_p) -> bool:
    return rego.regoGetDebugEnabled(impl)


rego.regoSetDebugPath.restype = ctypes.c_uint32
rego.regoSetDebugPath.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_set_debug_path(impl: ctypes.c_void_p, path: str):
    p_path = ctypes.create_string_buffer(path.encode("utf-8"))
    res = rego.regoSetDebugPath(impl, p_path)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetWellFormedChecksEnabled.restype = None
rego.regoSetWellFormedChecksEnabled.argtypes = [ctypes.c_void_p, ctypes.c_bool]


def rego_set_wf_checks_enabled(impl: ctypes.c_void_p, enabled: bool):
    return rego.regoSetWellFormedChecksEnabled(impl, enabled)


rego.regoGetWellFormedChecksEnabled.restype = ctypes.c_bool
rego.regoGetWellFormedChecksEnabled.argtypes = [ctypes.c_void_p]


def rego_get_wf_checks_enabled(impl: ctypes.c_void_p) -> bool:
    return rego.regoGetWellFormedChecksEnabled(impl)


rego.regoSetStrictBuiltInErrors.restype = None
rego.regoSetStrictBuiltInErrors.argtypes = [ctypes.c_void_p, ctypes.c_bool]


def rego_set_strict_builtin_errors(impl: ctypes.c_void_p, enabled: bool):
    return rego.regoSetStrictBuiltInErrors(impl, enabled)


rego.regoGetStrictBuiltInErrors.restype = ctypes.c_bool
rego.regoGetStrictBuiltInErrors.argtypes = [ctypes.c_void_p]


def rego_get_strict_builtin_errors(impl: ctypes.c_void_p) -> bool:
    return rego.regoGetStrictBuiltInErrors(impl)


rego.regoIsAvailableBuiltIn.restype = ctypes.c_bool
rego.regoIsAvailableBuiltIn.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_is_available_builtin(impl: ctypes.c_void_p, name: str) -> bool:
    p_name = ctypes.create_string_buffer(name.encode("utf-8"))
    return rego.regoIsAvailableBuiltIn(impl, p_name)


rego.regoQuery.restype = ctypes.c_void_p
rego.regoQuery.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_query(impl: ctypes.c_void_p, query: str) -> ctypes.c_void_p:
    p_query = ctypes.create_string_buffer(query.encode("utf-8"))
    p_output = rego.regoQuery(impl, p_query)
    if p_output == 0:
        raise RegoError(rego_get_error(impl))

    return p_output


rego.regoAddEntrypoint.restype = ctypes.c_uint32
rego.regoAddEntrypoint.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_add_entrypoint(impl: ctypes.c_void_p, entrypoint: str):
    p_entrypoint = ctypes.create_string_buffer(entrypoint.encode("utf-8"))
    res = rego.regoAddEntrypoint(impl, p_entrypoint)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoBuild.restype = ctypes.c_void_p
rego.regoBuild.argtypes = [ctypes.c_void_p]


def rego_build(impl: ctypes.c_void_p) -> ctypes.c_void_p:
    p_bundle = rego.regoBuild(impl)
    if p_bundle == 0:
        raise RegoError(rego_get_error(impl))

    return p_bundle


rego.regoBundleLoad.restype = ctypes.c_void_p
rego.regoBundleLoad.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_bundle_load(impl: ctypes.c_void_p, bundle_dir: str):
    p_bundle_dir = ctypes.create_string_buffer(bundle_dir.encode("utf-8"))
    p_bundle = rego.regoBundleLoad(impl, p_bundle_dir)
    if p_bundle == 0:
        raise RegoError(rego_get_error(impl))

    return p_bundle


rego.regoBundleLoadBinary.restype = ctypes.c_void_p
rego.regoBundleLoadBinary.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_bundle_load_binary(impl: ctypes.c_void_p, bundle_path: str):
    p_bundle_dir = ctypes.create_string_buffer(bundle_path.encode("utf-8"))
    p_bundle = rego.regoBundleLoadBinary(impl, p_bundle_dir)
    if p_bundle == 0:
        raise RegoError(rego_get_error(impl))

    return p_bundle


rego.regoBundleSave.restype = ctypes.c_uint32
rego.regoBundleSave.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]


def rego_bundle_save(impl: ctypes.c_void_p, bundle_dir: str, bundle: ctypes.c_void_p):
    p_bundle_dir = ctypes.create_string_buffer(bundle_dir.encode("utf-8"))
    res = rego.regoBundleSave(impl, p_bundle_dir, bundle)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoBundleSaveBinary.restype = ctypes.c_uint32
rego.regoBundleSaveBinary.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]


def rego_bundle_save_binary(impl: ctypes.c_void_p, bundle_path: str, bundle: ctypes.c_void_p):
    p_bundle_dir = ctypes.create_string_buffer(bundle_path.encode("utf-8"))
    res = rego.regoBundleSaveBinary(impl, p_bundle_dir, bundle)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetQuery.restype = ctypes.c_uint32
rego.regoSetQuery.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_set_query(impl: ctypes.c_void_p, query: str):
    p_query = ctypes.create_string_buffer(query.encode("utf-8"))
    res = rego.regoSetQuery(impl, p_query)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl))


rego.regoBundleQuery.restype = ctypes.c_void_p
rego.regoBundleQuery.argtypes = [ctypes.c_void_p, ctypes.c_void_p]


def rego_bundle_query(impl: ctypes.c_void_p, bundle: ctypes.c_void_p) -> ctypes.c_void_p:
    p_output = rego.regoBundleQuery(impl, bundle)
    if p_output == 0:
        raise RegoError(rego_get_error(impl))

    return p_output


rego.regoBundleQueryEntrypoint.restype = ctypes.c_void_p
rego.regoBundleQueryEntrypoint.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_char_p]


def rego_bundle_query_entrypoint(impl: ctypes.c_void_p, bundle: ctypes.c_void_p, entrypoint: str) -> ctypes.c_void_p:
    p_entrypoint = ctypes.create_string_buffer(entrypoint.encode("utf-8"))
    p_output = rego.regoBundleQueryEntrypoint(impl, bundle, p_entrypoint)
    if p_output == 0:
        raise RegoError(rego_get_error(impl))

    return p_output


# Output functions

rego.regoOutputOk.restype = ctypes.c_bool
rego.regoOutputOk.argtypes = [ctypes.c_void_p]


def rego_output_ok(impl: ctypes.c_void_p) -> bool:
    return rego.regoOutputOk(impl)


rego.regoOutputJSONSize.restype = ctypes.c_uint32
rego.regoOutputJSONSize.argtypes = [ctypes.c_void_p]
rego.regoOutputJSON.restype = ctypes.c_uint32
rego.regoOutputJSON.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_output_string(impl: ctypes.c_void_p) -> str:
    size = rego.regoOutputJSONSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoOutputJSON(impl, buf, size)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)

    return buf.value.decode("utf-8")


rego.regoOutputNode.restype = ctypes.c_void_p
rego.regoOutputNode.argtypes = [ctypes.c_void_p]
rego_output_node = rego.regoOutputNode


rego.regoOutputExpressionsAtIndex.restype = ctypes.c_void_p
rego.regoOutputExpressionsAtIndex.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
rego_output_expressions_at_index = rego.regoOutputExpressionsAtIndex


rego.regoOutputBindingAtIndex.restype = ctypes.c_void_p
rego.regoOutputBindingAtIndex.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_char_p]


def rego_output_binding_at_index(impl: ctypes.c_void_p, index: int, name: str) -> ctypes.c_void_p:
    p_name = ctypes.create_string_buffer(name.encode("utf-8"))
    p_node = rego.regoOutputBindingAtIndex(impl, index, p_name)
    if p_node == 0:
        raise RegoError(rego_get_error(impl))

    return p_node


rego.regoFreeOutput.restype = None
rego.regoFreeOutput.argtypes = [ctypes.c_void_p]
rego_free_output = rego.regoFreeOutput


# Node functions
rego.regoNodeType.restype = ctypes.c_uint32
rego.regoNodeType.argtypes = [ctypes.c_void_p]


def rego_node_type(impl: ctypes.c_void_p) -> NodeKind:
    kind = NodeKind(rego.regoNodeType(impl))
    if kind in (NodeKind.True_, NodeKind.False_):
        return NodeKind.Boolean

    return kind


rego.regoNodeTypeNameSize.restype = ctypes.c_uint32
rego.regoNodeTypeNameSize.argtypes = [ctypes.c_void_p]
rego.regoNodeTypeName.restype = ctypes.c_uint32
rego.regoNodeTypeName.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_node_type_name(impl: ctypes.c_void_p) -> str:
    size = rego.regoNodeTypeNameSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoNodeTypeName(impl, buf, size)
    if res != Code.OK:
        raise RegoError("Unable to retrieve node value", res)

    return buf.value.decode("utf-8")


rego.regoNodeValueSize.restype = ctypes.c_uint32
rego.regoNodeValueSize.argtypes = [ctypes.c_void_p]
rego.regoNodeValue.restype = ctypes.c_uint32
rego.regoNodeValue.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_node_value(impl: ctypes.c_void_p) -> str:
    size = rego.regoNodeValueSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoNodeValue(impl, buf, size)
    if res != Code.OK:
        raise RegoError("Unable to retrieve node value", res)

    return buf.value.decode("utf-8")


rego.regoNodeSize.restype = ctypes.c_uint32
rego.regoNodeSize.argtypes = [ctypes.c_void_p]
rego_node_size = rego.regoNodeSize


rego.regoNodeGet.restype = ctypes.c_void_p
rego.regoNodeGet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
rego_node_get = rego.regoNodeGet


rego.regoNodeJSONSize.restype = ctypes.c_uint32
rego.regoNodeJSONSize.argtypes = [ctypes.c_void_p]
rego.regoNodeJSON.restype = ctypes.c_uint32
rego.regoNodeJSON.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_node_json(impl: ctypes.c_void_p) -> str:
    size = rego.regoNodeJSONSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoNodeJSON(impl, buf, size)
    if res != Code.OK:
        raise RegoError("Unable to retrieve node JSON", res)

    return buf.value.decode("utf-8")


# Input functions

rego.regoNewInput.restype = ctypes.c_void_p
rego.regoNewInput.argtypes = []
rego_new_input = rego.regoNewInput

rego.regoInputInt.restype = ctypes.c_uint32
rego.regoInputInt.argtypes = [ctypes.c_void_p, ctypes.c_int64]


def rego_input_int(p_input: ctypes.c_void_p, value: int):
    res = rego.regoInputInt(p_input, value)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputFloat.restype = ctypes.c_uint32
rego.regoInputFloat.argtypes = [ctypes.c_void_p, ctypes.c_double]


def rego_input_float(p_input: ctypes.c_void_p, value: float):
    res = rego.regoInputFloat(p_input, value)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputString.restype = ctypes.c_uint32
rego.regoInputString.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_input_str(p_input: ctypes.c_void_p, value: str):
    p_value = ctypes.create_string_buffer(value.encode("utf-8"))
    res = rego.regoInputString(p_input, p_value)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputBoolean.restype = ctypes.c_uint32
rego.regoInputBoolean.argtypes = [ctypes.c_void_p, ctypes.c_bool]


def rego_input_bool(p_input: ctypes.c_void_p, value: bool):
    res = rego.regoInputBoolean(p_input, value)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputNull.restype = ctypes.c_uint32
rego.regoInputNull.argtypes = [ctypes.c_void_p]


def rego_input_null(p_input: ctypes.c_void_p):
    res = rego.regoInputNull(p_input)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputObjectItem.restype = ctypes.c_uint32
rego.regoInputObjectItem.argtypes = [ctypes.c_void_p]


def rego_input_object_item(p_input: ctypes.c_void_p):
    res = rego.regoInputObjectItem(p_input)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputObject.restype = ctypes.c_uint32
rego.regoInputObject.argtypes = [ctypes.c_void_p, ctypes.c_uint32]


def rego_input_object(p_input: ctypes.c_void_p, count: int):
    res = rego.regoInputObject(p_input, count)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputArray.restype = ctypes.c_uint32
rego.regoInputArray.argtypes = [ctypes.c_void_p, ctypes.c_uint32]


def rego_input_array(p_input: ctypes.c_void_p, count: int):
    res = rego.regoInputArray(p_input, count)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoInputSet.restype = ctypes.c_uint32
rego.regoInputSet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]


def rego_input_set(p_input: ctypes.c_void_p, count: int):
    res = rego.regoInputSet(p_input, count)
    if res != Code.OK:
        raise RegoError("Unable to create input", res)


rego.regoFreeInput.restype = None
rego.regoFreeInput.argtypes = [ctypes.c_void_p]
rego_free_input = rego.regoFreeInput


def rego_input_from_value(value: Any) -> ctypes.c_void_p:
    def add(p_in: ctypes.c_void_p, val: Any):
        if val is True or val is False:
            rego_input_bool(p_in, val)
            return

        if isinstance(val, int):
            rego_input_int(p_in, val)
            return

        if isinstance(val, float):
            rego_input_float(p_in, val)
            return

        if isinstance(val, str):
            rego_input_str(p_in, val)
            return

        if val is None:
            rego_input_null(p_in)
            return

        if isinstance(val, Set):
            for x in val:
                add(p_in, x)

            rego_input_set(p_in, len(val))
            return

        if isinstance(val, Mapping):
            for k, v in val.items():
                add(p_in, k)
                add(p_in, v)
                rego_input_object_item(p_in)

            rego_input_object(p_in, len(val))
            return

        if isinstance(val, Sequence):
            for x in val:
                add(p_in, x)

            rego_input_array(p_in, len(val))
            return

        raise ValueError("Cannot convert object of type " + str(type(val)) + " to rego input")

    p_input = rego_new_input()
    try:
        add(p_input, value)
    except RegoError as ex:
        rego_free_input(p_input)
        raise ex

    return p_input


# Bundle functions
rego.regoBundleOk.restype = ctypes.c_bool
rego.regoBundleOk.argtypes = [ctypes.c_void_p]


def rego_bundle_ok(impl: ctypes.c_void_p) -> bool:
    return rego.regoBundleOk(impl)


rego.regoBundleNode.restype = ctypes.c_void_p
rego.regoBundleNode.argtypes = [ctypes.c_void_p]
rego_bundle_node = rego.regoBundleNode


rego.regoFreeBundle.restype = None
rego.regoFreeBundle.argtypes = [ctypes.c_void_p]
rego_free_bundle = rego.regoFreeBundle
